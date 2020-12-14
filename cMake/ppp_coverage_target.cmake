############### to use #########################################
# add essential setup, see example in project CMakeLists.txt
# and `include(this_file)` in the project CMakeLists.txt
# after compiling,  run the unit test executable to generate info
# run  `make coverage` in the build folder to generate report
##################################################################

message(STATUS "run `make coverage` in the build folder to generate coverage report")

find_program( LCOV_PATH  NAMES lcov lcov.bat lcov.exe lcov.perl)
find_program( GENHTML_PATH NAMES genhtml genhtml.perl genhtml.bat )
find_program( GCOV_PATH gcov )

add_custom_target(coverage

    # Cleanup lcov
    COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} -directory . -b ${PROJECT_SOURCE_DIR} --zerocounters

    # Create baseline to make sure untouched files show up in the report
    COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} -c -i -d . -b ${PROJECT_SOURCE_DIR} -o ${PROJECT_NAME}.base

    # Run executables
    COMMAND bash ${PROJECT_SOURCE_DIR}/scripts/run_all_tests.sh

    # Capturing lcov counters and generating report
    COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} --directory . -b ${PROJECT_SOURCE_DIR} --capture --output-file ${PROJECT_NAME}.capture

    # add baseline counters
    COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} -a ${PROJECT_NAME}.base -a ${PROJECT_NAME}.capture --output-file ${PROJECT_NAME}.total

    # filter collected data to final coverage report and merge outputs
    COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} --remove ${PROJECT_NAME}.total ${LCOV_EXCLUDES} --output-file ${PROJECT_NAME}.info

    # Generate HTML output
    COMMAND ${GENHTML_PATH} -o ${PROJECT_NAME}_coverage ${PROJECT_NAME}.info

    # Set output files as GENERATED (will be removed on 'make clean')
    BYPRODUCTS
        ${PROJECT_NAME}.base
        ${PROJECT_NAME}.capture
        ${PROJECT_NAME}.total
        ${PROJECT_NAME}.info
        ${PROJECT_NAME}  # report directory

    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    VERBATIM # Protect arguments to commands
    COMMENT "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
)
