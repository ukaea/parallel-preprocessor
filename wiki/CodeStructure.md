
## Source Code Structure

This software has modular design to simplify dependency and avoid code duplication. For example, PPP module depends on C++ standard library and TBB/MPI only, all domain specific module depends on PPP and domain related third-party library. Module (C++ namespace) has the same name as the subfolder.

Here is description to each module with one-line description to reach header file.
Note: currently the link to module folder only work in markdown format hosted in git repository.

### [Base module](./md_Base_Readme.html)
This module provides the missing type system for C++, enabling creating class instance from name. 

This module is extracted and adapted from [FreeCAD open source project](https://www.freecadweb.org/), it is available as [an external component from github](https://github.com/qingfengxia/cppBase)
   - Type.h: type system for C++ 
   - BaseClass.h: root class for type system support

### [PPP module: Parallel processing infrastructure ](./index.html)
This module consists of base classes for data pipeline, and parallel processing.
It depends on only C++ standard library, TBB and some header only libraries.

#### Infrastructure for parallel processing
   - Executor.h: interface and single-thread executor
   - ThreadPoolExecutor.h: multi-threading executor
   - ParallelAccessor.h: Accessor interface for parallel coupling operation
   - SparseMatrix.h: lock free concurrent container to store adjacency matrix
   - AsynchronousDispatcher.h: lock free parallel accessor, based on adjacency matrix
   - Progressor.h: report percentage of progress to console

#### Base classes for data pipeline
   - DataObject.h: abstract data container passing through pipeline
   - PipelineController.h: coordinate and execute pipeline (processors in order)
   - DataStorage.h: save result to folder, database, etc
   - ProcessorResult.h: single processor result
   - ProcessorError.h: a root exception class derived from `std::exception`
   - Processor.h: json based processor meta data (readonly attribute, dependency)
   - ProcessorTemplate.h: to write a new processor without declare a new class
   - CouplingMatrixBuilder.h: calculate item pair coupling relationship for ParallelAccessor

   **Planned yet completed**
   - DistributiveExecutor.h: MPI support
   - CommandLineProcessor.h: run command line for each item in a process pool

#### Pipeline control
   - Parameter.h: mapping from json config input, to control pipeline and processor behavior
   - Context.h: singleton class for pipeline control (logging, data storage, configuration)
   - OperatorProxy.h: support local console and remote operator interaction with pipeline
   - AbstractOperator.h: abstract operator interface
   - ConsoleOperator: terminal operator with console textual input and output

#### Utilities
   - TypeDefs.h: alias, typedef and enum, etc 
   - Utilities.h: utility functions
   - Process.h: adapter of `boost:process` and ProcessPool
   - fs.h:  adaptor of std::filesystem and boost::filesystem
   - Logger.h: thread-safe logging using **loguru** and reporting in json
   - UniqueId.h: geometry hashing algorithm

### [PropertyContainer](md_PropertyContainer_Readme.html)
It is an independent reusable component, with test and Readme inside the module folder.
   - PropertyContainer.hpp: type-safe dynamic container, base class for DataObject 
   - Property.h: type-safe property based on `std::any`
In the future, FreeCAD's Property framework could be used instead
see FreeCAD Property <https://www.freecadweb.org/wiki/Property> `Enumeration, Quantity, Link, List`. 

### [Geom module: Geometry preprocessing] (md_Geom_Readme.html)

This is an example of full-fledged module for geometry preprocessing.

Concrete classes to build a geometry pipeline:
   - GeometryData.h: container for geometry objects, derived from `PPP::DataObject` class
   - GeometryReader.h: step, brep, FreeCAD file reading, and return GeometryData
   - GeometryWriter.h: geometry shared shape merging (after imprinting) and writing 
   - GeometryPipelineController.h: extending `PPP::PipelineController`
   - GeometryProcessor.h: derived from `PPP::Processor` parent for all geometry processors
   - ProcessorSample.h:  geometry processor skeleton, copy it when creating a new processor
   - Goem.cpp: function implementation that should not put in headers
   - GeometryMain.cpp: executable equals to `geomPipeline.py config.json`, for debugging purpose

Concrete geometry processors:
   - GeometryShapeChecker.h: geometry shape error check
   - BoundBoxBuilder.h: calculate bound box for shapes
   - GeometryPropertyBuilder.h: shape meta data extraction
   - GeometryImprinter.h: boolean fragment/imprinting for large assemblies
   - CollisionDetector.h: collision detection, interference check (digital mockup)
   - GeometrySearchBuilder.h: for action search, extract one shape by boundbox, ID, shape matching, etc

Some geometry processors under design and testing:
   - GeometryEnclosure.h: for action fix (currently not working)
   - GeometryFixer.h: for action fix (currently not working)
   - GeometryDecomposer.h: for action decompose (currently not working)
   - GeometryFaceter.h: for action tessellation (surface meshing)

Utility and types:
   - GeometryTypes,h: typedef, data classes,enum, json IO support, etc
   - OccUtils.h: utility functions to deal with brep file, such as dump problematic geometry
   - OpenCascadeAll.h: contains all OpenCASCADE headers used by this module

Operator interaction:
   - GeometryOperatorProxy.h: implements OperatorProxy for Geom module
   - GeometryGuiMain.cpp: start a pipeline with Qt 3D viewer (not fully tested)
   - GuiOperator.h: implements AbstractOperator interface with local 3D viewer
   - WebSocketOperator.h: implements AbstractOperator interface for remote operation by websocket

### [python module](../python)
   - high-level python interface written in [pybind11]()for ppp such as `geomPipeline.py`
   - utility scripts
   + python integration test
   + setup.py: python pypi package generator

### [test module](../test)
   - C++ unit tests for other modules, catch2
   - some demonstration executable written in C++

### [third-party](../third-party) libraries
Single header libraries are integrated directly into this PPP project, 

 - half.hpp: half precision floating point
 - json.hpp: as dump, configuration
 - mm.h: NIST matrix market read and write

The other are incorporated as git submodule, see also [Design.md](./Design.md). 
   - [loguru](https://github.com/Delgan/loguru): thread-safe logging for C++
   - [catch2](): C++ unit test framework
   - [SGeom](https://github.com/ukaea/SGeom): geometry module extracted from [Salome platform](https://www.salome-platform.org/)
   - [RTree](https://github.com/nushoin/RTree): spatial search algorithm

Some libraries not yet integrated
   + zlib:
   + XML:

===

### [scripts: utility](../scripts)
   + License checker from FreeCAD project
   + python tool to add license headers to all:    https://github.com/johann-petrak/licenseheaders

### [wiki: markdown document](../wiki)
Hand-crafted documentation for some specific topics, in addition to doxygen generated html documentation.

### [data: testing data](../data)
 Test data for each module should be organized into a specific subfolder.

### [cMake](../cMake)
Extra CMake config, mainly downloaded from open source project, see copyright/license in file headers.