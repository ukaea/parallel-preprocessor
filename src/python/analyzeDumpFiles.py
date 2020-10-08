#!/usr/bin/python3
# -*- coding: utf-8 -*-

# copyright  Qingfeng Xia
# UKAEA internal usage only

from __future__ import print_function, division
import sys
import os.path
import math
import json
import glob

"""
analyze dumpped problematic geometry files
USAGE: python3 this_script.py case_folder_path
"""

verbosity = False
volume_reltol = 0.05
boundbox_reltol = 0.02

if len(sys.argv) > 1:
    if os.path.exists(sys.argv[1]):
        case_folder = sys.argv[1]
else:  # testing mode, manually set case_folder
    case_folder = "../build/ppptest/mastu_spaceclaim/"
    case_folder = "../result/mastu_processed_nonsuppressed/"
    # case_folder = "/home/qxia/OneDrive/UKAEA_work/iter_clite_analysis/"


from detectFreeCAD import append_freecad_mod_path

append_freecad_mod_path()

try:
    import FreeCAD
except ImportError:
    print("freecad is not installed or detectable, exit from this script")
    sys.exit(0)

# version check
import FreeCAD as App
import Part


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


def get_boundbox(filename):
    _, fname = os.path.split(filename)
    document_name = fname.split(".")[0]
    App.newDocument(document_name)
    Part.insert(filename, document_name)
    # doc = App.getDocument(document_name)
    obj = App.ActiveDocument.Objects[0]  # get the first and only obj for brep
    # print(obj.Label)
    if hasattr(obj, "Shape"):
        bbox = obj.Shape.BoundBox
    App.closeDocument(App.ActiveDocument.Name)
    return bbox


def load_data(file_name):
    with open(file_name) as json_file:
        data = json.load(json_file)
        return data


metadata_filename = case_folder + os.path.sep + "shape_properties.json"
cinfo_filename = case_folder + os.path.sep + "myCollisionInfos.json"
cinfo = load_data(cinfo_filename)
# metadata = load_data(metadata_filename)
dump_file_format = "*.brep"



def float_equal(a, b, reltol = volume_reltol):
    maxv= max(math.fabs(a), math.fabs(b))
    return math.fabs(a - b) < reltol * maxv

def check_similarity(b1, b2, reltol = boundbox_reltol):
    # for boundbox
    a = [b1.XMin,  b1.YMin, b1.ZMin, b1.XMax, b1.YMax, b1.ZMax]
    b = [b2.XMin,  b2.YMin, b2.ZMin, b2.XMax, b2.YMax, b2.ZMax]
    s = []
    for i,v in enumerate(a):
        s.append(float_equal(v, b[i]))
    if not all(s):
        print("bound not similar: ", a, b)

def extract_id(fname):
    parts = fname.split("_")
    return [int(p) for p in parts if p.isnumeric()]

def get_collision_info(id):
    return cinfo[id]

##############################################

def checkBoundboxError():
    # boundbox change check
    pattern = "bndboxOriginal"
    flist = glob.glob(case_folder + "dump_" + pattern + dump_file_format)

    for f in flist:
        print(f)
        f1 = f.replace("Original", "Changed")
        bbox = get_boundbox(f)
        bbox1 = get_boundbox(f1)
        s = check_similarity(bbox, bbox1)

##############################################


def checkTessellationError():
    #
    pattern = "tessellationError"
    flist = glob.glob(case_folder + "dump_" + pattern + dump_file_format)

    for f in flist:
        print(f)
        s = load_shape(f)
        # not completed


def check_all_shapes(flist):
    document_name = "all_shapes"
    App.newDocument(document_name)
    for f in flist:
        Part.insert(f, document_name)
    # doc = App.getDocument(document_name)
    objs = App.ActiveDocument.Objects

    solids = []
    origVols = []
    origBboxes = []

    for obj in objs:
        bb = obj.Shape.BoundBox
        v = obj.Shape.Volume
        if verbosity:
            print(bb)
            print(v)
        origVols.append(v)
        origBboxes .append(bb)
        solids.append(obj.Shape)

    tol = 0.0
    pieces, map = solids[0].generalFuse(solids[1:], tol)
    result_fragments = pieces.Solids

    for i, solid in enumerate(result_fragments):
        if not float_equal(origVols[i], solid.Volume):
            print("volume_change as", origVols[i], solid.Volume)
        check_similarity(origBboxes[i], solid.BoundBox)

    App.ActiveDocument.closeDocument()


def checkBOPError(pattern = "BOPCheckFailed"):
    #
    flist = glob.glob(case_folder + "dump_" + pattern + dump_file_format)

    #if len(flist) >= 2:
    #    print("===")
    #   check_all_shapes(flist)
    #   print("===")

    # FreeCAD TopoDS_Shape has check(True) to perform BOP check, but throw ValueError
    # s.fixTolerance() can enforce tolerance, which can fix some error during BOPcheck
    for f in flist:
        print(f)
        s = load_shape(f)
        if s == None:
            print("return Shape is None")
            break
    # only test the last ome
    fix_shape(s)

def fix_shape(s):
        tol = 0.001
        # selection of these
        precision, mintol, maxtol = 0.01, tol * 0.1, tol * 50
        print("shape max tolerance = ", s.getTolerance(1))
        print("shape avg tolerance = ", s.getTolerance(0))
        try:
            ss = s.copy()
            #ss.fixTolerance(tol)  # just simply enforce tol, does not help
            #print("after fixTolerance(), shape max tolerance = ", ss.getTolerance(1))
            fixed = ss.fix(precision, mintol, maxtol)
            print("after fix(), shape max tolerance = ", ss.getTolerance(1))
            print("after fix(), shape avg tolerance = ", ss.getTolerance(0))
            #ss.exportBrep(f.replace(dump_file_format, "_fixed" + dump_file_format))
            ss.check(True)
            if fixed:
                print("shape fixed")
            else:
                print("shape NOT fixed")
        except Exception as e:
            print(e)

def checkChangeVolumeError():
    # sys.path.append("/usr/share/freecad-daily/Mod/Part/")
    import BOPTools.SplitFeatures
    import CompoundTools.Explode

    pattern = "ctypeErrorOriginal"
    pattern = "relatedShape"
    flist = glob.glob(case_folder + "dump_" + pattern + dump_file_format)

    for f in flist:
        print(f)
        document_name = "Unnamed"
        App.newDocument(document_name)
        Part.insert(f, document_name)
        # doc = App.getDocument(document_name)
        obj = App.ActiveDocument.Objects[0]
        s = obj.Shape
        solids = []
        origVols = []
        origBboxes = []
        if verbosity:
            print("before fragments")
        for solid in s.Solids:
            if verbosity:
                print(solid.BoundBox)
                print(solid.Volume)
            origVols.append(solid.Volume)
            origBboxes .append(solid.BoundBox)
            solids.append(solid)
        # bool fragments and print the result

        if App.GuiUp:
            g = CompoundTools.Explode.explodeCompound(obj)  # return a group obj
            j = BOPTools.SplitFeatures.makeBooleanFragments(name="BooleanFragments")
            j.Objects = g
            j.Mode = "Standard"  # CompSolid  Split
            # j.Tolerance = 0.0;
            j.Proxy.execute(j)
            j.purgeTouched()
            input_obj = (
                App.ActiveDocument.BooleanFragments
            )  # compound  type by standard Mode
            result_fragments = input_obj.Shape.Solids
        else:
            tol = 0.0
            pieces, map = solids[0].generalFuse(solids[1:], tol)
            result_fragments = pieces.Solids
        # CompoundTools.Explode.explodeCompound(input_obj)
        assert(len(solids) == len(result_fragments))

        if verbosity:
            print("after fragments")
        for i, solid in enumerate(result_fragments):
            vol = solid.Volume
            if verbosity:
                print(solid.BoundBox)
                print(vol)
            if not float_equal(origVols[i], vol):
                print("volume_change", origVols[i], vol)
            check_similarity(origBboxes[i], solid.BoundBox)

        App.closeDocument(document_name)


###############################################
if __name__ == "__main__":
    #checkBOPError()
    checkBoundboxError()
    checkChangeVolumeError()
    checkTessellationError()