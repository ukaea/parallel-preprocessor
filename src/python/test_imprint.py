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

#############################################################################
def makeBoxCubes(doc, N=3):
    print("a geometry of box with 3D stacking has been created")
    dim = 3
    length = 10
    nBoxes = N ** dim
    # Part.makeBox(length, length, length)
    boxes = [doc.addObject("Part::Box", "Box") for i in range(nBoxes)]
    for z in range(N):
        for y in range(N):
            for x in range(N):
                ind = N ** 2 * z + N * y + x
                boxes[ind].Placement = App.Placement(
                    App.Vector(x * length, y * length, z * length),
                    App.Rotation(App.Vector(0, 0, 1), 0),
                )
    doc.recompute()
    return boxes


def makeBoxLine(doc, N=6):
    print("a geometry of box in line has been created")
    length = 10
    nBoxes = N
    # Part.makeBox(length, length, length)
    boxes = [doc.addObject("Part::Box", "Box") for i in range(nBoxes)]
    for x in range(N):
        ind = x
        boxes[ind].Placement = App.Placement(
            App.Vector(x * length, 0, 0), App.Rotation(App.Vector(0, 0, 1), 0)
        )
    doc.recompute()
    return boxes


##############################################################
class StackedBoxesTest(GeomTestBase):
    def build_geometry(self, doc):
        return makeBoxCubes(doc, N=2)

    def validate_geometry(self, shape):
        N = 2  # cubes stack
        total_shared_faces = N * N * (N - 1) + N * (N - 1) * 2 * N
        nonshared_faces = N ** 3 * 6 - total_shared_faces
        print(
            "geometry info (type, solid count, face count, expected face count):",
            shape.ShapeType,
            len(shape.Solids),
            len(shape.Faces),
            nonshared_faces,
        )
        assert len(shape.Solids) == N ** 3
        # assert len(shape.Compounds) == 0
        assert shape.ShapeType == "CompSolid" or shape.ShapeType == "Compound"

        assert len(shape.Faces) == nonshared_faces
        # 2 cubes shares 1 face, total 11 faces


class AlignedBoxesTest(GeomTestBase):
    def build_geometry(self, doc):
        return makeBoxLine(doc, N=6)

    def validate_geometry(self, shape):
        print(
            "geometry info (type, solid count, face count):",
            shape.ShapeType,
            len(shape.Solids),
            len(shape.Faces),
        )
        N = len(shape.Solids)
        assert len(shape.Faces) == N * 6 - (N - 1)


class StackedCurvedTest(GeomTestBase):
    def build_geometry(self, doc):
        objs = [doc.addObject("Part::Box", "Box") for i in range(4)]

        # objs[0].ShapeColor = (0.67,0.00,1.00)
        # move the second and beyond to a place in contact
        objs[1].Placement = App.Placement(
            App.Vector(10, 0, 0), App.Rotation(App.Vector(0, 0, 1), 0)
        )
        objs[2].Placement = App.Placement(
            App.Vector(0, 10, 0), App.Rotation(App.Vector(0, 0, 1), 0)
        )
        objs[3].Placement = App.Placement(
            App.Vector(10, 10, 0), App.Rotation(App.Vector(0, 0, 1), 0)
        )
        doc.addObject("Part::Cylinder", "Cylinder")
        doc.Cylinder.Placement = App.Placement(
            App.Vector(10, 10, 0), App.Rotation(App.Vector(0, 0, 1), 0)
        )
        doc.recompute()
        ret = []
        for o in objs:
            cut = doc.addObject("Part::Cut", "Cut")
            cut.Base = o
            cut.Tool = doc.Cylinder
            ret.append(cut)
        doc.recompute()
        c2 = doc.addObject("Part::Cylinder", "Cylinder")
        c2 = App.ActiveDocument.ActiveObject
        tol = 1e-4
        c2.Placement = App.Placement(
            App.Vector(10 + tol, 10 + tol, 0), App.Rotation(App.Vector(0, 0, 1), 0)
        )
        # print(type(c2), type(doc.Cylinder), c2.Placement)
        ret.append(doc.Cylinder)  # doc.Cylinder is working
        return ret

    def validate_geometry(self, shape):
        print(
            "geometry info (type, solid count, face count):",
            shape.ShapeType,
            len(shape.Solids),
            len(shape.Faces),
        )
        N = len(shape.Solids)
        assert N == 5
        # 4 boxes has faces = (N-1) * 6 - 4, cylinder has 4 or 5 on side, plus 2
        assert len(shape.Faces) == (N - 1) * 6 - 4 + 6



# ===============================================================
if __name__ == "__main__":  # no need this if used with pytest-3
    unittest.main(exit=False)

