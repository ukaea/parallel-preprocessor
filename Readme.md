

**Parallel-preprocessor: a prototype of parallel CAE geometry preprocessing framework**

by Qingfeng Xia  
Research Software Engineering Group of UKAEA  
License: LGPL v2.1  
Copyright UKAEA, 2019~2020  

![.github/workflows/github-ci.yml](https://github.com/ukaea/parallel-preprocessor/workflows/.github/workflows/github-ci.yml/badge.svg)

Dr Andrew Davis of UKAEA, has contributed his technical insight,  test geometries and other support to this software project. Also thank to my colleagues Dr John Nonweiler, Dr Jonathon Shimwell, etc.  for testing and reviewing this software.

## License
License consideration: LGPL v2.1 instead of LGPL v3, because all relevant projects, OCCT, salome, FreeCAD, are using LGPL v2.1. LGPL v3 and LGPL v2.1 is not fully compatible to mixed up in source code level.

**doxygen generated API documentation**
To be added later

## Overview

Although this software aims to be a framework for more CAE/CAE preprocessing operatons, currently, this software provides only multi-threading geometry imprint and collision check, via command line user interface. This software has demonstrated faster and more controllable geometry imprinting on large geometry assemblies (10k+ parts) that is not possible on most existing CAD tools.

Note: This is not a production quality software, but a prototype to demonstrate the potential of accelereating CAE preprocessing by massive-parallel on HPC. We would like to apply more good practice in research software engineering, once resource is available.

## Platforms supported

This project has been designed to be cross-platform, but only Linux is supported as the baseline platform.

+ Ubuntu as the primary/baseline development platform, with deb binary package generated 
 [Download parallel-preprocessor-0.3-dev_ubuntu-18.04.deb](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3_ubuntu-18.04.deb)
 [Download parallel-preprocessor-0.3-dev_ubuntu-20.04.deb](https://github.com/ukaea/parallel-preprocessor/releases/download/dev/parallel-preprocessor-0.3-dev_ubuntu-20.04.deb)
+ Fedora, Centos can compile ppp from source, with OpenCascade 7.x source code or copr package.

+ Centos8 should work without much effort; Centos7 software stack is outdated, using docker/singularity instead.

+ Windows 10 users are encouraged to use WSL with one of the supported Linux distributions, while guide to compile on Windows has been added.

+ MacOS should be possible through homebrew, but compiling instruction is not provided.

Conda package and Linux native package for Ubuntu LTS may be availalbe in the future, see details in the document <Packaging.md>

## Installaton guide

### Binary package download
Ubuntu deb package and fedora 30+ rpm package, conda packages for windows, it should be available to download on **github Release** for this public github. 
**Be careful of python version it targets:  system-wide python or conda python**

For UKAEA internal gitlab only:  Ubuntu deb package and fedora 30+ rpm package  can be downloaded from UKAEA gitlab. <https://git.ccfe.ac.uk/scalable-multiphysics-framework/parallel-preprocessor>
click the download icon next to "WebIDE" button, select download artifacts zipped files.  **It is built for Ubuntu 18.04 only with system python version 3.6m**

### Compile from source
CompileOnLinux.md: Guide to install dependencies and compile on Linux (Centos,Fedora, Ubuntu), build instructions, see [installation guide in HTML](./md_wiki_CompileOnLinux.html)
BuildOnWindows.md: Guide to install dependecies via conda, and compile from source on windows see [build on windows guide in HTML](./md_wiki_BuildOnWindows.html)
CondaBuildGuide.md: Guide and notes to build binary conda packages for windows and Linux, see [conda build guide in HTML](./md_wiki_CondaBuildGuide.html)

### Test the installation
Testing.md: unit test and integration tests written in Python, see [unit test guide in HTML](./md_wiki_Testing.html). User can validate the software building and installation by run some of the test cases.
["python-integration-test" for ubuntu 18.04 deb](https://git.ccfe.ac.uk/scalable-multiphysics-framework/parallel-preprocessor/-/jobs/artifacts/master/download?job=python-integration-test) 


## Get started (tutorial)

GetStarted.md: quick started guide, see [quick started guide in HTML](./md_wiki_GetStarted.html)

---

## Developer guide

### Software Architecture

ArchitectureDesign.md: Software architecture design, component selection, parallel computation strategy [Software Design html page](./md_wiki_ArchitectureDesign.html)

### Code structure

CodeStructure.md: Module design and one-line description for each header files can be found in [Code Structure html page](./md_wiki_CodeStructure.html)

### Parallel computation design

see strategies for CPU, GPU and MPI parallel. In short, CPU-MPI is the focus of this framework, there are plenty framework for GPU acceleration.
[Parallel Design html page](./md_wiki_ParallelDesign.html)

### Guide to module designer
ModuleDesign.md: describes how to design a new module or extend the existing module  see [ Module Developer Guide html page](md_wiki_UserInterfaceDesign.html)

### Guide to packaging
Packaging.md: deb/rpm package generation by CPack, distribution strategy, see [packaging guide in HTML](./md_wiki_Packaging.html)

---

## Future plan

<./wiki/Roadmap.md>: lists short-term and long-term plan, depends on funding status. Partially sponsoring this project is welcomed to enhance existing modules or develop new modules.
see [Roadmap html page](./md_wiki_Roadmap.html)

---


## Acknowledgement

Funding source: STEP project in UKAEA <http://www.ccfe.ac.uk/step.aspx>
