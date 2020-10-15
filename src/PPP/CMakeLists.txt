message("===============================================================\n"
"Start to configure App module\n"
"===============================================================\n")

# lots of source file are made in header only for rapid prototyping
set(MyApp_SOURCES 
    "PipelineController.cpp"
    "PreCompiled.cpp"
    "Logger.cpp"   # this cpp include loguru.cpp
    #    "../third-party/loguru/loguru.cpp"
    "Utilities.cpp"
    "Context.cpp"
    # template class must be defined in header file, not cpp
    "${PROJECT_SOURCE_DIR}/third-party/mm_io.cpp"
)


# can we have more than one targets, yes!
add_library(MyApp SHARED ${MyApp_SOURCES})
if(WIN32)
    target_compile_definitions(MyApp PRIVATE APP_DLL_EXPORT=1)  # DLL export on windows
endif()


if(PPP_USE_TYPE)
    add_definitions(-DPPP_BUILD_TYPE=1)
    add_dependencies(MyApp MyBase)
    target_link_libraries(MyApp  MyBase)  # must be set on windows platform
endif()

# rename will not that affect dependency, even for python module building
set_target_properties(MyApp PROPERTIES OUTPUT_NAME "pppApp")
if(MSVC)
    target_compile_options(MyApp PRIVATE  /wd4251 /wd4275 )
else()
    target_compile_options(MyApp PRIVATE  -Wall -Wextra -flto -Wconversion -Wsuggest-override)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_options(MyApp PRIVATE  -flto)  # for cmake 3.13+

    # LTO is needed for Clang compiler, `CheckIPOSupported` also need cmake 3.9+
    # include(CheckIPOSupported)
    # check_ipo_supported(RESULT supported OUTPUT error)

    # if( supported )
    #     message(STATUS "IPO / LTO enabled")
    #     set_property(TARGET MyApp PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    # else()
    #     message(STATUS "IPO / LTO not supported: <${error}>")
    # endif()

endif()


message("====================== C++ filesystem linkage setup ======================\n")
## this code block need extra care in the future
## ubuntu 20.04 g++ 9 does not need to link with `stdc++fs`
if((${CMAKE_CXX_STANDARD} GREATER 17)  OR  (${CMAKE_CXX_STANDARD} EQUAL 17))
    if(NOT APPLE)  # on MacOS, g++ still use clang's libc++ lib
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"  OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            #find_library(stdcpp_fs_lib stdc++fs)  # does not work

            message("=== linked to stdc++fs on Linux platform for std::experimental::filesystem === ")
            target_link_libraries(MyApp stdc++fs)  # this seems only for Linux and GCC before full C++17 support
        endif()
    else()
        message("=== we should not get here for g++ on Linux  `NOT APPLE` and GNU === ")
    endif()
else()
    message("=== c++17 is not enabled, using boost::filesystem instead === ")
    find_package(Boost REQUIRED COMPONENTS   filesystem)
    target_link_libraries(MyApp  Boost::filesystem)
endif()
message("======================== end of C++ filesystem linkage  setup =================\n")

################################################################
# optional dep,  boost::process dep on boost::system
if(NOT WIN32)  # tmp disable on windows,  CommandLineProcessor class
    find_package(Boost COMPONENTS system)
    if(Boost_FOUND)
        if (Boost_VERSION VERSION_LESS 106900)
            # header only, linkage is not needed for boost >= 1.69
            target_link_libraries(MyApp Boost::system)
        endif()
    endif()
endif()


find_package(Threads)
# -pthread  option is not portable, while cmake 's ${CMAKE_THREAD_LIBS_INIT} is portabble
target_link_libraries(MyApp ${CMAKE_THREAD_LIBS_INIT}) # For pthreads
if(NOT WIN32)  # 32 and 64bit windows
    target_link_libraries(MyApp dl) # For `-ldl`  used by loguru
endif()

if(PPP_USE_TYPE)
    link_libraries(MyBase)
endif()

# becareful, all plural DIRS and LIBRARIES are per language, such as MPI_<lang>_COMPILER_FLAGS
if(PPP_USE_MPI)
    find_package(MPI)
    if(NOT MPI_CXX_FOUND)
        message("===============================================================\n"
                "MPI not found, make sure OpenMPI has been installed\n"
                "===============================================================\n")
    else()
        #add_definitions(-DPPP_BUILD_MPI=1)
        include_directories(${MPI_CXX_INCLUDE_DIRS})
        link_directories(${MPI_CXX_LIBRARY_DIRS})
        link_libraries(${MPI_CXX_LIBRARIES})  # full pathes for shared libraries
    endif()
endif()

############################################################
# unit test module
############################################################
if(PPP_USE_TEST)
    add_subdirectory(CoreTests)
endif()


# header install, binded with other library object
file(GLOB APP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)  # sep by new line

install(FILES ${APP_HEADERS}
  DESTINATION include/ppp/App
  COMPONENT headers
)

install(TARGETS MyApp
  RUNTIME DESTINATION bin  # windows dll must be in bin folder
  #PUBLIC_HEADER DESTINATION include/ppp/App
  LIBRARY DESTINATION lib
  COMPONENT libraries
  )

## Test binary will not be installed