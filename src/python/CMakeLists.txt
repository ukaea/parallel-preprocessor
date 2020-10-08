find_package(pybind11 QUIET)  # systemwide installation by package manager  `/usr/include/`
if(NOT pybind11_FOUND)
    message("===============================================================\n"
            "pybind11 not found on system, use the downloaded version \n"
            "===============================================================\n")
    if( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pybind11")
        execute_process(
            COMMAND git submodule add https://github.com/pybind/pybind11
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
    # there is no need to find_package, but just do the line below
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/pybind11")
    set(pybind11_FOUND ON)
    # if python version detection is not correct, remove CMakeCache.txt
    # cmake -DPYTHON_EXECUTABLE:FILEPATH=<path-to-python-executable> .
    message("found: PYTHON_VERSION_MAJOR = ${PYTHON_VERSION_MAJOR}")
endif()


if(pybind11_FOUND)

    pybind11_add_module(ppp "AppPy.cpp")
    target_link_libraries(ppp PRIVATE ${OCC_LIBS})  # is this line really needed?
    if(PPP_USE_TYPE)
        add_dependencies(ppp MyGeom MyBase)
        target_link_libraries(ppp PRIVATE MyBase)
    endif()
    target_link_libraries(ppp PRIVATE MyGeom)
    link_directories(${PROJECT_NAME} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    target_link_libraries(ppp PRIVATE pybind11::module)
    set_target_properties(ppp PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
                                            SUFFIX "${PYTHON_MODULE_EXTENSION}")
    message("======= python binding module built ======\n")


    # inside the build folder:  cmake  -DPYTHON_EXECUTABLE:FILEPATH=<path-to-python-executable> ../python
    # and run as an independent cmake project to generate python interface module for another py version
    # make install to install the ppp*.so
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
         # I am the top-level project.  OpenCascade and third-party including
         set(PARENT_SOURCE_DIR "../")
         list(APPEND CMAKE_MODULE_PATH "${PARENT_SOURCE_DIR}/cMake")
        find_package(OpenCasCade REQUIRED)   # use this project provided FindOpenCasCade.cmake
        if(OCC_FOUND)
            include_directories(${OCC_INCLUDE_DIR})
        else()
            message("===============================================================\n"
            "OpenCasCade not found, make sure oce or occt has been installed\n"
            "===============================================================\n")
        endif()
         include_directories("${PARENT_SOURCE_DIR}/third-party")  # for json, must be run before include App subfolder
         include_directories("${PARENT_SOURCE_DIR}")  # 
         link_directories(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    else()
         # I am called from other project with add_subdirectory().
    endif()


    # install to dynamic library path, just make pip wheel
    execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" -c "if True:
          from distutils import sysconfig as sc
          print(sc.get_python_lib(prefix='', plat_specific=True))"
        OUTPUT_VARIABLE PYTHON_SITE
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    message("ppp python modules will be installed to ${PYTHON_SITE}\n")

    install(TARGETS ppp  # single mod file goes into site-package folder
        #FILES ${mypackage_python_files}  # other files in a package folder
        DESTINATION ${PYTHON_SITE}
        COMPONENT python_interface  # just make it as "libraries"
        )

    # install to ${PYTHON_SITE} is ideal, 
    # 
    # or install to bin/ as the app compiled from C++ code, and make *.py executable +x
    # while, binary and py exe have the same name
    set(ppp_python_files
        "FreeCADParser.py"
        "detectFreeCAD.py"
        "geomPipeline.py"
        "pppPipelineController.py"
        "analyzeProcessedResult.py"
        "analyzeDumpFiles.py"
        "pppMonitorProgress.py"
        # python test_*.py are not included
    )

    # copy to build/bin for use without installation
    file(COPY ${ppp_python_files}
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                         GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )

    # need to generate package by cpack
    install(FILES ${ppp_python_files}
        #DESTINATION ${PYTHON_SITE}
        DESTINATION bin
        COMPONENT python_interface  # applications
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE
    )

endif()