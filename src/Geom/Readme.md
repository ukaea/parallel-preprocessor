# Design of Geom module

Feature overview is located in wiki folder.

## CLI design

subcommand style CLI will be enforced in the future. 

https://dzone.com/articles/multi-level-argparse-python
https://planetask.medium.com/command-line-subcommands-with-pythons-argparse-4dbac80f7110

FIXED:
`working-dir` cause error, because input file has not been translated into abspath, then change cwd to `working-dir`, so input file can not been found. it has been fixed.

TODO:
`dataStorage` in config.json, `"workingDir": args.workingDir,` 
a better name `data-dir` should be used instead of `working-dir`

## Imprint

currently merge is done by a single thread

### merge is delayed until writing result file

https://github.com/ukaea/parallel-preprocessor/issues/23

merge will invalidate `Item()`

## Scaling 

### CAD length units
In CAD world, the length unit by default im "MM", while in CAE world, ISO meter is the default unit.

Input geometry unit, should be saved in GeometryData class by GeometryReader, and check during write output.

1. OpenCASCADE brep file has no meta data for unit

2. unit or scale for SAT file format

> open your sat file with a text editor, change the first parameter of line 3 to "1" from "1000". This change will change the file unit from Meter to Millimeter.

3. Parasolid-XT	Meters and radians by convention
https://docs.cadexchanger.com/sdk/sdk_datamodel_units_page.html

### change output unit will cause scaling
add a new arg "--output-unit M" in python CLI interface

### delayed scaling until writing
calc scale ratio according to inputUnit and outputUnit

Transform will invalidate mesh triangulation attached to shape !!!

Transform maybe done in parallel, before merge, not sure it is worth of such operation?


1. OpenCASCADE STEP writer
`Interface_Static_SetCVal("Interface_Static_SetCVal("write.step.unit","MM")`

`outputUnit` in GeometryWriter config can scale, based on the `inputUnit`


2. OpenCASCADE brep write

`TopoDS_Shape`  this is unitless data structure, to scale, using `gp_Trsf`

```c++
    gp_Trsf ts;
    ts.SetScale(origin, scale);
    return BRepBuilderAPI_Transform(from, ts, true);
```

## To switch between CAD kernel
make new GeometryData derived class, put all OccUtils function as class virtual member functions, but there is performance penalty (virtual functions)