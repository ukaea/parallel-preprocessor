#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
After installation by pip/system package manager, `geomPipeline.py` should be on executable PATH
On Windows, it is not possible to run geomPipeline.py without make a batch file.

`geomPipeline.py imprint path-to-your-geometry.stp`
this script will generate a config file, save result into a folder has the name of your geometry file stem

test command line arguments by
python3 geomPipeline.py imprint --thread-count 4 --tolerance=0.1 --config  --no-merge ../data/test_geometry/test_geometry.stp && scite config.json

you can keep the `config_file_content` dict in `geomPipeline.py` as it is, which will build a default pipeline

imprint and merge can be split into 2 steps
geomPipeline.py  geometry_file --thread-count 6  --no-merge
geomPipeline.py merge  geometry_file --thread-count 6  

"""

USAGE = """
geomPipeline.py  {imprint|check|detect|decompose|...} input_filename

"""

import sys
import os.path
import json
import copy
from enum import Enum
from collections import OrderedDict
import shutil


# multiprocessing.cpu_count() can get the core number of the CPU
from multiprocessing import cpu_count
import argparse

from pppPipelineController import (
    ppp,
    ppp_start_pipeline,
    ppp_add_argument,
    ppp_parse_input,
    generate_config_file_header,
    generate_config_file,
    ppp_post_process,
)

########################### Geom specific arguments  #################################
class Action(Enum):
    check = "check"
    decompose = "decompose"
    detect = "detect"
    fix = "fix"
    merge = "merge"  # assuming input geometry has been imprinted, just merge
    imprint = "imprint"  # imprint and merge, add `--no-merge` to disable merge
    search = "search"
    tessellate = "tessellate"

    def __str__(self):
        return self.value


###################################################################
debugging = 0  # this debugging only control this python script, set 1 for more


def geom_add_argument(parser):
    parser.add_argument(
        "--metadata", help="input metadata file, only for brep input geometry"
    )

    # set the global parameters
    parser.add_argument(
        "--tolerance", type=float, help="tolerance for imprinting, unit MilliMeter"
    )

    # bool argument for imprint
    parser.add_argument(
        "--no-merge",
        action="store_true",
        help="do not merge the imprinted shapes, for two-step workflow",
    )

    # bool argument for imprint
    parser.add_argument(
        "--use-inscribed-shape",
        action="store_true",
        help="",
    )

    # bool argument for imprint
    parser.add_argument(
        "--ignore-failed",
        action="store_true",
        help="ignore failed (in BOP check, collision detect, etc) solids",
    )

    # it is not arbitrary scaling, but can change output units (m, cm, mm) of STEP file format
    parser.add_argument(
        "-ou", "--output-unit",
        dest="output_unit",
        type=str,
        help="output geometry unit, by default, `MM` for CAD, change unit will cause scaling when read back",
    )

    # it is not arbitrary scaling, but can change output units (m, cm, mm) of STEP file format
    parser.add_argument(
        "--input-unit",
        dest="input_unit",
        type=str,
        help="input geometry unit, by default, only needed for brep file without meta data for length ",
    )
    return parser


############################### arg parse ###############################
parser = argparse.ArgumentParser(usage=USAGE)
# positional argument, the first positional argument can have a default
parser.add_argument(
    "action", nargs="?", type=Action, default=Action.imprint, choices=list(Action)
)
parser = ppp_add_argument(parser)  # must be called after add the action arg
parser = geom_add_argument(parser)

args = parser.parse_args()
config_file_content = copy.copy(generate_config_file_header(args))

# after inputFile is given, all other default filenames are generated
inputFile = ppp_parse_input(args)
inputDir, inputFilename = os.path.split(inputFile)
case_name = inputFilename[: inputFilename.rfind(".")]
######################## module specific ##########################

outputFile = case_name + "_processed.brep"  # saved to case output folder
if args.outputFile:
    outputFile = args.outputFile
    print("args.outputFile = ", args.outputFile)
outputFile = os.path.abspath(outputFile)

# output metadata filename
outputMetadataFile = outputFile[: outputFile.rfind(".")] + "_metadata.json"

################################ geom arg ##############################
hasInputMetadataFile = False
if args.metadata:
    if os.path.exists(args.metadata):
        inputMetadataFile = args.metadata
        hasInputMetadataFile = True
    else:
        raise IOError("input metadata file does not exist: ", args.metadata)
else:
    if inputFile.endswith(".brep")  or inputFile.endswith(".brp"):
        inputMetadataFile = inputFile[: inputFile.rfind(".")] + "_metadata.json"
        if os.path.exists(inputMetadataFile):
            hasInputMetadataFile = True

globalTolerance = 0.001  # length unit is mm
if args.tolerance:
    globalTolerance = args.tolerance

mergeResultShapes = True  # by default, imprinting result should be merged/glued before writing brep file
if args.no_merge != None:
    mergeResultShapes = not args.no_merge  # it maybe list type

ignoreFailed = False
if args.ignore_failed != None and args.ignore_failed:
    ignoreFailed = True

outputUnit = "MM"  # argument can be capital or uncapital letter
if args.output_unit:
    if args.output_unit.upper() in ("MM", "CM", "M", "INCH"):  # STEP supported units
        outputUnit = args.output_unit.upper()

# in CAD, the default lengths unit is MM (millimeter)
inputUnit = "MM"
if args.input_unit:
    if args.input_unit.upper() in ("MM", "CM", "M", "INCH"):  # STEP supported units
        inputUnit = args.input_unit.upper()

if debugging > 0:
    print("action on the geometry is ", args.action)

########################################################

globalParameters = {  # shared by all processors
    "toleranceThreshold": {
        "type": "quantity",
        "value": globalTolerance,
        "unit": "mm",
        "range": [1e-5, 10],
        "doc": "this should be bigger than the tolerance of the shape",
    },
    "scriptFolder": {  # needed by C++ side code to find FreeCADParse.py
        "type": "path",
        "value": os.path.dirname(os.path.abspath(__file__)),
        "doc": "full path of the python scripts needed to run run python script in C++",
    },
}

readers = [  # it is possible to have multiple Reader instances in this list
    {
        "className": "Geom::GeometryReader",
        "dataFileName": inputFile,
        "metadataFileName": None if not hasInputMetadataFile else os.path.abspath(inputMetadataFile),
        "doc": "only step, iges, FCStd, brep+json metadata are supported",
    }
]

writers = [
    {
        "className": "Geom::GeometryWriter",
        "dataFileName": outputFile,
        "mergeResultShapes": {  # this corresponding to Parameter class, this map to App::Parameter<> class
            "type": "bool",
            "value": mergeResultShapes,
            "doc": "control whether imprinted solids will be merged (remove duplicate faces) before write-out",
        },
        "outputUnit": {
            "type": "string",
            "value": outputUnit,
            "doc": "control output geometry length unit, for CAD, default mm",
        },
        "doc": "if no directory part in `outputFile`, saved into the case `outputDir` folder",
    }
]

GeometryShapeChecker = {  # usually GeometryRead has done check after reading
    "className": "Geom::GeometryShapeChecker",
    "doc": "some config entry in processor is mappable to PPP::Processor::Attribute class",
    "output": {  # this corresponding to Parameter class, this map to App::Parameter<> class
        "type": "filename",  # renamed to `path`  type, or support both types
        "value": "shape_check_result.json",
        "doc": "save errors message as json file",
    },
    "checkingBooleanOperation": {
        "type": "bool",
        "value": not ignoreFailed,
        "doc": "check whether this shape is sound to perform boolean operation, by default off",
    },
    "suppressBOPCheckFailed": {
        "type": "bool",
        "value": not ignoreFailed,
        "doc": "whether disable/ suppress this item if BOP check failed, by default off",
    },
}

GeometryPropertyBuilder = {
    "className": "Geom::GeometryPropertyBuilder",
    "doc": "build meta data for the solid shapes",
    "output": {
        "type": "filename",
        "value": "shape_properties.json",
        "doc": "this may used as meta data such as material, hash Id",
    },
}

BoundBoxBuilder = {
    "className": "Geom::BoundBoxBuilder",
    "doc": "calc boundbox for collision detection, decompose",
}

InscribedShapeBuilder = {
    "className": "Geom::InscribedShapeBuilder",
    "doc": "calc max void or hollow space of a shape's oriented boundbox can hold",
    "output": {
        "type": "filename",
        "value": "inscribed_shapes.brep",
        "doc": "optional: result of inscribed shape calculation",
    },
}

# shared by all actions
basic_processors = [GeometryShapeChecker, GeometryPropertyBuilder, BoundBoxBuilder]

GeometrySearchBuilder = {
    "className": "Geom::GeometrySearchBuilder",
    "doc": "",
    "output": {
        "type": "filename",  # saved matched shapes into the case working directory
        "value": "research_results.brep",
        "doc": "filename to save the shapes matching search result, file format from suffix",
    },
    "suppressMatched": {
        "type": "bool",  # save the unmatched by appending a writer after
        "value": True,
        "doc": "matched suppressed from downstream processors and writer",
    },
    "searchType": {
        "type": "enum",
        "value": "BoundBox",
        "doc": "all possible enums:  GeometryFile, UniqueId, BoundBox ...",
    },
    "searchValues": {
        "type": "json",  # must be a list, convert into std::vector<Object> manually
        "value": [[0, 0, 0, 10, 10, 10]],  # one BoundBox is a list of 6 scalars
        "doc": " search criteria values depends on search type",
    },
}

CollisionDetector = {
    "className": "Geom::CollisionDetector",
    "doc": "detect contact or collision, clearance relationship between shape items",
    "tolerance": {
        "type": "quantity",
        "value": 0.001,
        "unit": "mm",
        "range": [1e-5, 1],
        "doc": "tolerance for boolean operation",
    },
    "clearanceThreshold": {
        "type": "quantity",
        "value": 0.5,
        "unit": "mm",
        "range": [1e-5, 1],
        "doc": "CollisionDector needs this threshold for potential contact after deformation",
    },
    "weakInterferenceThreshold": {
        "type": "float",
        "value": 0.1,
        "range": [1e-5, 0.5],
        "doc": "ratio of common volume to that of the smaller shape, after boolean fragment",
    },
    "suppressFloating": {
        "type": "bool",
        "value": False,
        "doc": "part without contact with other parts will be suppressed if set True",
    },
    "suppressErroneous": {
        "type": "bool",
        "value": True,
        "doc": "collision detection failed items will be suppressed if set True",
    },
    "ignoreUnknownCollisionType": {
        "type": "bool",
        "value": ignoreFailed,
        "range": [True, False],
        "doc": "ignore solids with BOP check error and carry on downstream processing",
    },
    "output": {
        "type": "filename",
        "value": "myCollisionInfos.json",
        "doc": "collision type info dump, implemented in CollisionDetector parental class",
    },
}

# Geom::CollisionDetector is now parental class for PPP::GeometryImprinter
GeometryImprinter = copy.copy(CollisionDetector)
GeometryImprinter["className"] = "Geom::GeometryImprinter"
GeometryImprinter["doc"] = "imprint (shared face merging) for face contact"

# post modification check before writing, should skip the suppressed items
post_GeometryShapeChecker = copy.copy(GeometryShapeChecker)
post_GeometryPropertyBuilder = copy.copy(GeometryPropertyBuilder)
post_GeometryPropertyBuilder["output"]["value"] = outputMetadataFile
# for fixing and imprinting
post_processors = [post_GeometryShapeChecker, post_GeometryPropertyBuilder]

# not  working
GeometryFixer = {"className": "Geom::GeometryFixer"}

GeometryDecomposer = {
    "className": "Geom::GeometryDecomposer",
    # output filename step can be given
    "numberOfPartitions": {
        "type": "int",
        "value": cpu_count(),
        "doc": "parbition number to be split into",
    },
    "doc": "decompose assembly into subassemblies and generate partitionId vector",
}

# parallel boolean operations
GeometryEnclosureBuilder = {
    "className": "Geom::GeometryEnclosureBuilder",  # todo: rename to BooleanOperator
    "enclosureGeometry": {
        "type": "filename",
        "value": "../data/test_geometry/enclosureGeometry.brep",
        "doc": "input geometry define a space to substract any solid inside",
    },
    "output": {
        "type": "filename",
        "value": "enclosureResult.brep",
        "doc": "result void space/vacuum space for some kind of simulation",
    },
    "doc": "this doc may be extracted from doc string of C++ class later by tool",
}

GeometryFaceter = {
    "className": "Geom::GeometryFaceter",
    "tessellationResolution": {
        "type": "quantity",
        "value": 0.1,
        "unit": "mm",
        "range": [1e-3, 10],
        "doc": "length resolution or mesh size for tessellation",
    },
    "output": {"type": "filename", "value": "facet_result.stl", "doc": ""},
    "doc": "some config entry in processor is mappable to PPP::Processor::Attribute class",
}

############### module specific pipeline building ###################
config_file_content["globalParameters"] = globalParameters
config_file_content["readers"] = readers

processors = basic_processors
if args.action == Action.check:
    pass
elif args.action == Action.decompose:
    processors.append(GeometryDecomposer)
elif args.action == Action.detect:
    processors.append(CollisionDetector)
elif args.action == Action.fix:
    processors.append(GeometryFixer)
    processors.append(GeometryShapeChecker)
    processors.append(post_GeometryPropertyBuilder)
    config_file_content["writers"] = writers
elif args.action == Action.imprint:
    if args.use_inscribed_shape:
        processors.append(InscribedShapeBuilder)
    processors.append(GeometryImprinter)
    processors += post_processors
    config_file_content["writers"] = writers
elif args.action == Action.merge:
    # just keep processors = basic_processors
    config_file_content["writers"] = writers
elif args.action == Action.search:
    processors.append(GeometrySearchBuilder)
elif args.action == Action.tessellate:
    processors.append(GeometryFaceter)
else:
    print("Action `{}` is not supported by the pipeline".format(args.action))
    sys.exit(-1)
config_file_content["processors"] = processors

##########################################################
## to test multiple data sharing one pipeline, turn off after testing
multifile_mode = False
if multifile_mode:
    config_file_content["readers"].append(
        {"className": "GeometryReader", "dataFileName": inputFile}
    )
    config_file_content["writers"].append(
        {"className": "GeometryWriter", "dataFileName": outputFile}
    )

config_file_name = generate_config_file(config_file_content, args)

###########################  module specific process ####################
# timeit has been done in c++ log
# p = ppp.GeometryPipeline(config_file_name)
ppp_start_pipeline(config_file_name)

############### module specific  post process ############################
ppp_post_process(args)
