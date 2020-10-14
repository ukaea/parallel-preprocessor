
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

####################### standalone app #####################
add_executable(UniqueIdValidator "UniqueIdValidator.cpp") #
target_link_libraries(UniqueIdValidator ${OCC_LIBS})

install(TARGETS UniqueIdValidator
        RUNTIME DESTINATION bin
        COMPONENT applications)

##################### standalone app ###########################
if (${PPP_USE_QT})
    # relies on QT, but user may not enable QT GUI
    add_executable(OccBVHdemo "OccBVHdemo.cpp") #
    target_link_libraries(OccBVHdemo ${OCC_LIBS})
    target_link_libraries(OccBVHdemo ${OCC_VIZ_LIBRARIES}) # list should be used without quote
    install(TARGETS OccBVHdemo
        RUNTIME DESTINATION bin
        COMPONENT applications)
endif()
