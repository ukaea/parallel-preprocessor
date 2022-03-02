

## Software design

### Architecture design

+ cross-platform: 
All selected components must be cross-platform. In this early development stage, only baseline platform is documented: 64 bit Ubuntu 18.04 with a c++17 compiler and python3  

+ modular and extensible: cover all aspects of CAE
  - parallel preprocessing infrastructure which can be used in scenarios other than CAE
  - CAD preprocessing, shape check, fixing, imprinting, etc 
  - parallel meshing 
  - solver setup.

+ scalable: from single node multiple-threading to MPI on super-computer

+ pipeline style workflow: VTK style
  tensorflow style graph topology may be supported in the future

+ python3 language binding: based on pybind11
  javascript binding is also under consideration

+ Industry standard compliant:  
  - standard CAD exchange format: STEP P214  (AP242 is not supported by OpenCASCADE yet)
  - JTOpen: there is no open source library available
  - FreeCAD/BREP

+ UTF8 encoding is assumed and stored in `std::string`
  Note: 1. this may work for macos and Linux of utf8 locale; 
        2. `filesystem::path::string()` not `filesystem::path::u8string()`

### Modular design

Folder structure corresponds to modular design, inspired by FreeCAD project.
see [CodeStructure.md](./CodeStructure.md) for description for each header file.

+ third-party/Base: type system and BaseClass for c++ (extracted from FreeCAD project)
+ PropertyContainer: heterogeneous container based on `std::any`, may be replaced with FreeCAD's in the future
+ PPP: framework infrastructure for parallel preprocessing, depends only on STL and some header-only third-party libraries.
+ Geom: geometry processing relies on OCCT, including 2D surface meshing (facetizing)
+ Mesh: 3D volume meshing (planned yet coded)
+ third-party: json, catch2, loguru, SGeom, etc
+ test: unit test
+ python: python wrapping and python script executables
+ scripts: utility script
+ data: testing data

Note: any namespace and module name of FreeCAD module should be avoided. E.g. PPP module's function can be mapped to `App` but `PPP` is used insrtead; `Base` module should be compatible with FreeCAD. This arrangement can help into the future to integrate with FreeCAD at source code level.

### Data driven pipeline design

This software has the data-driven design, users can just write a json configuration file to build up the pipeline and specify parameters to each processor. There is no need to program in C++, if existing processors are sufficient. Each action has a default pipeline, a series of geometry processors, see example in the generated `config.json`. 

json file is the input configuration format. Pipeline can be built from the json configuration, i.e. a list of class names. Each processor can be weaked by processor specific parameters and global parameters such tolerance.

`Parameter<T>` mappable to json repr.  Furthermore, enum types can be converted into json string, such as, ShapeType , which has string name  identical to FreeCAD ShapeType enum. 

### User interface design

- Console: basic command line with json as input configuration file
- Python: bindings for Python3 by `pybind11`
- Gui: Qt based, embedded into each module
- Web:  web interface for remote operator

Note: UI design is yet completed, some untested code are not merged into the repository yet

======================================================================


## Component design

### OCCT as the first CAD kernel

OCCT is the only available industry-strenth LGPL open source CAD kernel. 

OCCT official does not have a python binding, therefore, C++ is chosen as the programming language.

cmake OpenCascade detection
- FreeCAD provided
- OpenCASCADE official cmake config, provided by package "occt-misc" from Ubuntu PPA
`/usr/lib/cmake/opencascade/OpenCASCADEConfig.cmake`
each module has 
`/usr/lib/cmake/opencascade/OpenCASCADEApplicationFrameworkTargets.cmake`

Note: geometry exported form one CAD kernel then imported to another CAD kernel may cause some problems.

parallel-preprocessor, should work without X-windows, but some modules  (data-exchange, XCAF) of OpenCASCADE has dep on X-windows


### Most classes are header only

hpp for header only, h and cpp for other pairs (currently my header only is still called *.h, it will be split into h and cpp after API stabalised) . 


### Third-party libraries license

Only MIT and BSD open source libraries can be included into this LGPLv2 repository
+ magic_enum.hpp: MIT License
+ half.hpp: BSD
+ [mm_io.h and mm_io.cpp](https://math.nist.gov/MatrixMarket/mmio-c.html):  NIST as a US government institution, so these 2 files should be in public domain
+ pybind_json.hpp: BSD 3-Clause

software components that will be downloaded during compiling, e.g. git submodules
+ [SGeom](https://github.com/ukaea/SGeom): Geom module extracted from Salome project LGPLv2.1
+ [nhohmann json c++ lib](https://github.com/nlohmann/json):  MIT
+ [loguru logging for C++](https://github.com/emilk/loguru) public domain
+ [catch2: unit test](https://github.com/catchorg/Catch2/blob/master/LICENSE.txt): Boost Software License 1.0

software it links:
+ OpenCASCDE: LGPLv2.1
+ intel TBB:  MIT 
+ boost libraries: boost license, like MIT/BSD
+ QT:  LGPLv2.1

