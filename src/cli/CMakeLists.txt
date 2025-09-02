add_executable(ssrc cli.cpp)
target_link_libraries(ssrc shibatchsrc ${SLEEF_LIBRARIES})
target_compile_options(ssrc PRIVATE ${OpenMP_CXX_FLAGS})
target_link_options(ssrc PRIVATE ${OpenMP_CXX_FLAGS})
target_include_directories(ssrc PRIVATE ${OpenMP_CXX_INCLUDE_DIRS})
target_include_directories(ssrc PRIVATE "${PROJECT_SOURCE_DIR}/src/libshibatchsrc")

add_executable(scsa scsa.cpp)
target_link_libraries(scsa shibatchsrc ${SLEEF_LIBRARIES})
target_compile_options(scsa PRIVATE ${OpenMP_CXX_FLAGS})
target_link_options(scsa PRIVATE ${OpenMP_CXX_FLAGS})
target_include_directories(scsa PRIVATE ${OpenMP_CXX_INCLUDE_DIRS})
target_include_directories(scsa PRIVATE "${PROJECT_SOURCE_DIR}/src/libshibatchsrc")

install(
  TARGETS ssrc scsa
  DESTINATION "${INSTALL_BINDIR}"
  COMPONENT Runtime
)
