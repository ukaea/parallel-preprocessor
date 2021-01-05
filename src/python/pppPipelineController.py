#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# copyright note: this is a personal contribution from Qingfeng Xia
# created and tested on Sunday March 22, 2020

"""
This script defines utility functions that can be shared by all pipeline controllers such as GeomPipeline.py.
By using those functions,  consistent command line argument and config.json header can be achived.

At the end of this script. a demonstration of usage of PPP core module, using CommandLineProcessor.
ParallelAccessorTest.cpp is a demo (test) of instantiation of ProcessorTemplate class in C++

"""

import sys
import os.path
import shutil
import json
import copy
from collections import OrderedDict
from multiprocessing import cpu_count


import argparse
import shutil

from pppStartPipeline import ppp_start_pipeline, ppp

if sys.version_info.major == 2:
    print("python 2 is not supported! using python3")
    sys.exit()


###################################################################
USAGE = "input data file is the only compulsory argument, get help by -h argument"
generated_config_file_name = "config.json"
default_log_filename = "debug_info.log"

debugging = 0  # this verbosity only control this python script,
# args.verbosity argument control the console verbosity
if debugging:
    print("python argv:", sys.argv)


def ppp_add_argument(parser):
    # call this function after the first positional arg has been added to parser 

    # the second positional arg for input file
    parser.add_argument(
        "input", help="input data file, detect type from file suffix",
    )

    # optional arguments
    # do not use "nargs=1", it will return args.outputFile as a list instead of string
    parser.add_argument(
        "--working-dir", help="working folder path, by default, the current working folder",
        dest = "workingDir"
    )

    parser.add_argument(
        "-o", "--output-file", help="output file name relative to working-dir or absolute path",
        dest = "outputFile"
    )

    # parser.add_argument(
    #     "--output-dir",
    #     help="output folder path, by default a subfolder (input file stem) in the working-dir",
    #     dest = "outputDir"
    # )

    parser.add_argument(
        "--config",
        dest="config_only",
        action="store_true",
        help=" only generate config.json without run the pipeline processors",
    )

    parser.add_argument(
        "-nt",
        "--thread-count",
        dest="thread_count",
        type=int,
        default=cpu_count(),
        help="number of thread to use, by default, hardware core number",
    )

    parser.add_argument(
        "-v",
        "--verbosity",
        type=str,
        default="INFO",
        help="verbosity: for console or terminal: DEBUG, PROGRESS, INFO, WARNING, ERROR",
    )

    # must return parser, otherwise, modification to input parser will lost
    return parser  


############################ input and output ##############################
def is_url(s):
    for p in ["http", "ftp"]:
        if s.find(p) == 0:
            return True
    return False


def ppp_parse_input(args):
    if args.input:
        if is_url(args.input):
            # download to current folder
            import urllib.request

            urllib.request.urlretrieve(args.input, "downloaded_input_data")
            args.input = os.path.abspath("downloaded_input_data")

        elif os.path.exists(args.input):
            args.input = os.path.abspath(args.input)
        else:
            raise IOError("input file does not exist: ", args.input)
    else:
        raise Exception("input file must be given as an argument")
    return args.input  # must be abspath, as current dir may change


def ppp_check_argument(args):
    thread_count = cpu_count() - 1
    if args.thread_count:
        if args.thread_count > 0 and args.thread_count <= thread_count:
            thread_count = args.thread_count
        else:
            args.thread_count = cpu_count()  # can set args.value just like a dict
    if not args.workingDir:
        args.workingDir = os.path.abspath("./")  # debug and config file will be put here,
        # before copy to outputDir at the end of processing in C++ Context::finalize()


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
                "logFileName": default_log_filename,  # only INFO level or above will print to console
                "logLevel": "DEBUG",  # DEBUG, PROGRESS, INFO, WARNING
            },
        }
    )

def ppp_get_case_name(inputFile):
    inputDir, inputFilename = os.path.split(inputFile)
    case_name = inputFilename[: inputFilename.rfind(".")]
    return case_name

def ppp_get_output_dir(args):
    # if args.outputDir:
    #     outputDir = os.path.abspath(args.outputDir)
    # else:

    inputFile = ppp_parse_input(args)
    case_name = ppp_get_case_name(inputFile)
    # this naming must be consistent with DataStorage::generateStoragePath(input_name)
    caseDir = (args.workingDir + os.path.sep + case_name + "_processed")  # can be detected from output_filename full path
    return caseDir

def ppp_post_process(args):
    # currently, only make symbolic link to input file into the output dir

    inputFile = ppp_parse_input(args)
    inputDir, inputFilename = os.path.split(inputFile)
    caseDir = ppp_get_output_dir(args)
    linkToInputFile = caseDir + os.path.sep + inputFilename

    if os.path.exists(generated_config_file_name):
        shutil.copyfile(generated_config_file_name, caseDir + os.path.sep + generated_config_file_name)
    else:
        print("config file does not exists in the current dir", os.curdir)
    
    if os.path.exists(default_log_filename):
        shutil.copyfile(default_log_filename, caseDir + os.path.sep + default_log_filename)
    else:
        print("log file does not exists in the current dir", os.curdir)

    if os.path.exists(caseDir):
        try:  # failed on windows
            os.symlink(inputFile, linkToInputFile)
        except:
            print("failed to create symbolic link", linkToInputFile)


def generate_config_file(config_file_content, args):
    # parse args first, the write config file

    config_file_given = False
    # input json file is config, but not geomtry input manifest file
    if args.input.find(".json") > 0:
        with open(args.input, "r") as f:
            _json_file_content = json.loads(f.read())
            # check compulsory key in config.json file
            if "readers" in _json_file_content:
                config_file_given = True
                input_config_file_name = args.input
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

######################### module specific pipeline control ############################


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
        case_name = ppp_get_case_name(inputFile)
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
