"""
this enable test for the cases: test after package installed or test in the build folder
using ppp module is importable, otherwise use the executable: geomPipeline
"""

import shutil
import sys
import os.path

ppp_geom_executable = "geomPipeline"

try:
    import ppp  # in case this python module has been installed
except ImportError:
    # without installation,  python can load module from ` build_folder/lib`
    this_file_folder = os.path.dirname(os.path.abspath(__file__))
    built_module_dir = os.path.dirname(this_file_folder) + os.path.sep + "lib"
    if os.path.exists(os.path.abspath(built_module_dir)):
        sys.path.append(os.path.abspath(built_module_dir))
    else:
        built_module_dir = os.path.dirname(this_file_folder) + os.path.sep + "build/lib"
        if os.path.exists(os.path.abspath(built_module_dir)):
            sys.path.append(os.path.abspath(built_module_dir))

    try:
        import ppp  # in case of running this module in the build folder
    except ImportError:
        print("parallel-preprocessor python module `ppp` is not installed/importable")
        # print("if not installed, ppp module must be located in build folder")
        # print("../lib/ related to this script")
        print("start to run `geomPipeline config.json`in an external process")
        ppp = None

has_ppp_module = bool(ppp)
if not has_ppp_module:
    if not shutil.which(ppp_geom_executable):
        this_file_folder = os.path.dirname(os.path.abspath(__file__))
        ppp_geom_executable = os.path.join(this_file_folder, "bin", ppp_geom_executable)


def ppp_start_pipeline(config_file_name):
    print(config_file_name)
    if ppp:
        p = ppp.GeometryPipeline(config_file_name)

        # import pprint
        # pprint.pprint(p.config())  # correct
        p.process()
    else:
        import subprocess
        result = subprocess.check_output([ppp_executable, config_file_name])
        print(result)


if __name__ == "__main__":
    ppp_start_pipeline(sys.argv[1])