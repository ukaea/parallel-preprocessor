#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# copyright note: this is a personal contribution from Qingfeng Xia
# created and tested on Sunday March 22, 2020

"""
This is a demonstration of usage of PPP module, using CommandLineProcessor
ParallelAccessorTest.cpp is a demo (test) of instantiation of ProcessorTemplate class in C++

todo: make it a class, to be reused with GeomPipeline.py
"""

import sys
import os.path
import json
import copy
from collections import OrderedDict
from multiprocessing import cpu_count
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import shutil

from pppStartPipeline import ppp_start_pipeline, ppp

if sys.version_info.major == 2:
    print("python 2 is not supported! using python3")
    sys.exit()


###################################################################
USAGE = "input data file is the only compulsory argument, get help by -h argument"

debugging = 0  # this verbosity only control this python script,
# args.verbosity argument control the console verbosity
if debugging:
    print("python argv:", sys.argv)


def ppp_add_argument(parser):
    # the only compulsory arg
    parser.add_argument(
        "inputFile", help="input data file, detect type from file suffix"
    )

    # optional arguments
    parser.add_argument(
        "-o", "--outputFile", help="output file name (without folder path)"
    )
    # do not use "nargs=1" it will return args.outputFile as a list instead of string
    parser.add_argument(
        "--workingDir", help="working folder path, by default pwd"
    )
    parser.add_argument(
        "--outputDir",
        help="output folder path, by default a subfolder in workingDir",
    )

    parser.add_argument(
        "--config",
        dest="config_only",
        action="store_true",
        help=" only generate config.json without run the pipeline",
    )

    parser.add_argument(
        "-v",
        "--verbosity",
        type=str,
        default="INFO",
        help="verbosity: for console or termimal: DEBUG, PROGRESS, INFO, WARNING, ERROR",
    )
    parser.add_argument(
        "-nt",
        "--thread-count",
        dest="thread_count",
        type=int,
        default=cpu_count(),
        help="number of thread to use, max = hardware core number",
    )
    return (
        parser  # must return parser, otherwise, modification to input parser will lost
    )


############################ input and output ##############################
def is_url(s):
    for p in ["http", "ftp"]:
        if s.find(p) == 0:
            return True
    return False


def ppp_parse_input(args):
    if args.inputFile:
        if is_url(args.inputFile):
            # download to current folder
            import urllib.request

            urllib.request.urlretrieve(args.inputFile, "input_data")
            return "input_data"

        elif os.path.exists(args.inputFile):
            return args.inputFile
        else:
            raise IOError("input file does not exist: ", args.inputFile)
    else:
        raise Exception("input file must be given as an argument")


def ppp_check_argument(args):
    thread_count = cpu_count() - 1
    if args.thread_count:
        if args.thread_count > 0 and args.thread_count <= thread_count:
            thread_count = args.thread_count
        else:
            args.thread_count = cpu_count()  # can set args.value just like a dict
    if not args.workingDir:
        args.workingDir = "./"  # debug and config file will be put here,
        # before copy to outputDir at the end of processing in C++ Context::finalize()
    # print(args.thread_count)


####################################################################
# configuration header, shared by all pipelines
# styling: https://google.github.io/styleguide/jsoncstyleguide.xml
def generate_config_file_header(args):
    ppp_check_argument(args)

    return OrderedDict(
        {
            "name": "parallel-preprocessor",
            "version": 0.3,  # is that possible to get git commit
            "parallelism": {
                "DistributiveMode": False,  # multiple nodes distributive by MPI
                "numberOfNodes": 1,  # MPI nodes, currently only 1
                "threadsOnNode": args.thread_count,  # <1 mean hardware thread count
                "sharedMemoryAddress": True,  # shared memory address on each node
            },
            "dataStorage": {
                "workingDir": args.workingDir,
                "dataStorageType": "Folder",
                # "dataStoragePath": outputDir, # default input file stem as result subfolder name
            },
            "logger": {
                "verbosity": args.verbosity,  # print to console: DEBUG, PROGRESS, INFO, WARNING
                "logFileName": "debug_info.log",  # only INFO level or above will print to console
                "logLevel": "DEBUG",  # DEBUG, PROGRESS, INFO, WARNING
            },
        }
    )


def ppp_post_process(args):
    #
    inputFile = ppp_parse_input(args)
    inputDir, inputFilename = os.path.split(inputFile)
    case_name = inputFilename[: inputFilename.rfind(".")]

    outputDir = (  # this naming must be consistent with DataStorage::generateStoragePath(input_name)
        args.workingDir + os.path.sep + case_name + "_processed"
    )  # can be detected from output_filename full path
    if args.outputDir:
        outputDir = args.outputDir
    linkToInputFile = outputDir + os.path.sep + inputFilename

    if os.path.exists(outputDir):
        try:  # failed on windows
            os.symlink(inputFile, linkToInputFile)
        except:
            print("failed to create symbolic link", linkToInputFile)


#####################################################


def generate_config_file(config_file_content, args):
    # parse args first, the write config file

    config_file_given = False
    generated_config_file_name = "config.json"
    # input json file is config, but not geomtry input manifest file
    if args.inputFile.find(".json") > 0:
        with open(args.inputFile, "r") as f:
            _json_file_content = json.loads(f.read())
            # check compulsory key in config.json file
            if "readers" in _json_file_content:
                config_file_given = True
                input_config_file_name = args.inputFile
            else:
                pass  # it is a multiple geometry-material manifest json file
    #
    if config_file_given:
        return input_config_file_name
    else:
        with open(generated_config_file_name, "w", encoding="utf-8") as jf:
            json.dump(config_file_content, jf, ensure_ascii=False, indent=4)
            if debugging > 0:
                print("write out the configuration file", generated_config_file_name)

        if args.config_only:
            sys.exit()
        else:
            return generated_config_file_name


#####################  module specific processor config #####################

Reader = {
    "className": "PPP::Reader",
    "dataFileName": None,
    "doc": "only step, iges, FCStd, brep+json metadata are supported",
}

Writer = {
    "className": "PPP::Writer",
    "dataFileName": None,
    "doc": "if no directory part in `outputFile`, saved into the case `outputDir`",
}

# ProcessorTemplate<> can not be created from this json config object
CommandLineProcessor = {
    "className": "PPP::CommandLineProcessor",
    "doc": "run command line on each item (file path)",
    "commandLinePattern": {
        "type": "string",
        "value": "echo {}",
        "doc": "C++20 or python format, 1st arg is input filename, second output",
    },
    "blocking": {
        "type": "bool",
        "value": True,
        "doc": "blocking the calling thread until the ",
    },
    "usingPipeStream": False,  #  must be true if boost::process is not enabled
    "report": {
        "type": "filename",
        "value": "item_result.json",
        "doc": "save the item result output and error message",
    },
}

###############################################################################


class PipelineController(object):
    def __init__(self):
        pass

    def config(self):
        # this must be override by derived class

        parser = argparse.ArgumentParser(usage=USAGE)
        parser = ppp_add_argument(parser)
        args = parser.parse_args()

        # after inputFile is given, all other default filenames are generated
        inputFile = ppp_parse_input(args)
        inputDir, inputFilename = os.path.split(inputFile)
        case_name = inputFilename[: inputFilename.rfind(".")]
        ######################## module specific ##########################
        outputFile = case_name + "_processed.txt"  # saved to case output folder
        if args.outputFile:
            outputFile = args.outputFile

        Reader["dataFileName"] = inputFile
        Writer["dataFileName"] = outputFile

        config_dict = copy.copy(generate_config_file_header(args))
        config_dict["readers"] = [Reader]
        config_dict["processors"] = [CommandLineProcessor]

        self.config_file_name = generate_config_file(config_dict, args)
        self.args = args

    def process(self):

        ppp_start_pipeline(self.config_file_name)

        ######################### post process ##########################
        ppp_post_process(self.args)


if __name__ == "__main__":
    pc = PipelineController()
    pc.config()
    pc.process()
