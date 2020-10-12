## Build on MacOS

MacOS is a Unix-like system with POSIX interface, which makes software porting from Linux straight-forward. 

### Install Homebrew package manager

It is assumed c++ compiler have been installed, 

Homebrow is the missing package manager for MacOS, and it make software installation as convenient as `apt install pkg-name` on Linux. 

`/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"`


### Install Dependencies ( Mac OS X )

Install some developer tools for QA and documenatation generation
`brew update && brew install  cppcheck lcov poppler htmldoc graphviz doxygen`

Install third-party libraries that Parallel-Preprocessor relies on:
`brew install  boost tbb opencascade`
Those homebrew packages contents both binary library files and header files.

Qt and FreeCAD are optional dependencies.  To install the stable release of FreeCAD: run `brew cask install freecad`, and to use the latest FreeCAD available (0.19pre) run `brew install freecad`. 

### Compiling and installation

Which python to use?  MacOS has built in Python framework, other choice can be Anaconda, Homebrew python.  using `which python` to identify the python.

`cmake -DPython_EXECUTABLE=full_path_to_python  path_to_project_source`

After successfully build,  `make package` should generate a drag and drop in on .dmg package (yet tested) , driven by CPack. Note, cpack also support generate App bundle package for MacOS.

Homebrew formula could be added in the future.


### Xcode command line tool

Program can be compiled by XCode, may not compile with "Xcode command line tool". 
There may be compile error like this:

>  No rule to make target `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.15.sdk/System/Library/Frameworks/AppKit.framework', needed by `main'.  Stop.


A quick fix will be symbolic link from `/Library/Developer/CommandLintTool/SDKs/MacOSX10.15.sdk`to `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.15.sdk`

Other solution, yet tested:
```
SET(CMAKE_OSX_SYSROOT "/usr/lib/apple/SDKs/MacOSX10.11.sdk" CACHE PATH "Path to OSX SDK")
SET(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT})
```


### Notes on porting to MacOS

issue #1:  compiling failed on MacOS 10.15. 

```c++
/// NOTE: tbb::task_group does not support std::make_shared<>() on clang
auto threadPool = std::shared_ptr<ThreadPoolType>(new ThreadPoolType());
```

Clang and g++ use libc++.dylib and clang's header on MacOS, which have stricter requirement on classes that can be used in `make_shared<T>()`


#### C++17 FileSystem library linkage

Linkage to `stdc++fs` is required on Linux but not on MacOS

```
    if(NOT APPLE)
        if(GNUC)
            link_libraries(stdc++fs)  # this seems only for GCC compiler
        endif()
    endif()
```

#### Detect MacOS version in cmake

```
IF (APPLE)
   EXEC_PROGRAM(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
   STRING(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
   MESSAGE(STATUS "DARWIN_VERSION=${DARWIN_VERSION}")
   IF (DARWIN_VERSION GREATER 8)
     SET(APPLE_LEOPARD 1 INTERNAL)
     ADD_DEFINITIONS(-DAPPLE_LEOPARD)
   ENDIF (DARWIN_VERSION GREATER 8)
ENDIF(APPLE)
```

you can also parse the output of /usr/bin/sw_vers, which will give you
the OSX version (instead of the darwin version) and the build number.

https://cmake.org/pipermail/cmake/2007-October/017290.html