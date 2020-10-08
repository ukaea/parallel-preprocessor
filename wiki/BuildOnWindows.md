## Build on Windows 

Note: Windows support is quite limited (only 64bit windows10), tested 

+ PPP compiled by visual studio build tool 2019, and link with OpenCASCADE 7.4

+ install occt 7.4 from conda-forge channel

Note: only 64bit Python3/Anaconda,  vs compiler x64 will be supported.

mingw32-x64 has been tested, but failed for bugs in C++17 filesystem support. 

### Importance of visual studio C++ support

Python3 on windows are using visual C++ to compile module, in order to interoperation with cpython, ppp should be compiled with visual C++.
Although linux is primary platform for PPP, but multiple-threading on windows platform is also essential for CAD users. 

> Intel C++ is compatible with Microsoft Visual C++ on Windows and integrates into Microsoft Visual Studio. On Linux and Mac, it is compatible with GNU Compiler Collection (GCC) and the GNU toolchain.
<https://en.wikipedia.org/wiki/Intel_C%2B%2B_Compiler>

https://software.intel.com/content/www/us/en/develop/documentation/cpp-compiler-developer-guide-and-reference/top/compatibility-and-portability/microsoft-compatibility.html

Is intel C++ compiler binary compatible with visual c++?


### Install VS2017 or VS2019 VS build tool

Compiler must be VS2017 or VS2019, OpenCASCADE 7.4 is built with VS2017.   Since VS2015, VS final has binary back-compatible for compiler,  so VS2019 compiler can link to lib built by vs2015, vs2017. 

1. Open the "Developer Command Prompt for VS2019"
official doc, using vs2017

2. cd C:\OpenCASCADE-7.4.0-vc14-64\  and  run `env.bat`
if CASROOT env var has been set, this step maybe skip for cmake building

3. cd to "parallel-preprocessor" and `mkdir vcbuild` 

`cd vcbuild && cmake  .. -G \"NMake Makefiles\" && cmake --build .`


### Install Anaconda and dependencies

`cmake git`, are required on PATH to compile, in addition to a visual c++ compiler.
`cmake` can be installed by `conda install cmake`, cmake-gui is not available if cmake is installed by condas, but user can use "-DPPP_USE_TEST=OFF " to turn off some options.
`git` can be installed from "Git for Windows" installer. 

`boost`, can be installed by conda
`pybind11` can be installed by conda, alternatively `pybind11` can be downloaded (git clone) from github during cmake build processs

There are 3 ways to install OpenCASACDE.  TBB is a dependent for occt package on conda-forge and bundled with official OpenCASACDE release

### Method 1 (recommended): install OpenCASACDE via conda 

```sh
# conda for python 3.7 has problem recently, got stuck on `solve environment` forever,
#  so create a new env of python 3.6

conda create -n cf python=3.6
conda activate cf

conda install -c conda-forge occt=7.4
# it will install tbb-devel, tbb, qt, vtk, hdf5, jsoncpp, loguru
```

Launch vistual studio build tool native x64 command prompt, assuming python on path is anaconda's python 3
```sh
conda activate cf
mkdir condabuild
cd condabuild
cmake -G "NMake Makefiles" ..
cmake --build .
```

NOTE: after install occt, conda can not install other package any more, conflict error. 

cmake on PATH is still the with conda base environment, but it can detect correct OpenCASACDE lib, just like on Linux

test the executable, it is working, virtual env has higher priority than base conda env, so correct python version and dll can be found. 

Apparently, conda is the better way to compile and run "Parallel Preprocessor". 

### Method 2: Install official OpenCASCADE 7.4

It is compiled with visual studio C++ 2017. Customed installation, install only source code (we need header files) and binary files,  is sufficiennt  about 1GB.
validate the installation by run sample app.  `Inspector.exe`

As suggested in define OpenCASCADE official document, define the env var `CASROOT`, to the folder you can find `env.bat`

> You can define the environment variables with env.bat script located in  the $CASROOT folder. This script accepts two arguments to be used: the  version of Visual Studio (vc10 – vc142) and the architecture (win32 or  win64).

However, cmake version too high, got warning
> Policy CMP0074 is not set: find_package uses <PackageName>_ROOT variables.  For compatibility, CMake is ignoring the variable.
It means does not def env var with name *ROOT, but *_DIR for latest cmake. 

A few more environment variables can be defined,  `QT_DIR, FreeImage, FreeTypes, VTK` are also bundled with OCCT.
Parallel-preprocessor's project CMakeLists.txt has code section to detect `CASROOT` from environment variable and then set `TBB_DIR`

#### FindTBB failed, but mingw is fine!  MSVC_VERSION is outdated

```cmake
elseif((MSVC_VERSION EQUAL 1900)  OR (MSVC_VERSION GREATER 1900))
    set(COMPILER_PREFIX "vc14")
```


### Method 3:  FreeCAD libpack to build (skipped)

One potential way to compile ppp is to use the header and dll files from FreeCAD libpack. 
<https://wiki.freecadweb.org/Compile_on_Windows>

During installation, set the ppp installation path the same as FreeCAD installation path `D:\Software\FreeCAD0.18`,
all dll goes into `D:\Software\FreeCAD0.18\bin` and all python sripts installed to FreeCAD's python's site-packages. 

As conda build and packaging is working properly on windows, this option will not be explored


### Packaging for windows (todo)

#### windows installer generated by cmake

CMake's cpack can generate NSIS windows installer, it is not tested. 

https://martinrotter.github.io/it-programming/2014/05/09/integrating-nsis-cmake/

The installer does not contain all TBB, OCCT dlls, therefore, it can either copy all TBB, OCCT dll into package? or make them on windows PATH,
to run the parallel-preprocessor app and python module.

#### package by conda-build
see <./CondaBuildGuide.md>
conda package: OCCT and TBB dll files can be found on path,  `ppp.pyd` file should be put into anaconda's site-package folder


###  To run PPP unit test

#### If using official OCCT release
DLL can not be found,  set PATH global in env var

`set PATH=%PATH%;D:\Software\OpenCASCADE-7.4.0-vc14-64\tbb_2017.0.100\lib\intel64\vc14;D:\Software\OpenCASCADE-7.4.0-vc14-64\opencascade-7.4.0\win64\vc14\lib`

PATH for  *.dll, in cmd terminal
`set PATH=%PATH%;D:\\Software\\OpenCASCADE-7.4.0-vc14-64\\tbb_2017.0.100\\bin\\intel64\\vc14;D:\\Software\\OpenCASCADE-7.4.0-vc14-64\\opencascade-7.4.0\\win64\\vc14\\bin`

https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order#standard-search-order-for-desktop-applications

pppParallelTests.exe  assertion failed, due to file path problem, solved 

pppAppTests.exe can be debugged now, due to API different on Windows and Linux,  it is sovled by conditional compiling
> __forceinline errno_t __cdecl gmtime_s(struct tm *_Tm, const time_t *_Time) { return _gmtime64_s(_Tm, _Time); }

`pppGeomTests.exe`  no output, no error message. 
`pppGeomTests.exe` does not even enter running mode, in vs code, launch.json, set ` "stopAtEntry": true,`,  if there is still no output in debug console, it may means opencascade dll incompatabile. 
if dll is not found, there should be a warning  dialog and give the missing dll name. 

Is that because, visual studio 2019 compiled PPP and visual studio 2017 built OpenCASCADE DLL has some comptable issue?

Binary compatible for visual c++ , according to wikipedia

> All 14.x MSVC releases have a stable ABI,[[49\]](https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B#cite_note-49) and binaries built with these versions can be mixed in a forwards-compatible manner, noting the following restrictions:

    - The toolset version used must be equal to or higher than the highest toolset version used to build any linked binaries.
    - The MSVC Redistributable version must be equal to or higher than the toolset version used by any application component.
    - Static libraries or object files compiled with /GL (Whole program optimisation) aren't binary compatible between versions and must use the exact same toolset.

https://docs.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017?view=vs-2019

#### If using OCCT installed from conda-forge

even with visual studio 2019 compiler, there is no problem running all unit tests. 


#### IF using FreeCAD libpack  (skipped)
FreeCAD/bin has all DLL neeeed  (if dll version is same, FreeCAD 0.18.4 used OCCT 7.3), but is not a good idea, get into PATH