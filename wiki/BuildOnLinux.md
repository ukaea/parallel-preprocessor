# Linux Compiling Guide

## Install Dependencies

Note **Ubuntu and Fedora docker images with all dependencies are ready for quick testing.**

### Install from the pre-built binary package 

In short, if FreeCAD has been installed, all the runtime dependencies such OpenCASCADE, TBB, would have been installed. 

For example, on Ubuntu 18.04
```bash
# assume FreeCAD ppa has been added to your repository list
sudo add-apt-repository ppa:freecad-maintainers/freecad-stable
sudo apt update
sudo install freecad
sudo apt-get install libtbb2 libocct-foundation-7.3  libocct-data-exchange-7.3  libocct-modeling-data-7.3 libocct-modeling-algorithms-7.3  libocct-ocaf-7.3
# download the deb
sudo dpkg -i parallel-preprocessor*.deb
```

#### packages needed to build or develop

note: **package name are debian based**, see installation section for RedHat based operation systems

+ C++17 compiler is also needed. 
+ essential build tool: cmake, git, etc
+ `libtbb2-dev`: thread pool
+ OpenCASCADE (`libocct*-dev` and `occt-misc`) can be installed from freecad-daily PPA
  ubuntu has opencascade with dbgsym, but its version is too outdated. https://launchpad.net/ubuntu/+source/opencascade
+ `python3-dev`   pybind11 is now a submodule

#### Optional dependencies:
+ `boost`: if C++17 compiler is not available, `boost::any` and `boost::filesystem`
+ `freecad` or `freecad-daily`: to view result and test, using the freecad-python3
+ `doxygen`: for in-source documentation generation
+ `zlib` and zipiso++: for zipped stream IO: used by FreeCAD project, simulate java.zipfile API

Future dependencies:
+ `openmpi, libscotch, libhypre`, can be installed with fenics from Fenics PPA
+ `Qt` for GUI 3D viewer

If you follow the official guide and you can build FreeCAD-daily 0.19 (python3 is strongly recommended), you should get the OpenCASCADE 7.3 dev from their PPA repo.

There are some third-party libraries already integrated into the source code tree, see more details on [software design wiki page](./wiki/Design.md)

---

### Build from source

Tested compiler: g++ 7.x, g++8.x, clang 6 (needs CMake 3.13+ )

To use clang toolchain, use the following cmake command option.
```bash
cmake -T llvm 
# llvm-10, to specify the linker app: lld-link
```

### Method 1: building on local Linux

####  Ubuntu 18.04 and 20.04

The dependency is similar as Ubuntu 18.04, except OpenCASCADE is not installed from FreeCAD PPA, but Ubuntu official repository (version 7.3.3). 

```bash
# this repo contains submodule:
# for user clone this repo, git should automatically download all submodules, 
# if not, or not sure, just run
git submodule update --init --recursive
# otherwise, you will be warmed by cmake to run this command

# install compulsory dependencies
sudo apt-get install git cmake doxygen ccache g++

# add freecad-stable or freecad-daily ppa (for ubuntu 18.04)
source /etc/lsb-release  # help to detect ubuntu version, 
# $(lsb_release -c -s)
if [ "$DISTRIB_CODENAME" == "bionic" ]; then
add-apt-repository ppa:freecad-maintainers/freecad-daily  &&  apt-get update
apt-get install -y freecad-daily-python3 
fi

if [ "$DISTRIB_CODENAME" == "focal" ]; then
echo "libocct 7.3 can be installed from official repo"
fi

sudo apt-get install libocct*-dev occt*
sudo apt-get install python3-dev libtbb-dev

# libocct some module depends on Xwindows system, although
sudo apt-get install libx11-dev libxmu-dev libxi-dev

############ optional ######################
sudo apt-get install libboost-dev

# Qt interface,  see the gitlab CI scrypt for the most updated 

#pybind11 has been added as a submodule, there is no need to install
#if you can prefer a system-wide install, uncomment the following line
# sudo apt-get install python3-pybind11 pybind11-dev

# read zipped data folder,  cmake has FindZLIB, version 1.5, optional 
#sudo apt-get install zlib-dev  libzipiso++-dev

```


####  Fedora29+ and Centos8

Tested out on fedora 30, but it should work for fedora 29+ ,see [dockerfile for fedora](./Dockerfile_fedora)

Fedora 31 and 32 have OpenCASCADE in repository, there is no needs to compile from source.

1. install dependencies
```bash
#this script is extracted from dockerfile


# you must update before any yum install or add a repo
#fedora30 has the default python as python3
# `copr-cli` is needed to to add copr repo
yum install copr-cli -y && yum update -y 

# g++ 9.2 for f30 high enough, there is no need to install boost-devel
yum install g++ cmake make git doxygen -y

# python3
yum install python3 python3-devel  -y 

# optionally, install version 2.x of zipios++,  https://github.com/Zipios/Zipios
# not in use for the moment
# yum install zipios zipios-devel -y

############## optional ####################
# Qt5 is optional for GUI operation
yum install qt5-devel qt5-qtwebsockets-devel qt5-qtwebsockets -y
```

2. Install OpenCASCADE 7.x

2.1 opencascade 7.4 from package repository
For fedora 30+ since Jan 2020, there are freecad (python3) and opencascade 7.4 in repository to install

```bash
# opencascade 7.4 package  for fedora 30+ is available  
yum install opencascade-draw, opencascade-foundation,  opencascade-modeling,  opencascade-ocaf \
    opencascade-visualization opencascade-devel opencascade-draw freecad -y
```

2.2 Download the opencascade source code and compile from source, if not in package repository

```bash
###### dependencies needed to build OpenCASCADE from source ##########
# for OpenCASCADE, openGL is needed
yum install tbb tbb-devel freetype freetype-devel freeimage freeimage-devel -y \
        && yum install glew-devel SDL2-devel SDL2_image-devel glm-devel -y
# install those below if draw module is enabled for building OpenCASCADE
yum install tk tcl tk-devel tcl-devel -y

# package name distinguish capital letter, while debian name is just  libxmu
yum install libXmu-devel libXi-devel openmpi-devel boost-devel -y
```

To get the latest source code from [OCCT official website](https://www.opencascade.com/), you need register (free of charge)

Note for UKAEA users: I put the source code on UKAEA one drive to share: user your webbrowser with UKAEA email login to open the link, otherwise login in office 365
<https://ukaeauk-my.sharepoint.com/:u:/g/personal/qingfeng_xia_ukaea_uk/Efs6Dhazcb1MtH44bCyYSqUBpJ2nzvbb3wgytEjYtLIhXA?e=uo32mf>

```bash
cd opencascade*
mkdir build
cd build
cmake ..
make -j$(nproc)
make install
# by default install to the prefix: /usr/local/
```

### Compile parallel-preprocessor

CMake build system is employed to simplify cross-platform development

```bash
#git clone https://github.com/UKAEA/parallel-preprocessor.git
git clone git@git.ccfe.ac.uk:scalable-multiphysics-framework/parallel-preprocessor.git
git submodule update --init --recursive

#git clone (outside docker is easier for this non-public repo, to avoid trouble in authentication)

cd parallel-preprocessor
#
mkdir build
cd build
cmake ..
make -j$(nproc)
```

If python3 is not detected by pybind11,  then 
`cmake -DPYTHON_EXECUTABLE:FILEPATH=$(which python3) ..`
Qt5 have not yet been configured to compile on fedora in docker


### Method2: use docker image 

This image is based on ubuntu 18.04 with OpenCASCADE and FreeCAD installed.
```bash
docker pull qingfengxia/freecad-daily-python3
docker pull qingfengxia/ppp-fedora

# cd to your working folder
# I can not git clone this internal repo inside image, so it is done outside docker VM
# git clone will NOT download the external submodule loguru, so run this command
git submodule update --init --recursive

# log into this image, using full host path for volume binding
docker run  -ti  -u root -w /home/user/parallel-preprocessor -v $(pwd)/parallel-preprocessor:/home/user/parallel-preprocessor qingfengxia/freecad-daily-python3:latest  /bin/bash

# to run docker image locally, you can mapping dir and xwindows
# https://github.com/mviereck/x11docker

```

Inside the docker VM, or if you have managed all dep locally, change dir into the repo folder
`mkdir build && cd build && cmake .. && make -j4`

It is a CMake project. `rm -rf build` if you want to rebuild after some change in CMakeLists.txt

## Installation
### Use without system-wide installation

Without installation, this software can be evaluated by running [geomPipeline.py path_to_geometry_file](./python/geomPipeline.py), after building from source.

After out of source build in the `parallel-preprocessor/build` folder by `cmake .. && make -j6`, change directory to `cd parallel-preprocessor/build/ppptest`, run `python3 geomPipeline.py  path-to-your-geometry.stp`. User can user absolute or relative path for the input geometry file.

Note: change directory to the folder containing geomPipeline.py is not necessary, if user has put full path of `parallel-preprocessor/build/bin/` folder into user path. For example, by editing PATH varialbe in `~/.bashrc` on Ubuntu, it will just work as installed program.

#### add `bin` in the `build folder` to the user path

Just append the `bin`folder to user path in `~/.bashrc`, that is all.  you should be able to imprint geometry by `geomPipeline.py  input_geometry_path`.

This is the recommended way in this development stage, then it is easier to pull and build the latest source. There is no need for super user privilege.

However, unit test application such as `pppGeomTests` must be run in the `build/ppptest` folder for the moment, since test data in `parallel-preprocessor/build/data` are referred using relative path. 


### Use after install the deb/rpm package
#### Generate deb/rpm package

After successfully build this software from source using cmake, `make package` will generate the platform binary package, deb or rpm (currently, only Ubuntu 18.04 and fedora 30+). see [./Packaging.md] for more details.

#### Install parallel processor package
Precompiled binary packages may be provided in the future.

Platform package rpm and deb has been generated in the build directory, install it.
`sudo rpm -i parallel-preprocessor*`  on RedHat systems 
or `sudo dpkg -i parallel-preprocessor*` on debian/ubuntu

Note, super user privilege may be needed, depending on install prefix specified in cmake configuraton.

#### Use after installation
All python scripts and binary programs are installed to path, so there is no need to specify `geomPipeline.py` path. It can be run as a python3 script executable
`geomPipeline.py <your_geometry_file_path>`

### Install python conda package

A conda package can be generated to support more Linux platforms. 

However, it is not completed and tested yet since there is built C-extension module involved. User should install all the necessary dependency, such as OpenCASCADE 7.x, TBB, FreeCAD.



