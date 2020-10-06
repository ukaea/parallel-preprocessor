# https://github.com/catchorg/Catch2/blob/master/docs/cmake-integration.md
# catch2 must be a subfolder, so `add_subdirectory` is done in toplevel cmakelists.txt
#add_subdirectory(./Catch2)

# std can and should be applied to target only
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#########################################
find_package(Threads)
# -pthread  option is not portable
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")  # not portable
#target_link_libraries(MyApp ${CMAKE_THREAD_LIBS_INIT}) # portable

#file(GLOB TEST_SOURCES "*est*.cpp")  # not recommended way to find source
set(APP_TEST_SOURCES 
    "UtilitiesTest.cpp"
    "test_main.cpp"
)

add_executable(app_tests ${APP_TEST_SOURCES}) 
set_target_properties(app_tests PROPERTIES OUTPUT_NAME "pppAppTests")
target_link_libraries(app_tests ${CMAKE_THREAD_LIBS_INIT}) # For pthreads, portable

#add_dependencies(app_tests MyBase)
target_link_libraries(app_tests MyBase)
target_link_libraries(app_tests MyApp)
target_link_libraries(app_tests Catch2::Catch2)

######################################################
if (PPP_USE_GEOM)

  add_executable(geom_tests "GeomTests.cpp") # empty var is need, to use target_sources()
  set_target_properties(geom_tests PROPERTIES OUTPUT_NAME "pppGeomTests")

  if(PPP_USE_TYPE)
  add_dependencies(geom_tests MyBase)
  endif()

  target_link_libraries(geom_tests MyBase)
  target_link_libraries(geom_tests Catch2::Catch2)
  target_link_libraries(geom_tests ${CMAKE_THREAD_LIBS_INIT}) # portable

  # salome GEOMAlgo module
  target_link_libraries(geom_tests MyApp)
  target_link_libraries(geom_tests MyGeom)
  target_link_libraries(geom_tests GEOMAlgo)

  if(OCC_FOUND)
    target_link_libraries(geom_tests ${OCC_LIBS}) # list should be used without quote
  endif()

endif()

######################### test registration#####################
#  using contrib/Catch.cmake, ParseAndAddCatchTests.cmake
# current not working
#include(CTest)
#include(Catch)
#catch_discover_tests(foo)

##################### standalone app ###########################
if (PPP_USE_GEOM)
  if (${PPP_USE_QT})
    # relies on QT, but user may not enable QT GUI
    add_executable(OccBVHdemo "OccBVHdemo.cpp") #
    target_link_libraries(OccBVHdemo ${OCC_LIBS}) # compiled but does not work with OCCT 7.3
    target_link_libraries(OccBVHdemo ${OCC_VIZ_LIBRARIES}) # list should be used without quote
    install(TARGETS OccBVHdemo
      RUNTIME DESTINATION bin
      COMPONENT applications)
  endif()

  add_executable(UniqueIdValidator "UniqueIdValidator.cpp") #
  target_link_libraries(UniqueIdValidator ${OCC_LIBS}) # compiled but does not work with OCCT 7.3
  install(TARGETS UniqueIdValidator
    RUNTIME DESTINATION bin
    COMPONENT applications)
endif()
################## installation and packing #######################
# todo, not sure where they should go!