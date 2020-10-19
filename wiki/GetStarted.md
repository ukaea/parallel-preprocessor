## Get Started

### Quick start
The command line `geomPipeline.py imprint your_geometry_file_path` is the starting point for users, just replace *your_geometry_file_path* by your geometry. 

run `geomPipeline.py -h` for more options. 

### Geometry preprocessing features

The default action is **imprint**, outputting a geometry file (`*._processed.brep`) of one shape with duplicated shared faces removed, and also a `*_processed_metadata.json` file of meta data.  The latter file contains meta data such as material information for the geometry brep file. All the output files are located in a folder in the current directory.

The imprint operation can be split into 2 steps:  Imprint faces and Merge faces:
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
  ```json
    [{"material": "Copper",  "filename": "path_to_geometry_file1" },
     {"material": "Steel",  "filename": "path_to_geometry_file2" },
    ]
  ```

### Advanced usage (adjust pipeline parameters)
####  Pipeline configuration generation (python script)

`geomPipeline.py` will generate a json configuration based on user input (by default `config.json` in the current folder),  then starts the geometry preprocessing pipeline. For example, the imprint action will be organized into a pipeline of several GeometryProcessors, with default parameters written into the `config.json`. If the output is not ideal, users can edit parameters in the generated `config.json` and re-run the pipeline by `python3 geomPipeline.py config.json`, or equally `geomPipeline path_to_json_config.json`.  

In fact, python pipeline controller such as `geomPipeline.py` generates input configuration, all the processing computation is done by `geomPipeline` which is an executable compiled from C++ code. This executable only accepts a json configuration file, e.g. `geomPipeline path_to_json_config.json`.  

The split of high-level user-oriented python script and lower-level C++ program has the benifits:
+ to ease the debugging of mixed python and C++ programming
+ to ease the parallel programming, since Python has the GIL problem


### Debug your installation

NOTE:  if installed using deb/rpm on ubuntu and fedora, while user has anaconda activated, then user should give the full path of system python3 path, as the Linux package of ppp link  to system python `/usr/bin/python3`, and install ppp module to system python site. For example, on Ubuntu the ppp module `ppp.cpython-36m-x86_64-linux-gnu.so` is installed to `/usr/lib/python3/dist-packages/`

To use `geomPipeline.py`

`/usr/bin/python3 /user/bin/geomPipeline.py manifest.json`