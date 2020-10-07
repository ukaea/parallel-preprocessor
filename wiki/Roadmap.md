

# Roadmap

## Platform supports and code quality
### windows build and package

Compiling on Windows has been done using Conda installed OpenCASCADE or official OpenCASCADE installation.
Conda

### MacOS build and package

build on github workflow CI is undergoing
Homebrew formula

### Static code analysis

+ cppcheck
+ coverage report:  currently unit test coverage is about 60%, needs improvement
+ memory lint
+ threading 

### Other issues
#### Where should unit test data be installed to

Currently package does not include test data in the `data/` folder, because it is not decided where data should go. 

#### currently only support utf8
Unicode support is another problem to be solved, especially on Windows

### add guidance for contribution

## New modules or feature

### Under design not yet completed:
   - parallel enclosure, section operations
   - geometry fixing
   - interface and boundary identification
   - parallel geometry merging 

### MPI distributive parallelism (concept planning stage)
   - decomposition/partitioning
   - data structure/class for large assembly decomposition
   - efficient algorithm for large assembly decomposition
   - solid and faces global identification
   - coordinate boundary items for the decomposed subassemblies
   - etc. 

### Support commercial CAD kernels
   + Geom::Shape base class, to replace OccUtils.h
   + refactor Geom module to replace TopoDS_Shape with Geom::Shape
   + create new submodule/subfolder 
   + more geometry readers, such as JT format open support
   + binary module/plugin system
   + windows platform support

### Planned features in the long term

   - API refactoring: should model after VTK pipeline
   - architecture design refer to tensorflow
   - parallel volume meshing (gmsh has parallel meshing feature)
   - multi-physics solver input writing
   - part recognition (connection to machine learning)
   - geometrical defeaturing

### Future integration with VTK and FreeCAD

VTK: paraview post processing
FreeCAD:   exchange TopoDS_Shape in python or just by files

