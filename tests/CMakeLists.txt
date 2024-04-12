INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/qlibc)
SET(test_list
  test_qstring
  test_qhashtbl
  test_qhasharr
  test_qhasharr_darkdh
  test_qtreetbl
  test_qlist
  test_qvector
  test_qqueue
  test_qstack
  test_qhash
)

SET(test_file_list
  test_qhash_data_1.bin
  test_qhash_data_2.bin
  test_qhash_data_3.bin
  test_qhash_data_4.bin
)

# build test
FOREACH(element IN LISTS test_list)
  MESSAGE(STATUS "set build test: ${element}")
  ADD_EXECUTABLE(${element} ${element}.c)
  TARGET_LINK_LIBRARIES(${element} qlibc qlibcext)
ENDFOREACH()

# copy test file
FOREACH(element IN LISTS test_file_list)
  ADD_CUSTOM_TARGET(cpoy_${element} ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/${element}
      ${CMAKE_CURRENT_BINARY_DIR}/${element}
    COMMENT "Copying ${element} to ${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
  )
ENDFOREACH()

# run test
FOREACH(element IN LISTS test_list)
  MESSAGE(STATUS "set run test: ${element}")
  ADD_TEST(NAME ${element} COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/launcher.sh ${element})
ENDFOREACH()
