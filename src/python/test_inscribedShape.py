#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# copyright  Qingfeng Xia


from __future__ import print_function, division
import sys
import os.path
import os


"""feature test  done by FreeCAD script, non-MPI console mode only, on smaller geometry
this script may only works on POSIX system, 
USAGE: python3 this_script.py
"""

import unittest
from GeomTestBase import GeomTestBase, App, Part

length = 10
sheet_ratio = 0.1

def makeInscribedBox(doc, hollow=True):

    smallBox = doc.addObject("Part::Box", "Box")
    smallBox.Length = length*(1.0-2*sheet_ratio)
    smallBox.Width = length*(1.0-2*sheet_ratio)
    if hollow:
        smallBox.Placement = App.Placement(
            App.Vector(length*sheet_ratio, length*sheet_ratio),
            App.Rotation(App.Vector(0, 0, 1), 0),
        )
    doc.recompute()
    return smallBox

def makeInscribedCylinder(doc, hollow=True):

    s = doc.addObject("Part::Cylinder", "Cylinder")
    if hollow:
        s.Radius = length*(0.5 - sheet_ratio)
        s.Height = length

        s.Placement = App.Placement(
            App.Vector(length/2, length/2),
            App.Rotation(App.Vector(0, 0, 1), 0),
        )
    else:
        s.Radius = length*(1-2*sheet_ratio)
        s.Height = length

        s.Placement = App.Placement(
            App.Vector(length*sheet_ratio, length*sheet_ratio),
            App.Rotation(App.Vector(0, 0, 1), 0),
        )
    doc.recompute()
    return s

def makeInscribedSphere(doc, hollow=True):
    sheet_ratio = 0.05
    s = doc.addObject("Part::Sphere","Sphere")
    if hollow:
        s.Radius = length*(0.5 - sheet_ratio)

        s.Placement = App.Placement(
            App.Vector(length/2, length/2, length*sheet_ratio*3),
            App.Rotation(App.Vector(0, 0, 1), 0),
        )
    else:
        s.Radius = length*(1-2*sheet_ratio)

        s.Placement = App.Placement(
            App.Vector(0, 0, 0),
            App.Rotation(App.Vector(0, 0, 1), 0),
        )
    doc.recompute()
    return s

#############################################################################
def makeHollowBoxes(doc):
    print("a geometry of box without internal cut out")
    distance = length
    N = 6

    tools = []
    makers = [makeInscribedBox, makeInscribedCylinder, makeInscribedSphere]
    for f in makers:
        tools.append(f(doc, True))
        tools.append(f(doc, False))

    cuts = []
    for i in range(N):
        cut= doc.addObject("Part::Cut","Cut")
        cut.Base = doc.addObject("Part::Box", "Box")
        cut.Tool = tools[i]
        cut.Placement = App.Placement(
            App.Vector(i * distance, 0, 0), 
            App.Rotation(App.Vector(0, 0, 1), 0)  # todo: rotate the result shape&inscribed shape
        )
        doc.recompute() 
        cuts.append(cut) 
    
    return cuts


##############################################################
class InscribedBoxesTest(GeomTestBase):
    def build_geometry(self, doc):
        return makeHollowBoxes(doc)

    def validate_geometry(self, shape):

        assert len(shape.Solids) >= 1
        assert shape.ShapeType == "CompSolid" or shape.ShapeType == "Compound"

    def setup(self):
        args = "--use-inscribed-shape"
        result_file = self.get_processed_folder() + os.path.sep + "inscribed_shapes.brep"
        self.setup_config(action ="imprint", args = args, result_file=result_file)


# ===============================================================
if __name__ == "__main__":  # no need this if used with pytest-3
    unittest.main(exit=False)

