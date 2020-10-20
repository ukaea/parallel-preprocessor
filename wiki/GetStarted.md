# Get Started

## Command line interface

Following installation (see [wiki/Installation.md](wiki/Installation.md)), the starting point for users is the command line:

```bash
geomPipeline.py imprint your_geometry_file_path
```

(with *your_geometry_file_path* replaced by the path to your geometry.)

This starts the geometry pipeline. `geomPipeline.py` takes 2 positional arguments, the first is the action, and the second is the input file path.

Optional arguments for all pipelines:

```
  -h, --help            show this help message and exit
  -o OUTPUT_FILE, --output-file OUTPUT_FILE
                        output file name (without folder path)
  --working-dir WORKING_DIR
                        working folder path, by default the current working folder
  --output-dir OUTPUT_DIR
                        output folder path, by default, a subfolder in workingDir
  --config              only generate config.json without run the pipeline

  -nt THREAD_COUNT, --thread-count THREAD_COUNT
                        number of thread to use, max = hardware core number

  -v VERBOSITY, --verbosity VERBOSITY
                        verbosity: for console or terminal: DEBUG, PROGRESS, INFO, WARNING, ERROR
```

Optional arguments for the geometry pipeline:

```
  --metadata METADATA   input metadata file, only for brep input geometry
  --tolerance TOLERANCE
                        tolerance for imprinting, unit MilliMeter
  --no-merge            do not merge the imprinted shapes, for two-step workflow
  --ignore-failed       ignore failed (in BOP check, collision detect, etc) solids
```

Always run `geomPipeline.py -h` to get the latest arguments.  

## Geometry preprocessing features

Implemented geometry processing actions are: `check,detect,merge,imprint,search`, some other planned actions `tessellate, fix, decompose` can be found in [Roadmap.md](Roadmap.md)

The  **imprint** action outputs a geometry file (`*._processed.brep`) of one shape with duplicated shared faces removed, and also a `*_processed_metadata.json` file of meta data.  The latter file contains meta data such as material information for the geometry brep file. All the output files are located in a folder in the current directory.

The imprint operation is accomplished in 2 steps:  Imprint faces and Merge faces:
`geomPipeline.py imprint geometry_file --thread-count 6  --no-merge`
`geomPipeline.py merge  /home/qxia/Documents/StepMultiphysics/parallel-preprocessor/result --thread-count 6`

Usage of other actions such as `search, check, detect, decompose`

`geomPipeline.py check geometry_file` will check for errors, e.g. volume too small, invalid geometry, etc
`geomPipeline.py detect geometry_file` will detect collision between solid shapes, see more shape spatial relationship types defined in the type `Geom::CollisionType`

## Input geometry format supported

+ FreeCAD native format, *.FCStd
+ Step geometry exchange format, *.step
+ OpenCASCADE native format, \*.brep/\*.brp, may combined with `*.metadata.json` (PPP output meta data format)
+ a json manifest file: a list of dict (geometry file path + material name)

```json
    [{"material": "Copper",  "filename": "path_to_geometry_file1" },
     {"material": "Steel",  "filename": "path_to_geometry_file2" }]
```

  see doxygen generate document for this class for most updated information <>

## Debug your installation

`which geomPipeline` on Unix-like system, or `where geomPipeline` on Windows to see if executable has been installed on PATH. 

NOTE:  if installed using deb/rpm on ubuntu and fedora, while user has anaconda activated, then user will not be able to use c-extension module `ppp`. For example, on Ubuntu the ppp module `ppp.cpython-36m-x86_64-linux-gnu.so` is installed to `/usr/lib/python3/dist-packages/`. In that case, `python3 /usr/bin/geomPipeline.py manifest.json` will start an external process by python to run pipeline without using `ppp` module.

On windows, a batch file calling may be generated to run python script "geomPipeline.py" without "python path_to/geomePipeline.py".

## Advanced usage (adjust pipeline parameters)

### Pipeline configuration generation (python script)

`geomPipeline.py` will generate a json configuration based on user input (by default `config.json` in the current folder),  then starts the geometry preprocessing pipeline. For example, the imprint action will be organized into a pipeline of several GeometryProcessors, with default parameters written into the `config.json`. 

If the output is not ideal, users can edit parameters in the generated `config.json` and re-run the pipeline by `python3 geomPipeline.py config.json`, or equally `pppGeomPipeline path_to_json_config.json`.  

Actually, all the processing computation is done by `pppGeomPipeline` which is an executable compiled from C++ code. This executable only accepts a json configuration file, e.g. `pppGeomPipeline path_to_json_config.json`.  

The split of high-level user-oriented python script and lower-level C++ program has the benefits:

+ to ease the debugging of mixed python and C++ programming
+ to ease the parallel programming, since Python has the GIL problem
