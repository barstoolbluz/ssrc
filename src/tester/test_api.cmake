if (NOT WIN32)
  execute_process(
    COMMAND "${TARGET_FILE_ssrc}" --rate 48000 --bits 24 --stdout "${TMP_DIR_PATH}/noise.44100.wav"
    OUTPUT_FILE "${TMP_DIR_PATH}/noise.ssrc.44100.48000.24.wav"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )
  execute_process(
    COMMAND "${TARGET_FILE_ssrc}" --rate 44100 --bits 24 --stdin "${TMP_DIR_PATH}/noise.ssrc.48000.44100.24.wav"
    INPUT_FILE "${TMP_DIR_PATH}/noise.48000.wav"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )
  execute_process(
    COMMAND "${TARGET_FILE_ssrc}" --rate 48000 --bits -32 "${TMP_DIR_PATH}/noise.44100.wav" "${TMP_DIR_PATH}/noise.ssrc.44100.48000.-32.wav"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )
  execute_process(
    COMMAND "${TARGET_FILE_ssrc}" --rate 44100 --bits -32 --stdin --stdout
    INPUT_FILE "${TMP_DIR_PATH}/noise.48000.wav"
    OUTPUT_FILE "${TMP_DIR_PATH}/noise.ssrc.48000.44100.-32.wav"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )
else()
  execute_process(
    COMMAND "${TARGET_FILE_ssrc}" --rate 48000 --bits 24 "${TMP_DIR_PATH}/noise.44100.wav" "${TMP_DIR_PATH}/noise.ssrc.44100.48000.24.wav"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )
  execute_process(
    COMMAND "${TARGET_FILE_ssrc}" --rate 44100 --bits 24 "${TMP_DIR_PATH}/noise.48000.wav" "${TMP_DIR_PATH}/noise.ssrc.48000.44100.24.wav"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )
  execute_process(
    COMMAND "${TARGET_FILE_ssrc}" --rate 48000 --bits -32 "${TMP_DIR_PATH}/noise.44100.wav" "${TMP_DIR_PATH}/noise.ssrc.44100.48000.-32.wav"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )
  execute_process(
    COMMAND "${TARGET_FILE_ssrc}" --rate 44100 --bits -32 "${TMP_DIR_PATH}/noise.48000.wav" "${TMP_DIR_PATH}/noise.ssrc.48000.44100.-32.wav"
    COMMAND_ERROR_IS_FATAL ANY
    COMMAND_ECHO STDOUT
  )
endif()

execute_process(
  COMMAND "${TARGET_FILE_test_cppapi}" "${TMP_DIR_PATH}/noise.44100.wav" "${TMP_DIR_PATH}/noise.test_cppapi.44100.48000.24.wav" 48000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_test_cppapi}" "${TMP_DIR_PATH}/noise.48000.wav" "${TMP_DIR_PATH}/noise.test_cppapi.48000.44100.24.wav" 44100
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_test_soxrapi}" "${TMP_DIR_PATH}/noise.44100.wav" "${TMP_DIR_PATH}/noise.test_soxrapi.44100.48000.-32.wav" 48000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_test_soxrapi}" "${TMP_DIR_PATH}/noise.48000.wav" "${TMP_DIR_PATH}/noise.test_soxrapi.48000.44100.-32.wav" 44100
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E compare_files "${TMP_DIR_PATH}/noise.ssrc.44100.48000.24.wav" "${TMP_DIR_PATH}/noise.test_cppapi.44100.48000.24.wav"
  COMMAND "${CMAKE_COMMAND}" -E compare_files "${TMP_DIR_PATH}/noise.ssrc.48000.44100.24.wav" "${TMP_DIR_PATH}/noise.test_cppapi.48000.44100.24.wav"
  COMMAND "${CMAKE_COMMAND}" -E compare_files "${TMP_DIR_PATH}/noise.ssrc.44100.48000.-32.wav" "${TMP_DIR_PATH}/noise.test_soxrapi.44100.48000.-32.wav"
  COMMAND "${CMAKE_COMMAND}" -E compare_files "${TMP_DIR_PATH}/noise.ssrc.48000.44100.-32.wav" "${TMP_DIR_PATH}/noise.test_soxrapi.48000.44100.-32.wav"
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
