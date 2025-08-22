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

set(TMP_DIR_PATH ${EXECUTABLE_OUTPUT_PATH})

add_custom_command(
  OUTPUT ${TMP_DIR_PATH}/noise.44100.wav
  COMMAND $<TARGET_FILE:ssrc> --dither 99 --pdf 0 10000 --genSweep 44100 2 100000 0 0 --rate 44100 --bits 16 "${TMP_DIR_PATH}/noise.44100.wav"
  COMMAND $<TARGET_FILE:ssrc> --dither 99 --pdf 0 10000 --genSweep 48000 2 100000 0 0 --rate 48000 --bits 16 "${TMP_DIR_PATH}/noise.48000.wav"
  COMMAND $<TARGET_FILE:ssrc> --genSweep 44100 2 441000 10000 10000 --rate 44100 --bits -64 "${TMP_DIR_PATH}/sin10k.44100.wav"
  COMMAND $<TARGET_FILE:ssrc> --genSweep 48000 2 480000 10000 10000 --rate 48000 --bits -64 "${TMP_DIR_PATH}/sin10k.48000.wav"
  COMMAND $<TARGET_FILE:ssrc> --genImpulse 44100 1 50000 "${TMP_DIR_PATH}/impulse.44100.wav"
  COMMAND $<TARGET_FILE:ssrc> --genImpulse 48000 1 50000 "${TMP_DIR_PATH}/impulse.48000.wav"
  COMMENT "Generating noise.*.wav"
)

add_custom_target(
  generate_test_data
  DEPENDS ${TMP_DIR_PATH}/noise.44100.wav
)

add_dependencies(test_cppapi generate_test_data)

add_test(NAME test_api COMMAND "${CMAKE_COMMAND}"
  -D TARGET_FILE_ssrc=$<TARGET_FILE:ssrc>
  -D TARGET_FILE_test_cppapi=$<TARGET_FILE:test_cppapi>
  -D TARGET_FILE_test_soxrapi=$<TARGET_FILE:test_soxrapi>
  -D TMP_DIR_PATH=${TMP_DIR_PATH}
  -P ${CMAKE_CURRENT_LIST_DIR}/test_api.cmake
)

foreach(PROFILE standard long high insane)
  add_test(NAME test_sin10k_44100_48000_${PROFILE} COMMAND "${CMAKE_COMMAND}"
    -D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--profile\;${PROFILE}\;--rate\;48000\;--bits\;-64\;${TMP_DIR_PATH}/sin10k.44100.wav\;${TMP_DIR_PATH}/sin10k.44100.48000.${PROFILE}.wav
    -D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--check\;${CMAKE_CURRENT_LIST_DIR}/10kHz-140dB.scsa\;${TMP_DIR_PATH}/sin10k.44100.48000.${PROFILE}.wav\;100000\;460000\;10000
    -P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
  )

  add_test(NAME test_sin10k_48000_44100_${PROFILE} COMMAND "${CMAKE_COMMAND}"
    -D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--profile\;${PROFILE}\;--rate\;44100\;--bits\;-64\;${TMP_DIR_PATH}/sin10k.48000.wav\;${TMP_DIR_PATH}/sin10k.48000.44100.${PROFILE}.wav
    -D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--check\;${CMAKE_CURRENT_LIST_DIR}/10kHz-140dB.scsa\;${TMP_DIR_PATH}/sin10k.48000.44100.${PROFILE}.wav\;100000\;420000\;10000
    -P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
  )
endforeach()

foreach(PROFILE lightning fast short)
  add_test(NAME test_sin10k_44100_48000_${PROFILE} COMMAND "${CMAKE_COMMAND}"
    -D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--profile\;${PROFILE}\;--rate\;48000\;--bits\;-64\;${TMP_DIR_PATH}/sin10k.44100.wav\;${TMP_DIR_PATH}/sin10k.44100.48000.${PROFILE}.wav
    -D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--check\;${CMAKE_CURRENT_LIST_DIR}/10kHz-100dB.scsa\;${TMP_DIR_PATH}/sin10k.44100.48000.${PROFILE}.wav\;100000\;460000\;10000
    -P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
  )

  add_test(NAME test_sin10k_48000_44100_${PROFILE} COMMAND "${CMAKE_COMMAND}"
    -D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--profile\;${PROFILE}\;--rate\;44100\;--bits\;-64\;${TMP_DIR_PATH}/sin10k.48000.wav\;${TMP_DIR_PATH}/sin10k.48000.44100.${PROFILE}.wav
    -D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--check\;${CMAKE_CURRENT_LIST_DIR}/10kHz-100dB.scsa\;${TMP_DIR_PATH}/sin10k.48000.44100.${PROFILE}.wav\;100000\;420000\;10000
    -P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
  )
endforeach()

foreach(FS 44100 48000)
  foreach(BITDEPTH 8 16)
    foreach(DITHERTYPE 0 6 98 99)
      add_test(NAME test_sin10k_${FS}i${BITDEPTH}_dither_${DITHERTYPE} COMMAND "${CMAKE_COMMAND}"
	-D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--genSweep\;${FS}\;1\;480000\;10000\;10000\;--rate\;${FS}\;--bits\;${BITDEPTH}\;--dither\;${DITHERTYPE}\;${TMP_DIR_PATH}/sin10k.${FS}i${BITDEPTH}_d${DITHERTYPE}.wav
	-D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--svgout\;${TMP_DIR_PATH}/error.svg\;--check\;${CMAKE_CURRENT_LIST_DIR}/sin10k_${FS}i${BITDEPTH}_d${DITHERTYPE}.scsa\;--log2dftlen\;16\;${TMP_DIR_PATH}/sin10k.${FS}i${BITDEPTH}_d${DITHERTYPE}.wav\;200000
	-P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
      )
    endforeach()
  endforeach()
endforeach()

add_test(NAME test_impulse_48000_44100_insane COMMAND "${CMAKE_COMMAND}"
  -D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--profile\;insane\;--bits\;-64\;--rate\;44100\;${TMP_DIR_PATH}/impulse.48000.wav\;${TMP_DIR_PATH}/impulse.insane.48000.44100.wav
  -D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--check\;${CMAKE_CURRENT_LIST_DIR}/impulse.48000.44100.scsa\;--log2dftlen\;16\;${TMP_DIR_PATH}/impulse.insane.48000.44100.wav\;90000
  -P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
)

add_test(NAME test_impulse_48000_44100_standard COMMAND "${CMAKE_COMMAND}"
  -D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--profile\;standard\;--bits\;-64\;--rate\;44100\;${TMP_DIR_PATH}/impulse.48000.wav\;${TMP_DIR_PATH}/impulse.standard.48000.44100.wav
  -D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--check\;${CMAKE_CURRENT_LIST_DIR}/impulse.48000.44100.scsa\;--log2dftlen\;16\;${TMP_DIR_PATH}/impulse.standard.48000.44100.wav\;51000
  -P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
)

add_test(NAME test_impulse_44100_48000_insane COMMAND "${CMAKE_COMMAND}"
  -D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--profile\;insane\;--bits\;-64\;--rate\;48000\;${TMP_DIR_PATH}/impulse.44100.wav\;${TMP_DIR_PATH}/impulse.insane.44100.48000.wav
  -D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--check\;${CMAKE_CURRENT_LIST_DIR}/impulse.44100.48000.scsa\;--log2dftlen\;16\;${TMP_DIR_PATH}/impulse.insane.44100.48000.wav\;94000
  -P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
)

add_test(NAME test_impulse_44100_48000_standard COMMAND "${CMAKE_COMMAND}"
  -D COMMAND0_TO_EXECUTE=$<TARGET_FILE:ssrc>\;--profile\;standard\;--bits\;-64\;--rate\;48000\;${TMP_DIR_PATH}/impulse.44100.wav\;${TMP_DIR_PATH}/impulse.standard.44100.48000.wav
  -D COMMAND1_TO_EXECUTE=$<TARGET_FILE:scsa>\;--check\;${CMAKE_CURRENT_LIST_DIR}/impulse.44100.48000.scsa\;--log2dftlen\;16\;${TMP_DIR_PATH}/impulse.standard.44100.48000.wav\;53000
  -P ${CMAKE_CURRENT_LIST_DIR}/execute_two_commands.cmake
)
