## Port to Windows

This document records some notes on porting parallel-preprocessor to Windows 10

### Platform C++ runtime difference

C++ demangle, needs boost if using msvc compiler

Windows has no complete "signal.h" support. 
```c++
#include <csignal>
sigemptyset()   // no such on windows? mingw32?
```

### mingw32-w64 v8.1 compiler

mingw has problem with filesystem, and there is no easy solution. and As the official OpenCASCADE has been built with  VC141 (VS2017)
The binary compatibility of mingw and visual C++ is doubtable in production scenario.  

https://github.com/mxe/mxe/issues/2220
the gcc 9 patch seems to fix this issue
https://sourceforge.net/p/mingw-w64/bugs/737/

there is the error message

> D:/Software/mingw64/lib/gcc/x86_64-w64-mingw32/8.1.0/include/c++/bits/fs_path.h:237:47: error: no match for 'operator!=' (operand types are 'std::filesystem::__cxx11::path' and 'std::filesystem::__cxx11::path')
    || (__p.has_root_name() && __p.root_name() != root_name()))


Stop trying on mingw. 

### Windows visual c++ compiler different from g++

1. Visual C++ does not define `__cplusplus` properly to detect C++ std supported. 
Visual C++ support C++9x or just C++14, skipped c++11
`#ifndef _MSC_VER&& __cplusplus < 201402L`

2. NMake does not support parallel compiling, but msbuild has parallel support

3. To let visual C++ to support `not, and, or` keywords,  `#include <iso646.h>`

4. Clang' command line options is comptable with C++, but visual studio is distinct. 

5. VC (or even MingG32?)  link to  OCCT *.lib  (import lib), and to run needs *.dll files

6. some macro is needed to assist symbol import and export from dll, `DLLimport` 


#### conda install boost can not link to boost lib
-- Found the following Boost libraries:
--   system
'libboost_filesystem-vc140-mt-gd-x64-1_67.lib'

LINK : fatal error LNK1104: cannot open file 'libboost_filesystem-vc141-mt-gd-x64-1_67.lib'


### Visual C++ compiling output

There are static libraries (LIB) and dynamic libraries (DLL) - but note that .LIB files can be either static libraries (containing object files) or import libraries (containing symbols to allow the linker to link to a DLL).

*.exp  symbol export file (.exp) for the program,  `dumpbin /exports`
*.def  symbol definition file,  `__declspec(dllexport)`
*.lib  could be import library, for linker to link  and find symbols
*.ilk - this file is for incremental linking
*.pdb - Program database (.pdb) files, also called symbol files, map identifiers and statements in your project's source code to corresponding identifiers and instructions in compiled apps. These mapping files link the debugger to your source code, which enables debugging.

Typically DLLs go in a folder named “bin”, and import libraries go in a folder named “lib”

### dllexport for windows DLL

** header only library does not needs, __declspec(dllexport), only for impl in cpp files**

You can export data, functions, classes, or class member functions from a DLL using the `__declspec(dllexport)` keyword. 
`__declspec(dllexport)` adds the export directive to the object file so you do not need to use a .def file.  
MingW32 still needs this `__declspec(dllexport)` for windows. 

In short,  when building this module, `__declspec(dllexport)` should be used in header files to export symbols, while to use those symbols(functions/types) in another module,
` __declspec(dllimport)` should be used in the header files.  A more flexible solution:  for each cmake SHARED library target. e.g. define

```c++
#if _WIN32
#   ifdef APP_DLL_EXPORT
#       define  AppExport  __declspec(dllexport)
#   else
#       define  AppExport  __declspec(dllimport)
#   endif
#else
#   define  AppExport
#endif
```
Use the macro  for each class, struct, enum `class AppExport Processor` 
Also def the option, for the target, to export symbols

```py
if(WIN32)
    target_compile_definitions(MyApp PRIVATE APP_DLL_EXPORT=1)  # DLL export on windows
endif()
```

with this definition, cmake can generate pppApp.lib, but it is not in the output lib folder `lib` ,  but in another folder in the build directory `PPP/pppApp.lib`
TODO: let cmake collect those *.lib and put into lib output folder. 

for header only library, that is not necessary? 
```c++
extern "C"
{ // only need to export C interface if used by C++ source code
#include "third-party/mm_io.h"
}
```

`set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)`

#### for free functions inside namespace

free function means not member functions of any class/struct. 

They do need  `__declspec(dllexport)` , but if no other module use it, it is internal function, there is no need to export the function. 
However, in the case of function template, the keyword `__declspec(dllexport)` needs to be present explicitly for the symbols to be present on the dynamic library.
```c++
template AppExport void HelpingRegistration<double>( double );
AppExport void my_function(void);
```

 error C2491: definition of dllimport static data member not allowed
 > check  cmake for that module, e.g. copy-and-paste, but forget to change target name to the new target name
 ```cmake
if(WIN32)
    target_compile_definitions(MyApp PRIVATE APP_DLL_EXPORT=1)  # DLL export on windows
endif()
 ```

#### for free variable export

static variable may cause some trouble. 

#### for template class 
There is no need to specify `__declspec(dllexport)`, since template class impl usually in header file. 

> A template will only be compiled with specified template parameter. Thus there will be nothing in the dll to link against.
  In order to use a template you always need to include both: declaration and definition. You cannot put the definitions in a .cpp file like ordinary classes/functions. 
  http://www.cplusplus.com/forum/windows/199689/

If moving impl into cpp file, got missing symbol error, when used by another target/module.

https://stackoverflow.com/questions/17519879/visual-studio-dll-export-issue-for-class-and-function-template-instantiations

I believe you need to export the specializations. Have you tried this in your .cpp file:
```c++
template class EXPORT TemplatedStaticLib<double>;
template class EXPORT TemplatedStaticLib<std::string>;

//and pretty much the same in your header:

template class EXPORT TemplateStaticLib<double>;
template class EXPORT TemplateStaticLib<std::string>;

```
warning C4661: 'PPP::SparseMatrix<bool> PPP::SparseMatrix<bool>::readMatrixMarketFile(const std::string &)': no suitable definition provided for explicit template instantiation request

for typedef 

#### for third-party headers?  
1. use all third-party headers in cpp file, without exposed any symbol to public header files
2. SparseMatrix is a template class, its implementation should be only in header file, so  mm_io.h must be included in SparseMatrix.h,
In this situation, modified mm_io.h by adding  "AppExport" macro
3. loguru.h's LOG_F() can print real line number, and it is a submodule. Therefore do not modify loguru.h directly, by appending `AppExport` to each functions in loguru.hpp
but including loguru.cpp in each build target (*.dll, *.so), to solve the problem of symbol not defined/exported. 
```c++
#if defined(_WIN32)
#include "../third-party/loguru/loguru.cpp"
#endif
```

### suppress msvc warning on symbol exporting

https://stackoverflow.com/questions/24511376/how-to-dllexport-a-class-derived-from-stdruntime-error
How to dllexport a class derived from std::runtime_error?

`class __declspec(dllexport) BaseException : public std::runtime_error`
However, this gives me 
> warning C4275: non – DLL-interface class 'std::runtime_error' used as base for DLL-interface class 'BaseException'.

warning C4273: 'Geom::GeometryFixer::init': inconsistent dll linkage
warning C4275: non dll-interface class 'std::exception' used as base for dll-interface class 'PPP::ProcessorError'

Solutions:
    Export it.
    Ignore it.
    In-line it.

**warning C4275: non dll-interface class used as base for dll-interface class**

```
   if(MSVC)
        target_compile_options(fmt PRIVATE /wd4251  /wd4275)
    endif()
```


