# -*- coding: utf-8 -*-
# copyright  Qingfeng Xia
# UKAEA internal usage only

from __future__ import print_function, division
import sys
import os.path
import json
import unittest

"""
geometry fixing test 
USAGE: python3 this_script.py
"""


from GeomTestBase import GeomTestBase, App, Part, generate_config, ppp_start_pipeline


def load_shape(filename):
    # this function does not work properly on multiple files
    _, fname = os.path.split(filename)
    document_name = fname.split(".")[0]
    App.newDocument(document_name)
    Part.insert(filename, document_name)
    # doc = App.getDocument(document_name)
    obj = App.ActiveDocument.Objects[0]  # get the first and only obj for brep
    print(obj.Label)
    s = None
    if hasattr(obj, "Shape"):
        s = obj.Shape
    App.closeDocument(App.ActiveDocument.Name)
    return s  # it is possible to return shape after document close


class ShapeFixerTest(GeomTestBase):
    # this unit test must load shape instead of making shape
    # def build_geometry(self, doc):

    def _setUp(self):
        # why relative path does not work!
        self.geometry_file = "/home/qxia/Documents/StepMultiphysics/parallel-preprocessor/data/test_geometry/test_shapefix.brep"
        pos = self.geometry_file.rfind(".")
        self.output_geometry_file = self.geometry_file[:pos] + "_processed.brep"
        config = generate_config(self.geometry_file, self.output_geometry_file, "fix")
        ppp_start_pipeline(config)

    def test(self):
        self._setUp()
        s = load_shape(self.output_geometry_file)
        ss = s.copy()
        try:
            ss.check(True)
            assert True
        except ValueError as e:
            print(e)
            assert False


if __name__ == "__main__":
    unittest.main(exit=False)
