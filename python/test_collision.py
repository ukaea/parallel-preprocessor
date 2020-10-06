# -*- coding: utf-8 -*-
# copyright  Qingfeng Xia
# UKAEA internal usage only

from __future__ import print_function, division
import sys
import os.path
import json
import unittest

"""feature test  done by FreeCAD script, non-MPI console mode only, on smaller geometry
USAGE: python3 this_script.py
"""


from GeomTestBase import GeomTestBase, App, Part

######################### moved to test_imprint.py ############################
def solid_interference_maker(doc):
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

    box = doc.addObject("Part::Box", "Box")
    box.Width = 9  # Y axis length, make the last box smaller,
    objs.append(box)

    doc.recompute()
    return objs


class EnclosureTest(GeomTestBase):
    def build_geometry(self, doc):
        objs = solid_interference_maker(doc)

        # this box is enclusured in another box without sharing faces
        objs[4].Height = 9
        objs[4].Length = 9
        objs[4].Placement = App.Placement(
            App.Vector(10.5, 10.5, 0.5), App.Rotation(App.Vector(0, 0, 1), 0)
        )
        doc.recompute()

        # this box is coincident with the box with default placement
        box = doc.addObject("Part::Box", "Box")
        box.Width = 10  # Y axis length, make the last box smaller,
        objs.append(box)

        doc.recompute()
        return objs

    def validate_geometry(self, shape):
        print("len(shape.Faces)", len(shape.Faces))
        print("len(shape.Solids) = ", len(shape.Solids))

        assert len(shape.Solids) == 4
        assert shape.ShapeType == "CompSolid" or shape.ShapeType == "Compound"
        # assert len(shape.Faces) == 20  # 4 cubes shares 4 faces, total 20 faces

    def test(self):
        self.validate()


class WeakInterferenceTest(GeomTestBase):
    # def __init__(self, test_name="weak_interference"):
    #    GeomTestBase.__init__(test_name)

    def build_geometry(self, doc):
        objs = solid_interference_maker(doc)

        # this box interfere with some other boxes
        # if it is weak interference, it can fixed autmaitcally
        # see parameter setting in GeometryImprinter
        objs[4].Placement = App.Placement(
            App.Vector(-9.9, -5, 0), App.Rotation(App.Vector(0, 0, 1), 0)
        )
        doc.recompute()
        return objs

    def validate_geometry(self, shape):
        print("len(shape.Faces)", len(shape.Faces))
        print("len(shape.Solids) = ", len(shape.Solids))

        assert len(shape.Solids) == 5
        assert shape.ShapeType == "CompSolid" or shape.ShapeType == "Compound"
        assert len(shape.Faces) == 30  # 4 cubes shares 4 faces, total 20 faces + 10
        # todo: face count needs to be further investigated

    def test(self):
        self.validate()


class StrongInterferenceTest(GeomTestBase):
    def build_geometry(self, doc):
        objs = solid_interference_maker(doc)
        # this box interfere with 2 other boxes, so it should be auto suppressed
        objs[4].Placement = App.Placement(
            App.Vector(-4.5, 5, 0), App.Rotation(App.Vector(0, 0, 1), 0)
        )
        doc.recompute()
        return objs

    def validate_geometry(self, shape, supressing=True):
        print("len(shape.Faces)", len(shape.Faces))
        print("len(shape.Solids) = ", len(shape.Solids))
        assert shape.ShapeType == "CompSolid" or shape.ShapeType == "Compound"
        # depends on implementation on Cpp side
        if supressing:
            assert len(shape.Solids) == 4
            assert len(shape.Faces) == 20  # 4 cubes shares 4 faces, total 20 faces
        else:
            assert len(shape.Solids) == 5
            assert len(shape.Faces) == 26  # output geometry == input

    def test(self):
        self.validate()


if __name__ == "__main__":
    unittest.main(exit=False)
