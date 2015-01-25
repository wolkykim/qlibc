# ##### #
# ctest #
# ##### #
ENABLE_TESTING()

SET(TEST_DIR ${qlibc_SOURCE_DIR}/tests)
SET(TEST_PATHS ${TEST_DIR}/qstring.c ${TEST_DIR}/qcount.c)
SET(BUILD_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Copy test fixtures into test/ directory as well
FILE(COPY ${TEST_DIR}/fixtures/ DESTINATION ${BUILD_TEST_DIR}/fixtures/)

# For every .c file in test dir, compile and add test
FILE(GLOB tests "${TEST_DIR}/*.c")
FOREACH(test ${tests})
    GET_FILENAME_COMPONENT(name_without_extension ${test} NAME_WE)
    ADD_EXECUTABLE(${name_without_extension} ${test})
    TARGET_LINK_LIBRARIES(${name_without_extension} qlibc)
    ADD_TEST(NAME ${name_without_extension}
             WORKING_DIRECTORY ${BUILD_TEST_DIR}
             COMMAND ${BUILD_TEST_DIR}/${name_without_extension})
ENDFOREACH()
