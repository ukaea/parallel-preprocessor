

# Roadmap

## Platform supports and code quality
### windows build and package

Compiling on Windows has been done by installing dependencies OpenCASCADE by conda-forge or official OpenCASCADE installer.
Conda-forge package building seems has problem,  older C++ compiler version does not support C++17.

Currently only support utf8 `std::string` for non-ascii string and file path, will that cause problem on windows? seem not

### Text encoding
If non-ascii string will be used in terminal, it is assume the terminal uses UTF8 encoding which is true of unix-like OSs. 

> Paths in Windows (since XP) are stored natively in Unicode. The path class automatically does all necessary string conversions. It accepts arguments of both wide and narrow character arrays, and both std::string and std::wstring types formatted as UTF8 or UTF16.  source: https://docs.microsoft.com/en-us/cpp/standard-library/file-system-navigation?view=vs-2019

### MacOS build and package

build on github workflow CI is undergoing. Homebrew formula will not be provided 

### Static code analysis

High priority build into CI: 
+ code style check: clang-format
+ cppcheck
+ coverage report:  currently unit test coverage is about 60%, needs improvement

Low priority
+ memory security lint
+ threading safety check

### Unit test
#### Where should unit test data be installed to

Currently package does not include test data in the `data/` folder, because it is not decided where data should go. 


#### CTest and PyTest for test automation

#### Find large assembly in public domain to benchmark this software


## Enhance Geometry pipeline module

+ Unit: as a input parameter, currently default to mm, which is standard for CAD

### Support commercial CAD kernels
   + Geom::Shape base class, to replace OccUtils.h
   + refactor Geom module to replace TopoDS_Shape with Geom::Shape
   + create new submodule/subfolder 
   + more geometry readers, such as JT format open support
   + binary module/plugin system

### Geometry preprocessing operations:
Note: each task is worth of a journal paper, so it will not be available soon.
   - parallel enclosure, section operations (assuming geometry is valid, boolean operation will not fail)
   - geometry fixing (geometry data may not valid to perform boolean operation)

   - parallel geometry merging (currently imprint has been parallelized, but merging on single thread)
   - water-fill: for a given assembly, construct a shape for the volume that water can fill

## Planned features in the long term

### MPI distributive parallelism (concept planning stage)
   - decomposition/partitioning
   - data structure/class for large assembly decomposition
   - efficient algorithm for large assembly decomposition

### Part or boundary recognition for automated CAD-CAE simulation
   - part recognition (connection to machine learning)
   - interface and boundary identification 
   - solid and faces global identification
   - coordinate boundary items for the decomposed subassemblies
   - multi-physics solver input writing

### Parallel volume mesh module
   - parallel volume meshing (gmsh has parallel meshing feature)

### Geometrical defeaturing
   - parse some native CAD format to detect fillet, chamfer, etc
   - smaller edge, small face, fillet detection.

## Future integration with VTK and FreeCAD

VTK: paraview post processing
FreeCAD:   exchange TopoDS_Shape in python or just by files

   - API refactoring: should model after VTK pipeline
   - Soft architecture design refer to tensorflow


