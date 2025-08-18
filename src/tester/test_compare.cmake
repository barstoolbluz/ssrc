execute_process(
  COMMAND "${TARGET_FILE_ssrc}" --dither 99 --pdf 0 10000 --genSweep 44100 2 100000 0 0 --rate 44100 --bits 16 "${TMP_DIR_PATH}/noise.44100.wav"
  COMMAND "${TARGET_FILE_ssrc}" --dither 99 --pdf 0 10000 --genSweep 48000 2 100000 0 0 --rate 48000 --bits 16 "${TMP_DIR_PATH}/noise.48000.wav"
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
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
