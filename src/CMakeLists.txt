add_subdirectory("libshibatchsrc")
add_subdirectory("cli")
if (BUILD_TESTS)
  add_subdirectory("tester") # tester has to come at last
endif()
