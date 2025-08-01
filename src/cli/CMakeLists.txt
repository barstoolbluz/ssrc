add_executable(ssrc cli.cpp)
target_link_libraries(ssrc shibatchsrc ${SLEEF_LIBRARIES})
target_compile_options(ssrc PRIVATE ${OpenMP_CXX_FLAGS})
target_link_options(ssrc PRIVATE ${OpenMP_CXX_FLAGS})
target_include_directories(ssrc PRIVATE ${OpenMP_CXX_INCLUDE_DIRS})

install(
  TARGETS ssrc
  DESTINATION "${INSTALL_BINDIR}"
  COMPONENT Runtime
)
