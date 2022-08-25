#!/usr/bin/env python3
# -*- coding: utf-8 -*-

###########################################################################
#   Copyright (c) 2020 Qingfeng Xia  <qingfeng.xia@ukaea.uk>              #
#                                                                         #
#   This file is part of the FreeCAD CAx development system.              #
#                                                                         #
#   This library is free software; you can redistribute it and/or         #
#   modify it under the terms of the GNU Library General Public           #
#   License as published by the Free Software Foundation; either          #
#   version 2 of the License, or (at your option) any later version.      #
#                                                                         #
#   This library  is distributed in the hope that it will be useful,      #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU Library General Public License for more details.                  #
#                                                                         #
#   You should have received a copy of the GNU Library General Public     #
#   License along with this library; see the file COPYING.LIB. If not,    #
#   write to the Free Software Foundation, Inc., 59 Temple Place,         #
#   Suite 330, Boston, MA  02111-1307, USA                                #
#                                                                         #
###########################################################################

"""
this python3 only script works for all platforms if freecad is on command line PATH
on windows, if freecad is not on PATH , nor uninstall record found in registry
  then PYTHONPATH must be set

usage:
```python
from detectFreeCAD import append_freecad_mod_path
append_freecad_mod_path()
try:
    import FreeCAD
except ImportError:
    print("freecad is not installed or detectable, exit from this script")
    sys.exit(0)

```
"""

import sys
import subprocess
from shutil import which
import platform
import os.path


def is_executable(name):
    """Check whether `name` is on PATH and marked as executable.
    for python3 only, but cross-platform"""

    # from whichcraft import which
    return which(name) is not None


def detect_lib_path(out, libname):
    """parse ldd output and extract the lib, POSIX only
    OSX Dynamic library naming:  lib<libname>.<soversion>.dylib
    """
    # print(type(out))
    output = out.decode("utf8").split("\n")
    for l in output:
        if l.find(libname) >= 0:
            # print(l)
            i_start = l.find("=> ") + 3
            i_end = l.find(" (") + 1
            lib_path = l[i_start:i_end]
            return lib_path
    print("dynamic lib file is not found, check the name (without suffix)")


def get_lib_dir(program, libname):
    program_full_path = which(program)
    # print(program_full_path)
    if is_executable("ldd"):
        process = subprocess.Popen(
            ["ldd", program_full_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        out, err = process.communicate()
        solib_path = detect_lib_path(out, libname)
        lib_path = os.path.dirname(solib_path)
        if os.path.exists(lib_path):
            return lib_path
        else:
            print("library file " + libname + " is found, but lib dir does not exist")
    else:
        print("ldd is not available, it is not posix OS")
        # macos: has no ldd, https://stackoverflow.com/questions/45464584/macosx-which-dynamic-libraries-linked-by-binary


def get_freecad_app_name():
    ""
    if is_executable("freecad"):  # debian
        return "freecad"
    elif is_executable("FreeCAD"):  # fedora
        return "FreeCAD"
    elif is_executable("freecad-daily"):
        return "freecad-daily"
    else:
        print("FreeCAD is not installed or set on PATH")
        return None


def get_freecad_lib_path():
    ""
    os_name = platform.system()
    fc_name = get_freecad_app_name()
    if os_name == "Linux":
        if fc_name:
            return get_lib_dir(fc_name, "libFreeCADApp")
        else:
            return None
    elif os_name == "Windows":
        return get_lib_path_on_windows(fc_name)
    elif os_name == "Darwin":
        raise NotImplementedError("MACOS is not supported yet")
    else:
        # assuming POSIX platform with ldd command
        return get_lib_dir(fc_name, "libFreeCADApp")


def get_lib_path_on_windows(fc_name_on_path):
    # windows installer has the options add freecad to PYTHONPATH
    # this can also been done manually afterward, settting env variable PYTHONPATH
    # the code below assuming freecad is on command line search path

    lib_path = ""
    if fc_name_on_path:
        fc_full_path = which(fc_name_on_path)
        if fc_full_path:
            lib_path = os.path.dirname(os.path.dirname(fc_full_path)) + os.path.sep + "lib"
    else:  # check windows default installation path and registry key, if installed by installer
        fc_installation_path = get_installaton_path_on_windows("FreeCAD")
        if fc_installation_path:
            lib_path = fc_installation_path + os.path.sep + "lib"

    if os.path.exists(lib_path + os.path.sep + "Part.pyd"):
        return lib_path


def search_default_installaton_path_on_windows(fc_name):
    # tested for FreeCAD 0.20, Python 3.9
    pdir = os.path.expandvars("%ProgramFiles%")
    for d in os.listdir(pdir):
        if d.startswith("FreeCAD"):
            fc_dir = pdir + os.path.sep + d
            fc_path = fc_dir + os.path.sep + "bin" + os.path.sep + "FreeCAD.exe"
            if os.path.isfile(fc_path):
                return fc_dir


def get_installaton_path_on_windows(fc_name):
    # tested for FreeCAD 0.18 installed from installer, windows 10

    fc_dir = search_default_installaton_path_on_windows(fc_name)
    if fc_dir:
        return fc_dir
    import itertools
    from winreg import (
        ConnectRegistry,
        HKEY_LOCAL_MACHINE,
        OpenKeyEx,
        QueryValueEx,
        CloseKey,
        KEY_READ,
        EnumKey,
    )

    try:
        root = ConnectRegistry(None, HKEY_LOCAL_MACHINE)
        reg_path = r"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
        akey = OpenKeyEx(root, reg_path, 0, KEY_READ)
        for i in itertools.count():
            try:
                subname = EnumKey(akey, i)
            except Exception:
                break
            if subname.lower().find(fc_name.lower()) > 0:
                subkey = OpenKeyEx(akey, subname, 0, KEY_READ)
                pathname, regtype = QueryValueEx(subkey, "InstallLocation")
                CloseKey(subkey)
                return os.path.expandvars(pathname)
        # close key and root
        CloseKey(akey)
        CloseKey(root)
        return None
    except OSError:
        return None


def get_install_prefix(cmod_path):
    # return /usr/  instead of /usr/lib
    return os.path.dirname(os.path.dirname(os.path.dirname(cmod_path)))


def append_freecad_mod_path():
    try:
        import FreeCAD  # PYTHONPATH may has been set, do nothing
    except ImportError:
        # in case PYTHONPATH is not set
        cmod_path = get_freecad_lib_path()  # c module path
        if cmod_path:
            sys.path.append(cmod_path)
            pymod_path = os.path.dirname(cmod_path) + os.path.sep + "Mod"
            if os.path.exists(pymod_path):
                # for each module, add to sys.path, or just Part module
                # print(pymod_path + os.path.sep + "Part")
                sys.path.append(pymod_path + os.path.sep + "Part")
            else:
                # freecad-daily has lib and Mod in different folders for python3 build on Ubuntu
                # this code segment needs more tests on different platform
                try:
                    import FreeCAD  # it should not fail, as c-extension module is found

                    apymod_path = FreeCAD.getHomePath() + os.path.sep + "Mod"
                    apymod_path = apymod_path.replace("-python3", "")
                    # print(apymod_path)  freecad-daily GUI can provide correct home, but not here
                    if os.path.exists(apymod_path + os.path.sep + "Part"):
                        sys.path.append(apymod_path + os.path.sep + "Part")
                    else:
                        # there is new module system, all packages are organized into freecad
                        print("pure python module such as Mod/Part is not detected")
                        print("user should use `sys.path.append()` or set PYTHONPATH")
                except ImportError:
                    pass


if __name__ == "__main__":
    print("FreeCAD library path detection result: ", get_freecad_lib_path())
