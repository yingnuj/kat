function(kat_init_picotool)
  set(picotool_BUILD_TARGET picotoolBuild)
  set(picotool_TARGET picotool)
  set(picotool_INSTALL_DIR ${CMAKE_SOURCE_DIR}/tools)

  find_program(picotool_EXECUTABLE picotool
               HINTS ${CMAKE_SOURCE_DIR}/tools/picotool)
  if(NOT picotool_EXECUTABLE AND NOT TARGET picotool)
    message(
      "picotool not found - configuring to build and install picotool from source"
    )

    set(picotool_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/picotool)
    set(picotool_BINARY_DIR ${CMAKE_BINARY_DIR}/picotool/picotool-build)

    add_custom_target(
      picotoolForceReconfigure
      ${CMAKE_COMMAND} -E touch_nocreate "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
      VERBATIM)

    ExternalProject_Add(
      ${picotool_BUILD_TARGET}
      PREFIX picotool
      SOURCE_DIR ${picotool_SOURCE_DIR}
      BINARY_DIR ${picotool_BINARY_DIR}
      INSTALL_DIR ${picotool_INSTALL_DIR}
      CMAKE_ARGS "--no-warn-unused-cli"
                 "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
                 "-DPICO_SDK_PATH:FILEPATH=${PICO_SDK_PATH}"
                 "-DPICOTOOL_NO_LIBUSB=1"
                 "-DPICOTOOL_FLAT_INSTALL=1"
                 "-DCMAKE_INSTALL_PREFIX=${picotool_INSTALL_DIR}"
                 "-DCMAKE_RULE_MESSAGES=OFF" # quieten the build
                 "-DCMAKE_INSTALL_MESSAGE=NEVER" # quieten the install
      BUILD_ALWAYS 1 # force dependency checking
      EXCLUDE_FROM_ALL TRUE)
  else()
    message(
      "picotool found at ${picotool_EXECUTABLE} - skipping picotool build")
  endif()
  set(picotool_EXECUTABLE ${picotool_INSTALL_DIR}/picotool/picotool)
  add_executable(${picotool_TARGET} IMPORTED GLOBAL)
  set_property(TARGET ${picotool_TARGET} PROPERTY IMPORTED_LOCATION
                                                  ${picotool_EXECUTABLE})
  add_dependencies(${picotool_TARGET} ${picotool_BUILD_TARGET})
endfunction(kat_init_picotool)
