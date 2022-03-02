# Developer Guide

## Compile from source
[BuildOnLinux.md](BuildOnLinux.md): Guide to install dependencies and compile on Linux (Centos, Fedora, Ubuntu), build instructions.

[BuildOnWindows.md](BuildOnWindows.md): Guide to install dependencies via conda, and compile from source on Windows.

[BuildOnMacOS.md](BuildOnMacOS.md): Guide to install dependencies via Homebrew, and compile from source on MacOS.

[CondaBuildGuide.md](CondaBuildGuide.md): Guide and notes to build binary conda packages for windows and Linux.

### Guide to packaging
[Packaging.md](Packaging.md) deb/rpm package generation by CPack, distribution strategy.

### Guide to documentation 

[Documentation.md](Documentation.md): write and publish documentation

---

## Software Architecture

[ArchitectureDesign.md](ArchitectureDesign.md): Software architecture design, component selection, parallel computation strategy 

### Code structure

[CodeStructure.md](CodeStructure.md): Module design and one-line description for each header files 

### Parallel computation design

[ParallelDesign.md](ParallelDesign.md)  strategies for CPU, GPU and MPI parallel. In short, CPU-MPI is the focus of this framework, there are plenty framework for GPU acceleration.

### Guide to module designer
[DesignNewModule.md](DesignNewModule.md): describes how to design a new module or extend the existing module  

---

## Quality assurance
### Code style

- Qt style naming, function name begins with lower case letter, diff from OCCT, VTK
- private or protected class member is named as myVariable it is OCCT style
- clang-format for auto formatting, default LLVM
- spell check: command line can be made into git hook or vscode task
- code guideline/standard: https://github.com/qingfengxia/awesome-cpp#coding-style

### Static code analysis

- compiler: treating warning as error, setup in cmake
    `target_compile_options(MyLib PRIVATE -Werror -Wall -Wextra)`

static code analysis by cppcheck, clang-tidy, similarity check
- cppcheck --enable=all --std=c++11 --suppress=*:third-party/json.hpp src/


