# https://github.com/catchorg/Catch2/blob/master/docs/cmake-integration.md
# catch2 must be a subfolder, so `add_subdirectory` is done in toplevel cmakelists.txt

# if C++17 is not available, then conditionally turn off one test
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)


#########################################
find_package(Threads)
# NOTE: -pthread  option is not portable
# target_link_libraries(MyApp ${CMAKE_THREAD_LIBS_INIT}) # portable

#file(GLOB TEST_SOURCES "*est*.cpp")  # not recommended way to find source
set(APP_TEST_SOURCES 
    "UtilitiesTest.cpp"
    "test_main.cpp"
)

add_executable(app_tests ${APP_TEST_SOURCES}) 
set_target_properties(app_tests PROPERTIES OUTPUT_NAME "pppAppTests")
target_link_libraries(app_tests ${CMAKE_THREAD_LIBS_INIT}) # For pthreads, portable

target_link_libraries(app_tests MyBase)
target_link_libraries(app_tests MyApp)
target_link_libraries(app_tests Catch2::Catch2)


############################# ###########################
set(PARALLEL_TEST_SOURCES 
  "ParallelAccessorTest.cpp"
)

add_executable(parallel_tests ${PARALLEL_TEST_SOURCES})
set_target_properties(parallel_tests PROPERTIES OUTPUT_NAME "pppParallelTests")

target_link_libraries(parallel_tests ${CMAKE_THREAD_LIBS_INIT}) # For pthreads
add_dependencies(parallel_tests MyApp)
target_link_libraries(parallel_tests MyApp)
target_link_libraries(parallel_tests ${TBB_LIBRARIES})

if(MSVC)
    target_compile_options(parallel_tests PRIVATE  /wd4251 /wd4275 )
endif()

######################### test registration #####################
#  using contrib/Catch.cmake, ParseAndAddCatchTests.cmake
# current not working
#include(CTest)
#include(Catch)
#catch_discover_tests(foo)

