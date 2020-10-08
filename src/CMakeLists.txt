
if(WIN32)
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

add_subdirectory("PPP")


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
    add_subdirectory (PropertyContainer)  # Catch2 must be add() before this subdirectionary
endif()
