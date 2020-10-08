
![.github/workflows/github-ci.yml](https://github.com/ukaea/parallel-preprocessor/workflows/.github/workflows/github-ci.yml/badge.svg)

**Parallel-preprocessor: a prototype of parallel CAE geometry preprocessing framework**

by Qingfeng Xia  
Research Software Engineering Group of UKAEA  
License: LGPL v2.1  
Copyright UKAEA, 2019~2020  


Dr Andrew Davis of UKAEA, has contributed his technical insight,  test geometries and other support to this software project. Also thank to my colleagues Dr John Nonweiler, Dr Jonathon Shimwell, etc.  for testing and reviewing this software.


**doxygen generated API documentation** To be added later

## Overview

Although this software aims to be a framework for more CAE/CAE preprocessing operatons, currently, this software provides only multi-threading geometry imprint and collision check, via command line user interface. This software has demonstrated faster and more controllable geometry imprinting on large geometry assemblies (10k+ parts) that is not possible on most existing CAD tools.

Technical backgroud will be added later

## Disclaimer

This is **NOT** a production quality software, but a prototype to demonstrate the potential of accelereating CAE preprocessing by massive-parallel on HPC. We would like to apply more good practice in research software engineering, once resource is available.

According to the open source [license](./LICENSE),  **there is no warranty for this free library**

## Platforms supported

This project has been designed to be cross-platform, but only Linux is supported as the baseline platform.

+ Ubuntu as the primary/baseline development platform, with deb binary package generated 

+ Fedora, Centos can compile ppp from source, with OpenCascade 7.x source code or copr package.

+ Centos8 should work without much effort; Centos7 software stack is outdated, using docker/singularity instead.

+ Windows 10 users are encouraged to use WSL with one of the supported Linux distributions, while guide to compile on Windows has been added.

+ MacOS should be possible through homebrew, and compiling instruction has been provided.

Conda package and Linux native package for Ubuntu LTS may be available in the future, see [packaging.md](wiki/Packaging.md)

## Installation guide

### Download binary package
Ubuntu deb package and fedora 30+ rpm package, conda packages for windows, it should be available to download on **github Release** for this public github. 
**Be careful of python version it targets is system-wide python3**

 [Download parallel-preprocessor for ubuntu version 18.04](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3_ubuntu-18.04.deb)
 [Download parallel-preprocessor for ubuntu version 20.04](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3-dev_ubuntu-20.04.deb)

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

## Future plan

[wiki/Roadmap.md](wiki/Roadmap.md): lists short-term and long-term plan, depends on funding status. Partially sponsoring this project is welcomed to enhance existing modules or develop new modules.


---

## License consideration
License consideration: LGPL v2.1 instead of LGPL v3, because all relevant projects, OCCT, salome, FreeCAD, are using LGPL v2.1. LGPL v3 and LGPL v2.1 is not fully compatible to mixed up in source code level.

## Contribution

Please submit issue and fix pull request from this repository.

Note:  the main branch of this repo is `main` not `master`. 

## Acknowledgement

Funding source: STEP project in UKAEA <http://www.ccfe.ac.uk/step.aspx>
