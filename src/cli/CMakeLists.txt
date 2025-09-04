add_executable(ssrc cli.cpp)
target_link_libraries(ssrc shibatchsrc ${SLEEF_LIBRARIES})
target_include_directories(ssrc PRIVATE "${PROJECT_SOURCE_DIR}/src/libshibatchsrc")

add_executable(scsa scsa.cpp)
target_link_libraries(scsa shibatchsrc ${SLEEF_LIBRARIES})
target_include_directories(scsa PRIVATE "${PROJECT_SOURCE_DIR}/src/libshibatchsrc")

install(
  TARGETS ssrc scsa
  DESTINATION "${INSTALL_BINDIR}"
  COMPONENT Runtime
)
