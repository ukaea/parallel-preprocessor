## Build for Anaconda 

Conda package can be generated to support more Linux platforms, and Windows. Conda package build has been tested successfully on local host.  
In the future, parallel-preprocessor will be available on conda-forge channel for  `conda install -c condaforge parallel-preprocessor`  This conda package for parallel-preprocessor has both c++ binary and python wrapper installed, just like build deb package from source by cmake.


```
conda config --add channels conda-forge
conda config --set channel_priority strict
conda install occt
conda install parallel-preprocessor
```

### conda recipe: meta.yaml and build scripts

The meta.yaml and build scripts have been provided in the `recipe` folder of this repository.

The package config file `meta.yaml` can be generated by a command, or just adapted from an existing recipe online, e.g. 
<https://github.com/conda/conda-recipes/tree/master/libtiff>
Also note that the compilers used by conda-build can be specified using a `conda_build_config.yaml`.

The build scripts `build.sh` (for Unix-like OS) and `bld.bat` (windows batch file) have the install instruction (cmake is called just as compiling from source code.), install to a tmp prefix, then compressed into a zipped file, i.e. conda package.  


### build conda package from local machine

`parallel-preprocessor` depends on OpenCASCADE conda package, whose conda packaging configuration can be found at `https://github.com/conda-forge/occt-feedstock/`.  To build conda package from local machine, user should install all the necessary dependency, such as OpenCASCADE 7.x, TBB,  if conda build failed to download the dependencies specified in `meta.yaml`. 

To build conda package, first of all, install anaconda python3 and install necessary packages like `conda-build`, and run the command in the `recipe` folder of this repository: `conda build meta.ymal`

The conda build task is conducted a seperate virtual environment automatically, so is the test(run) process.  
`D:\Software\anaconda3\envs\cf\conda-bld\ppp_1597347412425`
On windows, cmake isntalled to the `base` conda environment can still detect `occt` package installed in the conda-build tmp conda environment.

NOTE: conda will not auto clean those tmp virtual env, user must manually remove the build virtual env.


anaconda upload /opt/anaconda3/conda-bld/linux-64/parallel-preprocessor-0.3-oct_2020.tar.bz2

To have conda build upload to anaconda.org automatically, use

`conda config --set anaconda_upload yes`


### install built local conda package

Where is the generated conda package?   
If testing script passed, the generated conda package, a compressed file, should be copied to the current working directory.  
If test failed, there may be error message:
> Tests failed for ppp-0.3-hdef9aff_2003.tar.bz2 - moving package to D:\Software\anaconda3\envs\cf\conda-bld\broken

After conda build, the built package can be installed:  `conda install --use-local ppp` or equally `conda install -c local ppp`  to install the built pkg. 
`conda install --use-local ppp --dry-run` to test if built package can be found and installed.   `conda verify` may also verify the package without installation.


It is expected binary conda package for parallel-preprocessor will be hosted on github or conda-forge.  After downloaded from github Release, the compressed package file can be installed by the command

```sh
conda install package-path/package-filename.tar.bz2
```

### Conda build variants

https://docs.conda.io/projects/conda-build/en/latest/resources/variants.html

selector document 

Here is an example to skip some variants in meta.yaml file.

```yaml
build:
    skip: true  # [win or osx]
```

### build for multiple python versions 

Since python the optional interface module for parallel preprocessor is C-extension, so separate binary packages are needed for conda python 3.6, 3.7 and 3.8. Depends on platforms, conda python 3.8 may be minimum version to support C++17. 

conda build support build for multiple version, by specify `--py` option multiple times.
`conda build --py 3.6 --py 3.7 --py 3.8  recipe` if this does not work,  try 
`conda build recipe --variants "{'python': ['2.7', '3.6'], 'vc': ['9', '14']}"`

After all, variant control is done by the yaml file`conda_build_config.yaml`. If this file does not exists same directory as` meta.yaml`, `conda build` may generate one. real also [Conda multi variant builds](https://medium.com/@MaheshSawaiker/conda-multi-variant-builds-8edc35c215d7)


### conda-forge build environment

Build environments in conda-forge CI (miniconda azure pipeline, ) is quite different from conda build on local machine. After trials and referencing to https://github.com/FreeCAD/FreeCAD/blob/master/conda/, finally conda build is working. 

conda windows python 3.8 using "VS 2017 update 9", although it is called `vs2015_runtime` indicating binary compatible, it may be using vs2017 (14.1) compiler. 

| [vs2015_runtime](https://docs.anaconda.com/anaconda/packages/py3.8_win-64/None) | 14.16.27012 | MSVC runtimes associated with cl.exe version 19.16.27032.1 (VS 2017 update 9) / None |
| ------------------------------------------------------------ | ----------- | ------------------------------------------------------------ |
| [vc](https://github.com/conda/conda/wiki/VC-features)        | 14.1        | A meta-package to impose mutual exclusivity among software built with different VS versions / Modified BSD License (3-clause) |

The default conda macos conda-build using MacOS SDK 10.09, XCode 11.6, but MacOS 10.13 is needed for C++17 support. Therefore, the MacOS deploy target has been limited to "10.15", in <../recipe/conda_build_config.yaml>


> /src/PropertyContainer/PropertyContainerTest.cpp:46:22: error: 'any_cast<std::__1::shared_ptr<A> >' is unavailable: introduced in macOS 10.13
auto data = std::any_cast<std::shared_ptr<myType>>(a);

### Upload to conda forge (yet done)

Here is guide from conda-forge website:
> If your package is not on conda-forge yet you can follow their contribution documentation here:  https://conda-forge.org/#add_recipe

The process: 
https://conda-forge.org/docs/maintainer/adding_pkgs.html

1. fork conda-forge example repo

https://github.com/qingfengxia/staged-recipes

2.  push it as a new repo from the branch "parallel-preprocessor"

https://github.com/conda-forge/staged-recipes/pull/12901#issuecomment-709514486


3. later the maintainer (pusher) will have the write control on that repo. 



