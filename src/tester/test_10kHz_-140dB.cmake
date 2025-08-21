execute_process(
  COMMAND "${TARGET_FILE_ssrc}" --genSweep 44100 2 441000 10000 10000 --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.wav"
  COMMAND "${TARGET_FILE_ssrc}" --genSweep 48000 2 480000 10000 10000 --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.wav"
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_ssrc}" --profile standard --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.wav" "${TMP_DIR_PATH}/sin10k.44100.48000.standard.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile standard --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.wav" "${TMP_DIR_PATH}/sin10k.48000.44100.standard.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile long --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.wav" "${TMP_DIR_PATH}/sin10k.44100.48000.long.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile long --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.wav" "${TMP_DIR_PATH}/sin10k.48000.44100.long.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile high --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.wav" "${TMP_DIR_PATH}/sin10k.44100.48000.high.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile high --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.wav" "${TMP_DIR_PATH}/sin10k.48000.44100.high.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile insane --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.wav" "${TMP_DIR_PATH}/sin10k.44100.48000.insane.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile insane --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.wav" "${TMP_DIR_PATH}/sin10k.48000.44100.insane.wav"
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-140dB.check" "${TMP_DIR_PATH}/sin10k.44100.48000.standard.wav" 20000 460000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-140dB.check" "${TMP_DIR_PATH}/sin10k.48000.44100.standard.wav" 20000 420000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-140dB.check" "${TMP_DIR_PATH}/sin10k.44100.48000.long.wav" 20000 460000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-140dB.check" "${TMP_DIR_PATH}/sin10k.48000.44100.long.wav" 20000 420000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-140dB.check" "${TMP_DIR_PATH}/sin10k.44100.48000.high.wav" 40000 460000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-140dB.check" "${TMP_DIR_PATH}/sin10k.48000.44100.high.wav" 40000 420000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-140dB.check" "${TMP_DIR_PATH}/sin10k.44100.48000.insane.wav" 100000 460000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-140dB.check" "${TMP_DIR_PATH}/sin10k.48000.44100.insane.wav" 100000 420000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
