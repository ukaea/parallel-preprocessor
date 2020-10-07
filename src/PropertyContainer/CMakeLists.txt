################################# help ################################
# it is fine to run cmake without a project() declaration, as a independent repo
# catch2 will be download automatically to the source folder
# https://github.com/catchorg/Catch2/blob/master/docs/cmake-integration.md
####################################################################

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # I am a toplevel/independent cmake

    # if not exist, download by git submodule
    if( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Catch2")
      execute_process(
          COMMAND git clone https://github.com/catchorg/Catch2
          WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      )
    endif()
    add_subdirectory(Catch2)
    message(STATUS "header-only lib catch2 has been enabled for this project")
else()
      # I am called from a parent project with add_subdirectory().
      # because `add_subdirectory()` is done by parent folder cmakelists.txt
endif()


# std can and should be applied to target only
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# auto discovery test, but still need to find source files
file(GLOB TEST_SOURCES "*est.cpp")

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(catch2_tests "") # empty var is needed, to use target_sources()
set_target_properties(catch2_tests PROPERTIES OUTPUT_NAME "propertyContainerTest")
target_sources(catch2_tests
PUBLIC
  ${TEST_SOURCES}
)

target_link_libraries(catch2_tests Catch2::Catch2)

