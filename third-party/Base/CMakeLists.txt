cmake_minimum_required(VERSION 3.0)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # I am a standalone project
    # the output lib has strange SOVERSION suffix, if add_subdirectory() to a parent project
    project(TypeSystem)
    set(PROJECT_BRIEF " TypeSystem extracted from FreeCAD by Qingfeng Xia")
    #project version definition
    set(CMAKE_BUILD_TYPE Debug)

    # std can and should be applied to target only
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
    # I am called from other project with add_subdirectory().
endif()

# it is fine to use clang, but more warning.
#SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")


set(MyBase_LIB "MyBase")
set(MyBase_TEST "MyBaseTest")
set(MyBase_MODULE "MyBase")

set(MyBase_SOURCES 
    BaseClass.cpp
    Type.cpp
    #Exception.cpp  # fialed to compile
    #Factory.cpp  # this can be compiled
)
add_library(${MyBase_LIB} SHARED ${MyBase_SOURCES})
set_target_properties(${MyBase_LIB} PROPERTIES OUTPUT_NAME "pppBase")

if(WIN32)
target_compile_definitions(${MyBase_LIB} PUBLIC -DBASE_DLL_EXPORT=1)
#generate_export_header(${MyBase_LIB})  # generate lib
endif()

# This will name your output .so files "libsomething.1.0" which is pretty useful
# if apply, apply project wide
#set_target_properties(${MyBase_LIB}
#PROPERTIES
#    VERSION ${PROJECT_VERSION}
#    SOVERSION ${PROJECT_VERSION}
#)

##############################################################
add_executable(${MyBase_TEST}
    TypeTest.cpp
)
#add_dependencies(MyTypeTest, MyType)
target_link_libraries(${MyBase_TEST} ${MyBase_LIB})
set_target_properties(${MyBase_TEST} PROPERTIES OUTPUT_NAME "pppBaseTests")


# To build shared libraries in Windows, we set CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS to TRUE.
# See https://cmake.org/cmake/help/v3.4/variable/CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS.html
# See https://blog.kitware.com/create-dlls-on-windows-without-declspec-using-new-cmake-export-all-feature/
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Let's set compiler-specific flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # G++
    target_compile_options(${MyBase_LIB} PRIVATE -Wall -Wextra)
else()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(${MyBase_LIB} PRIVATE /EHsc /MTd /W2 /c)
        # Set the DLLEXPORT variable to export symbols
        target_compile_definitions(${MyBase_LIB} PRIVATE WIN_EXPORT)
    endif()
endif()

########################################################
#  automatially detected by pybind11

#add_subdirectory(pybind11)
#pybind11_add_module(${MyBase_MODULE} example.cpp)
#target_link_libraries(${MyBase_MODULE} PRIVATE ${MyBase__LIB})

######################################################

set(BASE_HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/BaseClass.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Type.h"
)
#set_target_properties(${MyBase_LIB} PROPERTIES 
#    PUBLIC_HEADER "${BASE_HEADERS}")  # must use quote, otherwise only the first one

install(FILES ${BASE_HEADERS}
    DESTINATION include/ppp/Base
    COMPONENT headers
)
  
install(TARGETS ${MyBase_LIB}
  RUNTIME DESTINATION bin  # windows dll folder
  LIBRARY DESTINATION lib  # relative to install prefix
  #PUBLIC_HEADER DESTINATION include/ppp/Base  #
  COMPONENT libraries
)

