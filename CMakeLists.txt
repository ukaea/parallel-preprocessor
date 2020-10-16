cmake_minimum_required(VERSION 3.0)
#cmake_minimum_required(VERSION 2.8.11)  # centos7, not tested yet

set(PACKAGE_NAME "parallel-preprocessor")
set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "3")
set(PACKAGE_VERSION_PATCH "0")
execute_process(
    COMMAND
        git rev-parse --short HEAD
    RESULT_VARIABLE
        SHORT_HASH_RESULT
    OUTPUT_VARIABLE
        PACKAGE_VERSION_COMMIT_HASH)

# this version name is used by CPack
# cmake -DPPP_VERSION_NAME="0.3.1" would be useful to generate tagged package name
if(PPP_VERSION_NAME)
    set(PACKAGE_VERSION_NAME "${PPP_VERSION_NAME}")
else()
    set(PACKAGE_VERSION_NAME "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}-dev")
    #set(PACKAGE_VERSION_NAME "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")
endif()

project("parallel-preprocessor")
set(PROJECT_BRIEF "MPI distributive and multithreading massively parallel geometry processor \
    for large scale CAE and machine learning, by Qingfeng Xia, UKAEA, 2019")

##############################################################

# it is fine to use clang, but more warning.  CXX should be set in cmake command line
#SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")

# std can and should be applied to target only
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(MSVC)
  find_package(Boost)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

# FindMPI cause error in fedora gitlab runner, disable MPI as it not in used

#################################################################
# options are cached, `rm -rf build` dir and start freshly
###################### module selection ##########################
# App module must be enabled
option(PPP_USE_TYPE "use type system in base modlule" ON)
option(PPP_USE_GEOM "enable Geom module" ON)
option(PPP_USE_PYTHON "compile python wrapper" ON)
option(PPP_USE_TEST "use unit test module" ON)
option(PPP_USE_DOC "build doxygen dox" ON)

option(PPP_USE_CONDA "use Anaconda to install dependency on windows" ON)

################## dependency selection ##########################
# threading must be enabled
option(PPP_USE_TBB "use intel TBB for concurrent container and threadpool" ON)
option(PPP_USE_MPI "use intel MPI for distributive parallel" OFF)
option(PPP_USE_OCC "use OpenCASCADE open source CAD kernel" ON)

option(PPP_USE_GUI "use QT5 GUI toolbox to view goeometry" OFF)
option(PPP_USE_WEB "use websocket to view goeometry remotely" OFF)
option(PPP_USE_PCH "use precompiled header" OFF)  # it is fine with GCC 

# Enable or Disable Clang Tidy in Build
option(CLANG_TIDY "Enable Clang Tidy in Build" OFF)
# Enable or Disable Coverage with LCov During Compilation
option(CODE_COVERAGE "Enable code coverage with LCov" OFF)


##############################################################
# Platform specific cmake option override
##############################################################
if(WIN32)
    # disable doc on Windows platform
    set(PPP_USE_DOC OFF)
    set(PPP_USE_TYPE ON)
endif()

##############################################################
# PPP_DEV_MODE=ON is an env var defiend in $HOME/.profile, not .bashrc
# the developer can turn on more options on local PC
##############################################################
set(PPP_DEV_MODE $ENV{PPP_DEV_MODE})
if(PPP_DEV_MODE)
    message(STATUS "$ENV{PPP_DEV_MODE}, ${PPP_DEV_MODE}")
    message(STATUS "the developer mode can turn on more options than CI")
    set(PPP_USE_GUI OFF)
    set(PPP_USE_WEB OFF)
    #set(CMAKE_BUILD_TYPE Debug)  ## almost no difference in performance
    message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}, CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
endif()


message("----- parallel-preprocessor Build Options -----")
message(STATUS "Clang Tidy: ${CLANG_TIDY}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Code Coverage: ${CODE_COVERAGE}")
message("------------------------------------")


##############################################################
# coverage setup and clang_tidy setup
##############################################################
if(UNIX)
    if(CLANG_TIDY)
        if(APPLE)
        # Disable Clang-Tidy for macOS (as not available by default)
            message(WARNING "Clang Tidy option not currently supported on macOS, ignoring option")
        else()
            set(CLANG_TIDY_FLAGS -checks=*,-*default-arguments-calls*,-llvm-include-order*,-modernize-use-trailing-return-type*,-readability-isolate-declaration*,-google-runtime-references*,-fuchsia-*,-llvm-header-guard* -header-filter=${CMAKE_SOURCE_DIR}/src,-${CMAKE_CURRENT_SOURCE_DIR}/src/tclap)
            set(CMAKE_CXX_CLANG_TIDY clang-tidy ${CLANG_TIDY_FLAGS})
        endif(APPLE)
    endif()
    if(CODE_COVERAGE)
        message(STATUS "Including Code Coverage, so must be debug build type")
        set(COVERAGE_COMPILER_FLAGS "-fprofile-arcs -ftest-coverage")
        set(CMAKE_BUILD_TYPE Debug)
        set(LCOV_EXCLUDES "build/*" "*/third-party/*" "/usr*")
        set (CMAKE_CXX_FLAGS "${COVERAGE_COMPILER_FLAGS} ${CMAKE_CXX_FLAGS}")
        include("cMake/ppp_coverage_target.cmake")  # this cmake is project depedent, running scripts/run_all_test.sh in the build folder
    endif()
endif()


####################################################

# extra search path provided by this project, such OCCT SMesh
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cMake")



## put all targets in bin and lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
link_directories(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})


find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()


if(PPP_USE_PCH)
    add_definitions(-D_PreComp_)
    # MSVC need more setup, see end of file  `FreeCAD/src/Mod/Part/PPP/CMakeList.txt`
endif()

include_directories("${PROJECT_SOURCE_DIR}")  # for json, must be run before include src/ subfolder
include_directories("${PROJECT_SOURCE_DIR}/third-party")  # for Base module, not needed if Base is converted into git submodule
if(PPP_USE_TEST)
    add_subdirectory(third-party/Catch2)
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/third-party/loguru")
    message(FATAL_ERROR "have you forgot to download all other submodule before run cmake\n"
            "in the repo root folder, run `git submodule update --init --recursive`")
endif()

if(PPP_USE_GUI)
    # the other choice is target_compile_definitions() in each module
    add_definitions(-DPPP_BUILD_GUI=1)
endif()

if(PPP_USE_TYPE)
    add_definitions(-DPPP_BUILD_TYPE=1)
    add_subdirectory("third-party/Base")    # build libpppBase shared library as target `MyBase`
endif()

# must be put after third-party
include_directories("src")
add_subdirectory("src")



message("============= test preparation =================== \n"
" copy `python` source dir into `ppptest` in the building folder\n"
" symbolic link `data` folder into build folder for testing \n"
" run `./run_all_tests.sh` in the build folder after building \n "
"============= end of preparation =================== \n")

######################## data for test ###########################
if(WIN32)
    message(" symbolic link may not be supported on windows 10 before 2016 without admin previledge, xcopy instead \n")
    # mklink is not recongized, so copy instead on Windows
    execute_process (
        #COMMAND cmd /c "IF  not EXIST \"${PROJECT_BINARY_DIR}\\data\" ( mklink /J  \"${PROJECT_BINARY_DIR}\\data\" \"${PROJECT_SOURCE_DIR}\\data\" )"
        COMMAND cmd /c " xcopy  \"${PROJECT_SOURCE_DIR}\\data\" \"${PROJECT_BINARY_DIR}\\data\" /c /l /v /e /k /y "
        OUTPUT_VARIABLE outVar
    )
else()
    execute_process (
        COMMAND bash -c "if [ ! -s ${PROJECT_BINARY_DIR}/data ] ; then ln -s ${PROJECT_SOURCE_DIR}/data ${PROJECT_BINARY_DIR}/data ; fi"
        OUTPUT_VARIABLE outVar
    )

endif()

######################## pyhton files and shell scripts for test ######################
# will override each time? yes
file(COPY ${PROJECT_SOURCE_DIR}/scripts/run_all_tests.sh 
    DESTINATION ${PROJECT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
# copy python/ into build folder, to enable test in build folder without installation
file(COPY  ${PROJECT_SOURCE_DIR}/src/python 
    DESTINATION ${PROJECT_BINARY_DIR}/ppptest
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

############################################################
# CPack configuration  `make package`
############################################################
# ${PROJECT_SOURCE_DIR}/cMake/ppp_package
include(ppp_package)

############################################################
# Create doxygen documentation `make doc`
############################################################
# add a target to generate API documentation with Doxygen
if(PPP_USE_DOC)
    find_package(Doxygen QUIET)
    option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

    if(BUILD_DOCUMENTATION)
        if(NOT DOXYGEN_FOUND)
            message(FATAL_ERROR "Doxygen is needed to build the documentation.")
        endif()

        set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${doxyfile_in} ${doxyfile} @ONLY)

        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)

        # by default it is installed to PREFIX/share/doc,  skip install if online only
        #install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION doc)
    endif()
endif()
