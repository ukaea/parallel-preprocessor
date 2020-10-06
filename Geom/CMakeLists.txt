# Geom module for PPP
if(PPP_USE_TYPE)
    add_definitions(-DPPP_BUILD_TYPE=1)  # c++ type reflection support
    link_libraries(MyBase)
endif()

# lots of source file are made as header-only for rapid prototyping
set(MyGeom_SOURCES 
    "OccUtils.cpp"
    #"GeometryFixer.cpp"
    "CollisionDetector.cpp"
    "Geom.cpp"
    "OpenCascadeAll.cpp"
#    "ShapeUpgrade_UnifyCoincidentDomain.cxx"
)

# can we have more than one targets, yes!
add_library(MyGeom SHARED ${MyGeom_SOURCES})
target_link_libraries(MyGeom MyApp)
add_dependencies(MyGeom MyApp)  # can this target be found?

# Geom module extracted from salome project, it is a git submodule, LGPL 2.1
target_link_libraries(MyGeom GEOMAlgo)
#target_include_directories(MyGeom PUBLIC "${PROJECT_SOURCE_DIR}/third-party/SGeom/src/GEOMAlgo")

if(WIN32)
    target_compile_definitions(MyGeom PRIVATE GEOM_DLL_EXPORT=1)  # DLL export on windows
endif()

set_target_properties(MyGeom PROPERTIES OUTPUT_NAME "pppGeom")
#-march=native has no big difference in performance
if(MSVC)
    target_compile_options(MyGeom PRIVATE /wd4996 /wd4251)
else()
    # OCCT has lots of error msg for -Wconversion, so it is turned off
    target_compile_options(MyGeom PRIVATE  -Wall -Wextra -flto)
    #  remove this flag for the moment `-Werror`
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_options(MyGeom PRIVATE  -flto)  # clang build for cmake 3.13+
endif()
########################## main ##############################
add_executable(MyGeomMain "GeometryMain.cpp")
add_dependencies(MyGeomMain MyGeom)

target_link_libraries(MyGeomMain MyGeom)
set_target_properties(MyGeomMain PROPERTIES OUTPUT_NAME "geomPipeline")

if(MSVC)
    target_compile_options(MyGeomMain PRIVATE /wd4996 /wd4251)
else()
    # OCCT has lots of error msg for -Wconversion, so it is turned off
    target_compile_options(MyGeomMain PRIVATE  -Wall -Wextra -flto)
    #  remove this flag for the moment `-Werror`
endif()


############################################
if(PPP_USE_WEB)
    set(PPP_USE_WEBSOCKETS ON)
endif()

if(PPP_USE_GUI)
    add_subdirectory(Gui)
endif()


# /usr/lib/x86_64-linux-gnu/libTKXDESTEP.so.7  link to libTKV3d.so
target_link_libraries(MyGeom ${OCC_LIBS}) # list should be used without quote


# debugging print, leave it for future usage
if(OFF)
    # cmake underscore is not printed into console, due to vscode font issue
    message("============= debug print my app LIBS===================\n")
    #message(${OCC_LIBS})  # This does not print list properly
    foreach(X IN LISTS OCC_LIBS)
        message(STATUS "${X}")
    endforeach()
    message("============================================================\n")

    set(OCC_LIBS_FULL_PATH "")
    foreach(X IN LISTS OCC_LIBS)
        list(APPEND OCC_LIBS_FULL_PATH
            ${OCC_LIBRARY_DIR}/lib{X}.so
        )
       message(STATUS ${OCC_LIBRARY_DIR}/lib{X}.so)
    endforeach()
endif()

#########################################################
# data installation and in build folder testing

# # run after the target building task is completed
# add_custom_command(TARGET <kernel-target> POST_BUILD
#     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/performCrc32.py
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#     COMMENT "Running CRC32 check..."
# )

##################### install ###########################
file(GLOB GEOM_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

install(FILES ${GEOM_HEADERS}   # do not put header list in double quote
  DESTINATION include/ppp/Geom
  COMPONENT headers
)

install(TARGETS MyGeom
  RUNTIME DESTINATION bin
  #PUBLIC_HEADER DESTINATION include/ppp/Geom
  LIBRARY DESTINATION lib
  COMPONENT libraries
)

install(TARGETS MyGeomMain
  RUNTIME DESTINATION bin
  COMPONENT applications
)

