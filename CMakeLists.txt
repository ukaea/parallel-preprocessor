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
set(PROJECT_VERSION ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR})

##############################################################

# it is fine to use clang, but more warning.  CXX should be set in cmake command line
#SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")

# std can and should be applied to target only
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if((${CMAKE_CXX_STANDARD} GREATER 17)  OR  (${CMAKE_CXX_STANDARD} EQUAL 17))
    if(GNUC)
        link_libraries(stdc++fs)  # this seems only for GCC compiler
    
    endif()
else()
    if(OFF)  # windows conda install has problem of vc version
        find_package(Boost REQUIRED COMPONENTS
                    filesystem)
        link_libraries(Boost::filesystem)
    endif()
endif()


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


message("----- parallel-preprocessor Build Options -----")
message(STATUS "Clang Tidy: ${CLANG_TIDY}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Code Coverage: ${CODE_COVERAGE}")
message("------------------------------------")
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
        message(STATUS "Including Code Coverage")
        set(COVERAGE_COMPILER_FLAGS "-g -fprofile-arcs -ftest-coverage")
        set(LCOV_EXCLUDES "build/*" "*/third-party/*" "/usr*")
        set (CMAKE_CXX_FLAGS "${COVERAGE_COMPILER_FLAGS} ${CMAKE_CXX_FLAGS}")
        include("cMake/ppp_coverage_target.cmake")
    endif()
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

####################################################

# extra search path provided by this project, such OCCT SMesh
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cMake")


if(WIN32)
    # disable doc on Windows platform
    set(PPP_USE_DOC OFF)
    set(PPP_USE_TYPE ON)

    if(PPP_USE_CONDA)
        # do nothing, cmake can detect occt and tbb installed by conda on windows
    else(PPP_USE_CONDA)
        if(DEFINED $ENV{CASROOT})
            message(STATUS "env var CASROOT defined as $ENV{CASROOT}, it is required on windows")
        endif()
        set(OCC_DIR "D:/Software/OpenCASCADE-7.4.0-vc14-64/opencascade-7.4.0")

        # must prepend, otherwise, this repo's FindOpenCasCade.cmake take the priority over FindOPENCASCADE.cmake
        list(PREPEND CMAKE_MODULE_PATH  "${OCC_DIR}/cmake")
        set(OCC_INCLUDE_DIR "${OCC_DIR}/inc")  # the hint to detect all components
        message(STATUS "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH} on windows")

        # TBB_ROOT is a environment var, or cmake var hint
        # Policy CMP0074 is not set: find_package uses <PackageName>_ROOT variables.
        set(TBB_ROOT_DIR "${OCC_ROOT}/../tbb_2017.0.100")
    endif()
endif()

# it can be used by subdirectory cmakelist.txt
include_directories("./") 

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

if(PPP_USE_GUI)
    # the other choice is target_compile_definitions() in each module
    add_definitions(-DPPP_BUILD_GUI=1)
endif()

if(PPP_USE_TYPE)
    add_definitions(-DPPP_BUILD_TYPE=1)
    add_subdirectory("third-party/Base")    
endif()

if(PPP_USE_TBB)
    if(WIN32)
        find_package(TBB REQUIRED)  # in the future it may be replaced by an in-house implementation
    else()
        find_package(TBB REQUIRED) 
    endif()
    if(TBB_FOUND)
        message("found TBB version: ${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}")
        ## For propertyContainer.hpp, but concurrent container does NOT support operator []
        #add_definitions("-DUSE_CONCURRENT_HASH_CONTAINER")
        include_directories(${TBB_INCLUDE_DIRS})
        #link_directories(${TBB_LIBRARY_DIRS})  # no such var,  libraries has already the full path
        link_libraries(${TBB_LIBRARIES})
    endif()
endif()

############################################################


include_directories("${PROJECT_SOURCE_DIR}/third-party")  # for json, must be run before include App subfolder
add_subdirectory("PPP")

# deal with data and script subdirectory
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/third-party/loguru")
message(FATAL_ERROR "have you forgot to download all other submodule before run cmake\n"
        "in the repo root folder, run `git submodule update --init --recursive`")
endif()

##############################################################

if(PPP_USE_GEOM)
    # it is safe to add include and library directories at top level CMakeLists.txt
    # to be used by subdirectory like test and python interface
    # but do NOT use link_libraries(),  use target_link_libraries() instead

    if(UNIX)
        list(APPEND CMAKE_MODULE_PATH "/usr/lib/cmake/opencascade/")
    endif()

    if(OFF)  # use OCCT official cmake
        # need to install "occt-misc" debian package,  fedora name ???
        #  otherwise, missing package configuration OpenCASCADEConfig.cmake  

        #find_package(OpenCASCADE REQUIRED)
        #find_package(OpenCASCADE CONFIG QUIET)
        # still not working on windows

        find_package(OpenCASCADE COMPONENTS FoundationClasses REQUIRED)
        find_package(OpenCASCADE COMPONENTS ModelingData REQUIRED)
        find_package(OpenCASCADE COMPONENTS ModelingAlgorithms REQUIRED)
        find_package(OpenCASCADE COMPONENTS DataExchange REQUIRED)
        find_package(OpenCASCADE COMPONENTS ApplicationFramework REQUIRED)
        if(PPP_USE_GUI)
            find_package(OpenCASCADE COMPONENTS Visualization REQUIRED)
            set(OCC_VIZ_LIBS
                ${OpenCASCADE_Visualization_LIBRARIES}
            )
        endif()
        include_directories(${OpenCASCADE_INCLUDE_DIR})
        link_directories(${OpenCASCADE_LIBRARY_DIR})

        # adapted to be compatible with FreeCAD occ.cmake
        set(OCC_LIBS
            # variables defined in OpenCASCADEConfig.cmake  
            ${OpenCASCADE_FoundationClasses_LIBRARIES}
            ${OpenCASCADE_ModelingData_LIBRARIES}
            ${OpenCASCADE_ModelingAlgorithms_LIBRARIES}
            ${OpenCASCADE_DataExchange_LIBRARIES}
            ${OpenCASCADE_ApplicationFramework_LIBRARIES}
        )
        if(OpenCASCADE_FOUND)
            message(" found OpenCASCADSE installation")
        endif()
    else()
        find_package(OpenCasCade REQUIRED)   # use this project provided FindOpenCasCade.cmake
        if(OCC_FOUND)
            include_directories(${OCC_INCLUDE_DIR})
            link_directories(${OCC_LIBRARY_DIR})  
            set(OCC_LIBS
                ${OCC_LIBRARIES}
                ${OCC_OCAF_LIBRARIES}
            )
        else()
            message("===============================================================\n"
            "OpenCasCade not found, make sure oce or occt has been installed\n"
            "===============================================================\n")
        endif()
    endif()
endif()


if(PPP_USE_GEOM)
    add_subdirectory("Geom")
    add_subdirectory("third-party/SGeom")
    # only needed by Geom module
    include_directories("${PROJECT_SOURCE_DIR}/third-party/SGeom/src/GEOMAlgo")
endif()

######### auto foramt check by cmake or git hook, or IDE ################
#find_package(ClangFormat)
#if(NOT CLANG_FORMAT_FOUND)
#    set(CLANG_FORMAT_BIN_NAME clang-format)
#    set(CLANG_FORMAT_EXCLUDE_PATTERNS  "build/" ${CMAKE_BINARY_DIR})
#endif()

############################################################
# python wrap currently only Geom module
############################################################
if(PPP_USE_PYTHON)
    add_subdirectory("python")
endif(PPP_USE_PYTHON)


############################################################
# unit test module
############################################################
if(PPP_USE_TEST)
    add_subdirectory(test)
    add_subdirectory(third-party/Catch2)
    add_subdirectory (PropertyContainer)  # Catch2 must be add() before this subdirectionary
endif()


######################## data for test ###########################
#configure_file(<input> <output> COPYONLY)

if(WIN32)
    message(" symbolic link may not be supported on windows 10 before 2016 without admin previledge, xcopy instead \n")
    # mklink is not recongized, so copy instead
    execute_process (
        #COMMAND cmd /c "IF  not EXIST \"${PROJECT_BINARY_DIR}\\data\" ( mklink /J  \"${PROJECT_BINARY_DIR}\\data\" \"${PROJECT_SOURCE_DIR}\\data\" )"
        COMMAND cmd /c " xcopy  \"${PROJECT_SOURCE_DIR}\\data\" \"${PROJECT_BINARY_DIR}\\data\" /c /l /v /e /k /y "
        OUTPUT_VARIABLE outVar
    )
else()
    message("============= test preparation =================== \n"
            " copy `python` source dir into `ppptest` in the building folder\n"
            " symbolic link `data` folder into build folder for testing \n"
            " run `./run_all_tests.sh` in the build folder after building \n "
            "============= end of preparation =================== \n")
    execute_process (
        COMMAND bash -c "if [ ! -s ${PROJECT_BINARY_DIR}/data ] ; then ln -s ${PROJECT_SOURCE_DIR}/data ${PROJECT_BINARY_DIR}/data ; fi"
        OUTPUT_VARIABLE outVar
    )

endif()
# will override each time?
file(COPY ${PROJECT_SOURCE_DIR}/scripts/run_all_tests.sh 
    DESTINATION ${PROJECT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
# copy do not make symbolic to add temp file
file(COPY  ${PROJECT_SOURCE_DIR}/python 
    DESTINATION ${PROJECT_BINARY_DIR}/ppptest
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

###########################################
# Check 32/64 bit platform #
# copy from FreeCAD project, LGPL v3
###########################################
if (${CMAKE_SIZEOF_VOID_P} MATCHES "8") # It is 64bit, otherwise 32 bit systems match 4
	add_definitions(-D_OCC64)
	set(BIT_LENGTH 64)
else (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
	set(BIT_LENGTH 32)
endif(${CMAKE_SIZEOF_VOID_P} MATCHES "8")

# Define helper macro option_with_default
macro( option_with_default OPTION_NAME OPTION_STRING OPTION_DEFAULT )
	if( NOT DEFINED ${OPTION_NAME} )
		set( ${OPTION_NAME} ${OPTION_DEFAULT} )
	endif( NOT DEFINED ${OPTION_NAME} )
	option( ${OPTION_NAME} "${OPTION_STRING}" ${${OPTION_NAME}} )
endmacro( option_with_default OPTION_NAME OPTION_STRING OPTION_DEFAULT )

############################################################
# Create DEB, RPM, activated by the command `make package` or `cpack`
# simultaneously building multiple package types -DCPACK_GENERATOR="DEB;RPM")
# conda, pip 
############################################################

if (UNIX)
    include("${PROJECT_SOURCE_DIR}/cMake/DetectOS.cmake")
    message("OS detected by `DetectOS.cmake` as `${CMAKE_OS_NAME}` and `${CMAKE_OS_VERSION}`")
    if (${CMAKE_OS_NAME} STREQUAL "Debian" OR ${CMAKE_OS_NAME} STREQUAL "Ubuntu")
        # Tell CPack to generate a .deb package
        set(CPACK_GENERATOR "DEB")
        set(CPACK_PACKAGE_NAME "parallel-preprocessor_${CMAKE_OS_ID}-${CMAKE_OS_VERSION}")
        # -${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}  not defined variable
        string( TOLOWER "${PACKAGE_NAME}-${PACKAGE_VERSION_NAME}_${CMAKE_OS_ID}-${CMAKE_OS_VERSION}" 
                CPACK_PACKAGE_FILE_NAME )
    endif()
    if (${CMAKE_OS_NAME} STREQUAL "RedHat" OR ${CMAKE_OS_NAME} STREQUAL "Fedora")
        set(CPACK_GENERATOR "RPM")
        set(CPACK_PACKAGE_NAME "parallel-preprocessor_${CMAKE_OS_ID}-${CMAKE_OS_VERSION}")
    endif()
    if (${CMAKE_OS_NAME} STREQUAL "OSX")
        set(CPACK_GENERATOR "DRAGNDROP")
        set(CPACK_PACKAGE_NAME "parallel-preprocessor_${CMAKE_OS_ID}-${CMAKE_OS_VERSION}")
    endif()
endif()
if(WIN32)
    # NSIS can be generated by opencascade and Qt dll are not bundled, so not quite working
    set(CPACK_GENERATOR "NSIS64")  # zip 7z are other choices
    set(CPACK_PACKAGE_NAME "parallel-preprocessor-${CMAKE_OS_VERSION}_${CMAKE_OS_ID}")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ${PACKAGE_NAME})
endif()

# Set a Package Maintainer. This is required
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Qingfeng Xia @ UKAEA")
#set(CPACK_PACKAGE_DESCRIPTION, ${PROJECT_BRIEF})
set(CPACK_PACKAGE_DESCRIPTION, "${CMAKE_CURRENT_SOURCE_DIR}/Readme.md")
# Set a Package Version
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION_NAME})
#set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

################################################################

set(CPACK_IGNORE_FILES "\\.psd$;/\\.git/;/backup/;\\.#;/#;\\.tar.gz$;/stage/;/build/;/condabuild/;\\.diff$;\\.DS_Store'")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})

#################################################################
#  split into the runtime, python and development component packages
# COMPONENT and GROUP are different concepts
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/Component-Install-With-CPack
#################################################################


# https://dominoc925.blogspot.com/2016/09/create-windows-installer-using-cmake.html
set(PPP_SINGLE_PACKAGE ON)

if(NOT PPP_SINGLE_PACKAGE)
    set(CPACK_COMPONENTS_ALL applications libraries headers python data)
    # all components in one package is the default behaviour
    # windows and macos may select component in the GUI installer wizard
    # otherwise generate multple component packages on Linux
    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_DEB_COMPONENT_INSTALL ON)

    set (CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "MyLib Applications")
    set (CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
    set (CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
    set (CPACK_COMPONENT_PYTHON_DISPLAY_NAME "python interface")
endif()

# set(CPACK_COMPONENTS_GROUPING IGNORE)  #1 package per component
# set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE) # 1 package for all 
# 1 package per component group the default behavior. for deb and rpm
#set(CPACK_COMPONENT_APPLICATIONS_GROUP "Runtime")
#set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
#set(CPACK_COMPONENT_HEADERS_GROUP "Development")


#####################################################################
# windows NSIS-specific CPack variables                  
# to enable, or cmake with -DCPACK_GENERATOR="NSIS64"          
# cmake -DCPACK_GENERATOR="ZIP" ..                                  
#######################################################################
if( CPACK_GENERATOR MATCHES ".*NSIS.*" )

    #set(CPACK_NSIS_INSTALLED_ICON_NAME "${APP_LOW_NAME}.ico")
    #set(CPACK_NSIS_HELP_LINK ${APP_URL})
    #set(CPACK_NSIS_URL_INFO_ABOUT ${APP_URL})
    set(CPACK_NSIS_CONTACT ${CPACK_DEBIAN_PACKAGE_MAINTAINER})

endif()


#####################################################################
# Debian-specific CPack variables                  
# automatic enabled by DetectOS.cmake                                                 
#######################################################################
if( CPACK_GENERATOR MATCHES ".*DEB.*" )
	set(CPACK_DEBIAN_PACKAGE_SECTION "science")
	# potentially split nonGui and Gui packages, lots of dep are given by OCC
	set(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "cmake (>= 2.8),
        libocct-foundation-dev, libocct-data-exchange-dev, libocct-modeling-data-dev,
        libocct-modeling-algorithms-dev, libocct-ocaf-dev, occt-misc,
        python3-dev, tbb2-dev")
    # optional GUI remated : libx11-6, libxt6, libxext6, libxi-dev, libxmu-dev,
    #           libfreetype6 (>= 2.2.1), libfreeimage, 
    #           libocct-visualization-dev, tcl8.5 (>= 8.5.0), tk8.5 (>= 8.5.0), 
    # on Ubuntu, version number is part of package name, no package alias?
    #   libocct-foundation, libocct-data-exchange, libocct-modeling-data,
    #   libocct-modeling-algorithms, libocct-ocaf,
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.3), python3, libtbb2,
        libgcc1 (>= 1:4.1.1), libgomp1 (>= 4.2.1), libstdc++6 (>= 4.4.0)"
        )
	set(CPACK_DEBIAN_PACKAGE_SUGGESTS "freecad")
	set(CPACK_DEBIAN_PACKAGE_PROVIDES ${PROJECT_NAME})
	if( BIT_LENGTH EQUAL 64 )
		set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64" )
	else()
		set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386" )
	endif()
endif()

###############################################################################
# RPM-specific CPack variables.                                               #
# automatic enabled by DetectOS.cmake, 
# or enforace,   cmake with -DCPACK_GENERATOR="RPM", or uncomment             #
# set(CPACK_GENERATOR "RPM")                                                  #
###############################################################################
if( CPACK_GENERATOR MATCHES ".*RPM.*" )
    # c and c++ runtime should be skipped:  libstdc++6 >= 4.4.0, libc6 >= 2.3, libgcc >= 4.1.1, libgomp >= 4.2.1
    # X-windows, tcl and tk, they are dep of OpenCASCADE
    # using dnf list <package name> to find out the version,  project installation guide has the package name
    # libfreetype6 >= 2.2.1, libgl1-mesa-glx, libglu1-mesa,  libx11-6, libxext6, libxt6 tcl >= 8.5.0, tk >= 8.5.0
    # tk, tcl, tk-devel, tcl-devel, freetype, freetype-devel, freeimage, freeimage-devel, 
    # glew-devel, SDL2-devel, SDL2_image-devel, glm-devel, libXmu-devel, libXi-devel,
    set( CPACK_RPM_PACKAGE_REQUIRES "tbb, tbb-devel, python3")
	set( CPACK_RPM_PACKAGE_PROVIDES ${PACKAGE_NAME})
	set( CPACK_PACKAGE_RELOCATABLE "FALSE" )
	if( BIT_LENGTH EQUAL 64 )
		set( CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64" )
	else()
		set( CPACK_RPM_PACKAGE_ARCHITECTURE "i586" )
	endif()

endif()

# finally include CPack
include(CPack)


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
