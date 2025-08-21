add_executable(test_cppapi test_cppapi.cpp)
target_link_libraries(test_cppapi shibatchsrc ${SLEEF_LIBRARIES})
target_compile_options(test_cppapi PRIVATE ${OpenMP_C_FLAGS})
target_link_options(test_cppapi PRIVATE ${OpenMP_C_FLAGS})
target_include_directories(test_cppapi PRIVATE ${OpenMP_C_INCLUDE_DIRS})

add_executable(test_soxrapi test_soxrapi.c)
target_link_libraries(test_soxrapi shibatchsrc ${SLEEF_LIBRARIES})
target_compile_options(test_soxrapi PRIVATE ${OpenMP_C_FLAGS})
target_link_options(test_soxrapi PRIVATE ${OpenMP_C_FLAGS})
target_include_directories(test_soxrapi PRIVATE ${OpenMP_C_INCLUDE_DIRS})
target_include_directories(test_soxrapi PRIVATE "${PROJECT_SOURCE_DIR}/src/libshibatchsrc")

add_test(NAME test_compare COMMAND "${CMAKE_COMMAND}"
  -D TARGET_FILE_ssrc=$<TARGET_FILE:ssrc>
  -D TARGET_FILE_test_cppapi=$<TARGET_FILE:test_cppapi>
  -D TARGET_FILE_test_soxrapi=$<TARGET_FILE:test_soxrapi>
  -D TMP_DIR_PATH=${EXECUTABLE_OUTPUT_PATH}
  -P ${CMAKE_CURRENT_LIST_DIR}/test_compare.cmake
)

add_test(NAME test_10kHz-140dB COMMAND "${CMAKE_COMMAND}"
  -D TARGET_FILE_ssrc=$<TARGET_FILE:ssrc>
  -D TARGET_FILE_scsa=$<TARGET_FILE:scsa>
  -D TARGET_FILE_test_cppapi=$<TARGET_FILE:test_cppapi>
  -D TARGET_FILE_test_soxrapi=$<TARGET_FILE:test_soxrapi>
  -D TMP_DIR_PATH=${EXECUTABLE_OUTPUT_PATH}
  -D TESTER_DIR_PATH=${CMAKE_CURRENT_LIST_DIR}
  -P ${CMAKE_CURRENT_LIST_DIR}/test_10kHz_-140dB.cmake
)

add_test(NAME test_10kHz-100dB COMMAND "${CMAKE_COMMAND}"
  -D TARGET_FILE_ssrc=$<TARGET_FILE:ssrc>
  -D TARGET_FILE_scsa=$<TARGET_FILE:scsa>
  -D TARGET_FILE_test_cppapi=$<TARGET_FILE:test_cppapi>
  -D TARGET_FILE_test_soxrapi=$<TARGET_FILE:test_soxrapi>
  -D TMP_DIR_PATH=${EXECUTABLE_OUTPUT_PATH}
  -D TESTER_DIR_PATH=${CMAKE_CURRENT_LIST_DIR}
  -P ${CMAKE_CURRENT_LIST_DIR}/test_10kHz_-100dB.cmake
)
