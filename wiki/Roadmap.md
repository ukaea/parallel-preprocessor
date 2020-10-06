

# Roadmap

## Platform supports
### windows build and package

done, needs more test

### currently only support utf8
Unicode support is another problem to be solved.

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

### planned features in the long term

   - API refactoring: should model after VTK pipeline
   - architecture design refer to tensorflow
   - parallel volume meshing (gmsh has parallel meshing feature)
   - multi-physics solver input writing
   - part recognition (connection to machine learning)
   - geometrical defeaturing

### future integration with VTK or FreeCAD

VTK: paraview post processing
FreeCAD:   exchange TopoDS_Shape in python or just by files

