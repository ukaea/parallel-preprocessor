"""
this script parse FreeCAD. FCstd file format without using FreeCAD lib
it does not use any third-party library, so any python3 will work
see ppp_FreeCAD_reader.md for documentation
todo: App::Part has material property
      App::Part placement property
"""

import sys
import os.path
import json
import struct
import zipfile
import shutil

# from lxml import etree
import xml.etree.ElementTree as ET
from collections import OrderedDict

if sys.version_info.major < 2:
    print("only python 3+ is supported")
    sys.exit()

print(sys.argv)
if len(sys.argv) == 1:
    print("Usage: this_script input_freeCAD_file output_meta_data_file(optional)")
    print("or: this_script input_freeCAD_file freecad_extract_to_folder")
    print("without any input and out files, just run the test with example data")
    # fc_filename = 'Document.xml'
    fc_filename = "../data/pppFreeCADpreprocessorTest.FCStd"
    fc_filename1 = "../data/part_in_group.FCStd"
    fc_filename2 = "../../ppp_validation_geomtry/mastu_100shapes.FCStd"
    meta_data_output_filename = fc_filename[:-6] + ".json"
elif len(sys.argv) == 2:
    fc_filename = sys.argv[1]
    if not os.path.exists(sys.argv[1]):
        raise "Input file does not exist, exit!"
    meta_data_output_filename = fc_filename[:-6] + ".json"
elif len(sys.argv) == 3:
    fc_filename = sys.argv[1]
    output = sys.argv[2]
    if output.find(".json") > 0:
        meta_data_output_filename = output
    else:  # folder to extract
        zipObj = zipfile.ZipFile(sys.argv[1], "r")
        # Extract all the contents of zip file in different directory
        zipObj.extractall(output)
        meta_data_output_filename = output + os.path.sep + "metadata.json"
        print("extract FreeCAD file into the folder: ", output)
else:
    print("Usage: this_script input_freeCAD_file output_meta_data_file(optional)")
    sys.exit()


# if sys.byteorder != "little":
#    raise Exception("this script assume little endianness for cpu write the input file")
def unpack_color_value(v):
    """ FreeCAD save the packed RGBA vector into one `unsigned long` value
    PropertyStandard.cpp
    see source code in the Color class of App/Material.cpp, it is fixed as little endianness
    Color& setPackedValue(uint32_t rgba)
    {
        this->set((rgba >> 24)/255.0f,
                ((rgba >> 16)&0xff)/255.0f,
                ((rgba >> 8)&0xff)/255.0f,
                (rgba&0xff)/255.0f);
        return *this;
    }
    # test in python
    i = int("0x08060402", 16)
    struct.unpack('BBBB', i.to_bytes(4, byteorder="big"))
    (8, 6, 4, 2)
    """
    rgba = struct.unpack("BBBB", int(v).to_bytes(4, byteorder="big"))
    return [i / 255 for i in rgba]


def process_color_property(vobjects):
    pass


class FreeCADParser(object):
    def __init__(self, fc_filename):
        # shared variables
        self.shapes = OrderedDict()  # Part::Feature
        # Groups or Parts, only one type of shapes grouping should be used
        self.parts = OrderedDict()  # subassembly support, `App::Part`
        # `App::Part` should not be used, group in another group not supported!
        self.groups = OrderedDict()  # material

        self.doc = self.read_document(fc_filename)
        self.filename = fc_filename
        # self.boundaries = OrderedDict()
        # return {"shapes": shapes, "parts": parts, "groups": groups}

    def read_document(self, fc_filename, gui=False):
        suffix = fc_filename.split(".")[-1]
        if suffix.lower() == "fcstd":

            archive = zipfile.ZipFile(fc_filename, "r")
            if gui:
                doc = archive.read("GuiDocument.xml")
            else:
                doc = archive.read("Document.xml")
            return doc
        else:
            doc = open(fc_filename, "rb").read()  # lxml must read binary?
            return doc

    def register_objects(self, objects):
        for o in objects.findall("Object"):
            if o.attrib["type"].find("App::Part") >= 0:
                self.parts[o.attrib["name"]] = {
                    "type": o.attrib["type"],
                    "id": o.attrib["id"],
                    "groups": [],
                }
            elif o.attrib["type"].find("Part") >= 0:  # Part:: or PartDesign::
                self.shapes[o.attrib["name"]] = {
                    "type": o.attrib["type"],
                    "id": o.attrib["id"],
                    "groups": [],
                }
            elif o.attrib["type"].find("App::DocumentObjectGroup") >= 0:
                self.groups[o.attrib["name"]] = {
                    "type": o.attrib["type"],
                    "id": o.attrib["id"],
                }
            else:
                # material and boundary condition
                pass
            # print(o.tag, o.attrib["name"])  #  how is `name` attribute related with object's Name Property ?

    def process_object_data(self, objectData):
        for o in objectData.findall("Object"):
            if o.attrib["name"] in self.shapes.keys():
                self.process_part_feature(o)
            if o.attrib["name"] in self.parts.keys():
                self.process_group(o, self.parts)
            elif o.attrib["name"] in self.groups.keys():
                self.process_group(o, self.groups)
            else:
                pass
            # print(o.attrib["name"])  # name is the unique key

    def process_part_feature(self, o):
        d = self.shapes[o.attrib["name"]]
        for p in o.find("Properties").findall("Property"):
            if p.attrib["name"] == "Label":  # the human readable name is actually label
                d["name"] = p.find("String").attrib["value"]
            elif p.attrib["name"] == "Visibility":
                d["visible"] = p.find("Bool").attrib["value"] == "true"
            elif p.attrib["name"] == "Shape":
                d["filename"] = p.find("Part").attrib["file"]
            elif p.attrib["name"] == "Placement":
                d["translation"] = ""  # Todo
            elif p.attrib["name"] == "AttachmentOffset":
                d["partTranslation"] = ""  # Todo
            # elif p.attrib['name'] == "Color":  # may be exist only in GuiDocuments
            #    d["color"] =
            else:
                pass

    def process_group(self, o, groups):
        d = groups[o.attrib["name"]]
        for p in o.find("Properties").findall("Property"):
            if p.attrib["name"] == "Label":
                d["label"] = p.find("String").attrib["value"]
            elif p.attrib["name"] == "Visibility":
                d["visible"] = p.find("Bool").attrib["value"] == "true"
            elif p.attrib["name"] == "Group":
                d["members"] = [
                    l.attrib["value"]
                    for l in p.find("LinkList").findall("Link")
                    # if not of GroupType
                ]
                # assert all([n in shapes.keys() for n in d["members"]])
            else:
                pass

    def parse_placement_property(self, pPlacement):
        """
        needed for shape in "App::Part" with modified placement
        C++
        python side
        """
        p = pPlacement.find("PropertyPlacement")
        position = [p.attrib["Px"], p.attrib["Py"], p.attrib["Pz"]]
        origin = [p.attrib["Ox"], p.attrib["Oy"], p.attrib["Oz"]]
        # rotation, a quaternion

    def link_shape_to_groups(self, groups):
        #
        for gname in groups.keys():  # can we change value in variable in outer scope?
            g = groups[gname]
            for name in g["members"]:  # link to object name
                if (
                    name in self.shapes.keys()
                ):  # not all obj in group are shapes, check it
                    self.shapes[name]["groups"].append(g["label"])

    def process_visual_object_data(self, vpobjects):
        for o in vpobjects:
            if o.attrib["name"] in self.shapes.keys():
                d = self.shapes[o.attrib["name"]]
                for p in o.find("Properties").findall("Property"):
                    if p.attrib["name"] == "ShapeColor":
                        d["color"] = unpack_color_value(
                            p.find("PropertyColor").attrib["value"]
                        )
                    # LineColor, PointColor, Visibility

    def process(self):

        root = ET.fromstring(self.doc)
        self.register_objects(root.find("Objects"))  # generate an OrderedDict of dict
        self.process_object_data(
            root.find("ObjectData")
        )  # further fill the dict of dict
        self.link_shape_to_groups(self.groups)  # reversely fill group info into shapes
        self.link_shape_to_groups(self.parts)  # assembly

        gui_root = ET.fromstring(self.read_document(self.filename, gui=True))
        self.process_visual_object_data(
            gui_root.find("ViewProviderData").findall("ViewProvider")
        )

        with open(meta_data_output_filename, "w", encoding="utf-8") as f:
            json.dump(list(self.shapes.values()), f, indent=4)
            print(
                "write parsed FreeCAD file meta data into the file: "
                + meta_data_output_filename
            )


if __name__ == "__main__":
    parser = FreeCADParser(fc_filename)
    parser.process()
    parser = FreeCADParser(fc_filename1)
    parser.process()
