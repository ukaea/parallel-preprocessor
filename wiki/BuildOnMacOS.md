## Build on MacOS

MacOS is a Unix-like system with POSIX interface, which makes software porting from Linux straight fowards. 

### Install Homebrew package manager

It is assumed xcode and clang++ compiler have been installed. Only some third-party libraries and extra 

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

After sucessfully build,  `make package` should generate a drag and drop in on .dmg package (yet tested) , driven by CPack.

Homebrew formula could be added in the future.