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
#from geomPipeline import ppp_start_pipeline

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

############################################################

default_geometry_filename = (
    gettempdir()
    + os.path.sep
    + "test_geometry.stp"  # tempfile.tempdir is None on Ubuntu?
)  # todo: fix the tmp folder for windows

######################### utilities ############################


def generate_config(inf, outf, action="imprint", default_config_file_name="config.json"):
    # generate a configuration without running it

    this_geomPipeline = os.path.dirname(os.path.abspath(__file__)) + os.path.sep + "geomPipeline.py"
    #print("geomPipeline.py {} {} --config".format(action, inf))
    if not os.path.exists(this_geomPipeline):
        this_geomPipeline = "geomPipeline.py"
    os.system("{} {} {} --config".format(this_geomPipeline, action, inf))

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
    derived class need to implement 2 methods: 
    build_geometry(freecadDoc) and validate_geometry(partObject)
    """

    # there is error to user constructor for unit test, not new style object
    # def __init__(self, methodName="runTest"):
    # super(GeomTestBase, self).__init__(methodName)
    # unittest.TestCase.__init__(methodName)
    # self.geometry_file = default_geometry_filename
    # pass

    def build_geometry(self, doc):
        self.skip_test_base = True
        pass

    def validate_geometry(self, partObject):
        pass

    def generate_test_filename(self):
        return gettempdir() + os.path.sep + self.__class__.__name__ + ".stp"

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

    def _setUp(self):
        self.geometry_file = self.build()
        self.output_geometry_file = self.geometry_file.replace(
            ".stp", "_processed.brep"
        )
        config = generate_config(self.geometry_file, self.output_geometry_file)

        ppp_start_pipeline(config)

    def validate(self):
        self._setUp()
        # this method should be override if result is not single DocumentObject
        assert os.path.exists(self.geometry_file)
        default_document_name = self.__class__.__name__ + "_processed"
        App.newDocument(default_document_name)
        Part.insert(self.output_geometry_file, default_document_name)
        doc = App.getDocument(default_document_name)
        obj = doc.Objects[0]  # get the first and only obj for brep
        if hasattr(obj, "Shape"):
            shape = obj.Shape
            self.validate_geometry(shape)
        # tear down
        App.closeDocument(default_document_name)  # will this be done if failed?

    # def test(self):
    #    if hasattr(self, "skip_test_base") and (not self.skip_test_base):
    #        self.validate()
