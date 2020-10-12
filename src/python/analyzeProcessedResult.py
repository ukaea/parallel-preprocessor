#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os.path
import sys
import json
import glob
import scipy.io

"""
USAGE: this_script.py  path_to_processed_result_folder
"""

if len(sys.argv) > 1:
    case_folder = sys.argv[1]
    if not os.path.exists(case_folder):
        print("input argument: result folder does not exist!", case_folder)
        sys.exit(-1)
else:
    print("no input argument for the result folder, use the default")
    case_folder = "../build/ppptest/test/"
    case_folder = "/home/qxia/OneDrive/UKAEA_work/iter_clite_analysis/"
    case_folder = "/home/qxia/Documents/StepMultiphysics/parallel-preprocessor/result/mastu_processed/"

# in that case folder, ppp will generate those 2 files by GeometryPropertyBuilder
matched_files = glob.glob(case_folder + os.path.sep + "*metadata.json")
metadata_filename = None
if matched_files:
    metadata_filename = matched_files[0]


matrix_filename = case_folder + os.path.sep + "myFilteredMatrix.mm"
# "myCouplingMatrix.mm" is the final result exluding  NoCollision type
collisionInfo_filename = case_folder + os.path.sep + "myCollisionInfos.json"
clearance_threshold = 1


# fixing weak_interference has some log entry
log_filename = case_folder + os.path.sep + "debug_info.log"


def load_data(file_name):
    with open(file_name) as json_file:
        data = json.load(json_file)
        return data


def metadata_stat(file_name):
    if metadata_filename:
        nb_suppressed = 0
        d = load_data(file_name)
        for p in d:
            if p["suppressed"]:
                nb_suppressed += 1
        nb_record = len(d)
        print(f"{nb_suppressed} suppresed out of metadata record number {nb_record}")


total_op = 0


def collision_stat(collisionInfo_filename):

    mystat = {
        "NoCollision": 0,
        "Floating": 0,
        "FaceContact": 0,
        "Clearance": 0,
        "Interference": 0,
        "Enclosure": 0,
        "Error": 0,
        "Unknown": 0,
    }

    cinfo = load_data(collisionInfo_filename)
    N = len(cinfo)

    none_row = 0
    for row in cinfo:
        if row:
            collect_stat(mystat, row)
        else:  # if item has been suppressed before imprinting such as too small volume
            none_row += 1

    # maybe suppressed for some reason
    print("item suppressed before imprinting or floating", none_row)
    print_stat(mystat, N)


def collect_stat(mystat, row):
    global total_op
    for i, info in row.items():
        total_op += 1
        ctype = info["collisionType"]
        if ctype in mystat:
            if (
                ctype == "Clearance"
            ):  # actually big clearance is not saved during processing
                if info["value"] > clearance_threshold:
                    mystat["NoCollision"] += 1
            else:
                mystat[ctype] += 1
        else:
            print("not stat type: ", info["collisionType"])


def print_stat(mystat, N):
    print("total items (solids) processed ", N)
    print("total pairs detected (boolean operations) ", total_op)
    print("sparse ratio = total pairs detected/ (N*N): ", total_op / N / N)

    for key in mystat:
        print(key, ": number", mystat[key], ", ratio", mystat[key] / total_op)


def mm_stat(matrix_filename):
    # collisionInfo.json is sufficient to analysis
    if os.path.exists(matrix_filename):
        mat = scipy.io.mmread(matrix_filename)
        total_operation = mat.nnz
        N = mat.shape[0]
        print("sparse ratio for boundbox check", total_operation / N / N)
    else:
        print(f"{matrix_filename} does not exist")


def log_stat(log_filename):
    # collisionInfo.json is sufficient to analysis
    weak_interference = 0
    if os.path.exists(log_filename):
        with open(log_filename, "r") as f:
            for line in f.readlines():
                if line.find("weak interference fixed as face contact") > 0:
                    weak_interference += 1
            print(f"{weak_interference} weak_interference has been fixed")
    else:
        print(f"{log_filename} does not exist")


###############################################
if __name__ == "__main__":
    metadata_stat(metadata_filename)
    collision_stat(collisionInfo_filename)
    log_stat(log_filename)
    mm_stat(matrix_filename)
