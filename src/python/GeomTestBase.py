# -*- coding: utf-8 -*-
# copyright  Qingfeng Xia
# UKAEA internal usage only

from __future__ import print_function, division
import sys
import os.path
import os
from tempfile import gettempdir
import json

import unittest

######################################################

from pppStartPipeline import ppp_start_pipeline
from detectFreeCAD import append_freecad_mod_path

append_freecad_mod_path()
try:
    import FreeCAD
except ImportError:
    print("freecad is not installed or detectable, exit from this script")
    sys.exit(0)

import FreeCAD as App
import Part


######################### utilities ############################


def generate_config(inf, outf, action="imprint", args="", default_config_file_name="config.json"):
    # generate a configuration without running it

    this_geomPipeline = os.path.dirname(os.path.abspath(__file__)) + os.path.sep + "geomPipeline.py"
    #print("geomPipeline.py {} {} --config".format(action, inf))
    if not os.path.exists(this_geomPipeline):
        this_geomPipeline = "geomPipeline.py"
    os.system("{} {} {} {} --config".format(this_geomPipeline, action, inf, args))

    jf = open(default_config_file_name, "r", encoding="utf-8")
    config_file_content = json.load(jf)
    config_file_content["readers"][0]["dataFileName"] = inf
    config_file_content["writers"][0]["dataFileName"] = outf
    config_file_content["dataStorage"]["workingDir"] = gettempdir()
    config_file_content["logger"]["verbosity"] = "WARNING"

    jf.close()
    config_file_name = default_config_file_name
    with open(config_file_name, "w", encoding="utf-8") as jf:
        json.dump(config_file_content, jf, ensure_ascii=False, indent=4)
    return config_file_name


class GeomTestBase(unittest.TestCase):
    """
    For imprint tests, derived classes only need to implement 2 methods: 
    `build_geometry(freecadDoc)` and `validate_geometry(partObject)`
    For other tests, `setup` may need to be rewritten to generate config.json
    """

    # there is error to user constructor for unit test, not new style object
    # def __init__(self, methodName="runTest"):
    # super(GeomTestBase, self).__init__(methodName)
    # unittest.TestCase.__init__(methodName)
    # self.geometry_file = default_geometry_filename
    # pass

    def build_geometry(self, doc):
        pass

    def validate_geometry(self, partObject):
        pass

    def generate_test_filename(self):
        return gettempdir() + os.path.sep + self.__class__.__name__ + ".stp"

    def get_processed_folder(self):
        return gettempdir() + os.path.sep + self.__class__.__name__ + "_processed"

    def build(self):
        outfile = self.generate_test_filename()
        document_name = self.__class__.__name__
        if os.path.exists(outfile):
            os.remove(outfile)
        App.newDocument(document_name)
        doc = App.getDocument(document_name)

        __objs__ = self.build_geometry(doc)
        if __objs__:
            Part.export(__objs__, outfile)  # it build a compound shape
            del __objs__

        App.closeDocument(document_name)
        return outfile

    def setup_config(self, action ="imprint", args = "", input_file=None, result_file=None):
        # setup up input and output files and generate pipeline config.json
        if input_file:
            self.geometry_file = input_file
        else:
            self.geometry_file = self.build()
        input_stem = ".".join(self.geometry_file.split(".")[:-1])
        self.output_geometry_file = input_stem + "_processed.brep"

        self.config_file = generate_config(self.geometry_file, self.output_geometry_file, action, args=args)

        if result_file:
            self.result_file = result_file

    def setup(self):
        self.setup_config()

    def validate(self):
        # this method should be override if result is not single DocumentObject

        ppp_start_pipeline(self.config_file)
        assert os.path.exists(self.geometry_file)
        default_document_name = self.__class__.__name__ + "_processed"
        App.newDocument(default_document_name)
        if hasattr(self, "result_file"):
            Part.insert(self.result_file, default_document_name)
        else:
            Part.insert(self.output_geometry_file, default_document_name)
        doc = App.getDocument(default_document_name)
        obj = doc.Objects[0]  # get the first and only obj for brep
        if hasattr(obj, "Shape"):
            shape = obj.Shape
            self.validate_geometry(shape)
        # tear down
        App.closeDocument(default_document_name)  # will this be done if failed?

    def test(self):
        # skip the test of this BaseClass
        if self.__class__.__name__ != "GeomTestBase":
            self.build()
            self.setup()
            self.validate()