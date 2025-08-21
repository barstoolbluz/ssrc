execute_process(
  COMMAND "${TARGET_FILE_ssrc}" --genSweep 44100 2 441000 10000 10000 --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.0.wav"
  COMMAND "${TARGET_FILE_ssrc}" --genSweep 48000 2 480000 10000 10000 --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.0.wav"
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_ssrc}" --profile lightning --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.0.wav" "${TMP_DIR_PATH}/sin10k.44100.48000.lightning.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile lightning --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.0.wav" "${TMP_DIR_PATH}/sin10k.48000.44100.lightning.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile fast --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.0.wav" "${TMP_DIR_PATH}/sin10k.44100.48000.fast.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile fast --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.0.wav" "${TMP_DIR_PATH}/sin10k.48000.44100.fast.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile short --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.0.wav" "${TMP_DIR_PATH}/sin10k.44100.48000.short.wav"
  COMMAND "${TARGET_FILE_ssrc}" --profile short --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.0.wav" "${TMP_DIR_PATH}/sin10k.48000.44100.short.wav"
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-100dB.check" "${TMP_DIR_PATH}/sin10k.44100.48000.lightning.wav" 20000 460000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-100dB.check" "${TMP_DIR_PATH}/sin10k.48000.44100.lightning.wav" 20000 420000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-100dB.check" "${TMP_DIR_PATH}/sin10k.44100.48000.fast.wav" 20000 460000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-100dB.check" "${TMP_DIR_PATH}/sin10k.48000.44100.fast.wav" 20000 420000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-100dB.check" "${TMP_DIR_PATH}/sin10k.44100.48000.short.wav" 40000 460000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
execute_process(
  COMMAND "${TARGET_FILE_scsa}" --check "${TESTER_DIR_PATH}/10kHz-100dB.check" "${TMP_DIR_PATH}/sin10k.48000.44100.short.wav" 40000 420000 10000
  COMMAND_ERROR_IS_FATAL ANY
  COMMAND_ECHO STDOUT
)
