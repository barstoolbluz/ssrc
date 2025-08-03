add_library(shibatchsrc libssrc.cpp)
add_dependencies(shibatchsrc ext_sleef)

set_target_properties(shibatchsrc PROPERTIES
  VERSION ${SSRC_VERSION}
  SOVERSION ${SSRC_SOVERSION}
  PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/src/include/shibatch/ssrc.hpp
  PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/src/include/shibatch/shapercoefs.h
  )

install(
  TARGETS shibatchsrc
  DESTINATION "${INSTALL_LIBDIR}"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/shibatch"
  COMPONENT shibatch_Development
)
