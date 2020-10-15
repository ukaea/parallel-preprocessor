

![Ubuntu and MacOS CI](https://github.com/ukaea/parallel-preprocessor/workflows/parallel-preprocessor/badge.svg)
![Fedora CI](https://github.com/ukaea/parallel-preprocessor/workflows/parallel-preprocessor-docker/badge.svg)

**Parallel-preprocessor: a prototype of parallel CAE geometry preprocessing framework**

by Qingfeng Xia  
Research Software Engineering Group of UKAEA  
License: LGPL v2.1  
Copyright UKAEA, 2019~2020  


Dr Andrew Davis of UKAEA, has contributed his technical insight,  test geometries and other support to this software project. Also thank to my colleagues Dr John Nonweiler, Dr Jonathon Shimwell, etc.  for testing and reviewing this software.


[**doxygen generated API documentation with wiki pages**](https://ukaea.github.io/parallel-preprocessor/site/doxygen-docs.html)

## Feature overview

This software aims to be a framework for more CAE/CAE **preprocessing operatons** for large geometry assemblies upto 1 millions parts, such as such as fusion reactor, areoplane, areo-engine as a whole, using high performance computation  infrastructure like Exa-scale super-computer. see more [Technical Overview](./wiki/TechOverview.md) on why a parallel preprocessor is needed in the era of E-scale (10^18 FLops) computation.

Currently, this software provides only multi-threading geometry imprint and collision check, via command line user interface. This software has demonstrated faster and more controllable geometry **collision-detection, imprinting** on **large geometry assemblies (10k+ parts)** that is not possible on most existing CAD tools.

![CPU usage of parallel-preprocessor using 64 threads on a 32-core CPU](./wiki/assets/ppp_multithreading_cpu_usage.png)

Screenshot for the CPU usage of parallel-preprocessor using 64 threads on a 32-core CPU (Source: Dr Andy Davis)


## Future plan

[wiki/Roadmap.md](wiki/Roadmap.md): lists short-term and long-term plan, depends on funding status. Partially sponsoring this project is welcomed to enhance existing modules or develop new modules.

## Disclaimer

This is **NOT** a production quality software, but a prototype to demonstrate the potential of accelereating CAE preprocessing by massive-parallel on HPC. We would like to apply more good practice in research software engineering, once resource is available.

According to the open source [license](./LICENSE),  **there is no warranty for this free library**

## Platforms supported

This project has been designed to be cross-platform, but only Linux is supported as the baseline platform.

+ Ubuntu latest LTS as the primary/baseline development platform, with deb binary package generated 
    - Debian package should be achievable, since OpenCASCADE are available in official repository

+ Fedora 30+  with OpenCascade 7.x package available from official repository, with rpm binary package generated.

+ Compiling from source code for other Linux platforms is straight-forward,  driven by cmake and cpack, guidance provided. 
    - Centos8 should work without much effort, but OpenCASCADE must be compiled from source at first.
    - Centos7 software stack is outdated for compiler and cmake , using docker/singularity instead.

+ Windows 10 users are encouraged to use WSL with one of the supported Linux distributions, while guide to compile on Windows has been added.

+ MacOS compiling and packaging is done via homebrew, DragNDrop binary package is available.

Conda package and Linux native package for Ubuntu LTS may be available in the future, see [packaging.md](wiki/Packaging.md)

## Installation guide

**Note: user must install runtime dependencies (TBB, OpenCASCADE, etc, see Compile guide wiki pages for each platform) then install the downloaded binary package. Hint: if user have freecad installed, then all dependencies should have installed**

### Download (x86_64 architecture) binary packages
Ubuntu deb package and fedora 30+ rpm package, conda packages for windows, it should be available to download on **github Release** for this public github. The unstable package build from the latest code on the main branch can be downloaded here <https://github.com/ukaea/parallel-preprocessor/releases/tag/dev>

**Note: choose the correct operation system, and the package is targeting at system-wide python3**

The package file has the name pattern: `parallel-preprocessor-<this_software_version>-dev_<OS name>-<OS version>.<package_suffix>` If your OS is not supported, you need to compile it by yourself,  there is documentation for installation dependency and build for all major platforms.

`apt remove parallel-preprocessor`
`dpkg -i parallel-preprocessor*.deb`
 [Download parallel-preprocessor for ubuntu version 18.04](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3-dev_ubuntu-18.04.deb)

 [Download parallel-preprocessor for ubuntu version 20.04](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3-dev_ubuntu-20.04.deb)

[Download parallel-preprocessor for debian version 10](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3-dev_debian-10.deb)


 [Download parallel-preprocessor for fedora version 31](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3-dev_fedora-31.rpm)

 [Download parallel-preprocessor for fedora version 32](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3-dev_fedora-32.rpm)

 Coming soon: 
 [Download parallel-preprocessor for MacOS 10.15](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3-dev_macos-latest.dmg)

 Coming later:   Conda package for Windows 10.

### Compile from source
[wiki/BuildOnLinux.md](wiki/BuildOnLinux.md): Guide to install dependencies and compile on Linux (Centos, Fedora, Ubuntu), build instructions.

[wiki/BuildOnWindows.md](wiki/BuildOnWindows.md): Guide to install dependencies via conda, and compile from source on Windows.

[wiki/BuildOnMacOS.md](wiki/BuildOnMacOS.md): Guide to install dependencies via Homebrew, and compile from source on MacOS.

[wiki/CondaBuildGuide.md](wiki/CondaBuildGuide.md): Guide and notes to build binary conda packages for windows and Linux.

### Test the installation

[wiki/Testing.md](wiki/Testing.md): unit test and integration tests written in Python. User can validate the software building and installation by run some of the test cases.


## Get started (tutorial)

[wiki/GetStarted.md](wiki/GetStarted.md): quick started guide to use geometry processing pipeline

---

## Developer guide

### Software Architecture

[wiki/ArchitectureDesign.md](wiki/ArchitectureDesign.md): Software architecture design, component selection, parallel computation strategy 

### Code structure

[CodeStructure.md](wiki/CodeStructure.md): Module design and one-line description for each header files 

### Parallel computation design

[wiki/ParallelDesign.md](wiki/ParallelDesign.md)  strategies for CPU, GPU and MPI parallel. In short, CPU-MPI is the focus of this framework, there are plenty framework for GPU acceleration.

### Guide to module designer
[wiki/DesignNewModule.md](wiki/DesignNewModule.md): describes how to design a new module or extend the existing module  

### Guide to packaging
[wiki/Packaging.md](wiki/Packaging.md) deb/rpm package generation by CPack, distribution strategy.

---

## License consideration
License consideration: LGPL v2.1 instead of LGPL v3, because all relevant projects, OCCT, salome, FreeCAD, are using LGPL v2.1. LGPL v3 and LGPL v2.1 is not fully compatible to mixed up in source code level.

## Contribution

Please submit issue and fix pull request from this repository.

Note:  the main branch of this repo is `main` not `master`. 

## Acknowledgement

Funding from August 2019 ~ April 2020: STEP project in UKAEA <http://www.ccfe.ac.uk/step.aspx>

