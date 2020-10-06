## Get started

### Quick start
The command line `geomPipeline.py your_geometry_file_path` is the starting point for users, just replace *your_geometry_file_path* by your geometry. 

NOTE:  if installed using deb/rpm on ubuntu and fedora, while user has anaconda activated, then user should give the full path of system python3 path, as the Linux package of ppp link  to system python `/usr/bin/python3`, and install ppp module to system python site. For example, on Ubuntu the ppp module `ppp.cpython-36m-x86_64-linux-gnu.so` is installed to `/usr/lib/python3/dist-packages/`

To use `geomPipeline.py`

`/usr/bin/python3 /user/bin/geomPipeline.py manifest.json`

Todo: build pp as a conda package for all platforms.

### Pipeline configuration

Users can use geomPipeline.py for geometry imprinting and merging. This python script will generate a json configuration file (by default `config.json` in the current folder) then starts the geometry preprocessing pipeline. Users can edit parameters in the generated `config.json` and re-run the pipeline by `python3 geomPipeline.py config.json`.

### Geometry preprocessing features

The default action is **imprint**, outputting a geometry file (`*._processed.brep`) of one compound with duplicated shared faces removed, and also a `*_processed_metadata.json` file of meta data.  The latter file contains meta data such as material information for the geometry brep file. All the output files are located in a folder in the current directory.

Imprint and merge can be split into 2 steps:
`geomPipeline.py imprint geometry_file --thread-count 6  --no-merge`
`geomPipeline.py merge  /home/qxia/Documents/StepMultiphysics/parallel-preprocessor/result --thread-count 6`

Other actions are `search, check, detect, decompose`, etc, see more options by running `geomPipeline.py -h`. 

`geomPipeline.py check geometry_file` will check for errors, e.g. volume too small, invalid geometry, etc
`geomPipeline.py detect geometry_file` will detect collision between solid shapes, see more shape relations types at Geom::CollisionType

### Input geometry format supported

+ FreeCAD native format, *.FCStd
+ Step geometry exchange format, *.step/*.step
+ OpenCASCADE native format, *.brep/*.brp, may combined with `*.metadata.json` (parallel-processor output meta data format)
+ a json manifest file: a list of dict (geometry file path + material name)

### Use native programs (compiled from C++ code)

These programs only accept a json configuration file, e.g. `geomPipeline path_to_json_config.json`. 
This c++ program is mainly used to debug C++ code. In fact, high-level python pipeline controller such as `geomPipeline.py` is just generate input configuration, all the processing computation is written in C++.