cmake_minimum_required(VERSION 2.8)
project(xfsx CXX)

set(XFSX_VERSION "0.5.1" CACHE STRING "Library version")
set(XFSX_DATE "2015-06-30" CACHE STRING "Date of last significant change")

# guard from super-projects, i.e. when it is added as subdirectory
IF(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  include (${CMAKE_SOURCE_DIR}/cmake/modules/coverage.cmake)
  include (${CMAKE_SOURCE_DIR}/cmake/modules/sanitize.cmake)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
    message(FATAL_ERROR "You need at least version 4.8 when compiling with GCC")
  endif()
endif()

# guard from super-projects, i.e. when it is added as subdirectory
IF(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # Remove existing -Wall in debug flags because we unconditionally supply
  # -Wall and we don't want a trailing -Wall re-enable the silenced
  # warnings
  string(REPLACE "-Wall" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-local-typedefs")
  # Without that clang issues some questionable warnings
  # See also:
  # http://stackoverflow.com/questions/13905200/is-it-wise-to-ignore-gcc-clangs-wmissing-braces-warning
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstrict-aliasing -Wstrict-aliasing")
endif()

find_package(Boost 1.58
  COMPONENTS
    system # needed by filesystem, log
    filesystem
    regex
    unit_test_framework
  REQUIRED)

# guard from super-projects, i.e. when it is added as subdirectory
IF(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_subdirectory(libixxx)
  add_subdirectory(libixxxutil)
  add_subdirectory(libxxxml)
  add_subdirectory(libgrammar)
  set(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS true)
  add_subdirectory(cppformat)
  set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS})
endif()


configure_file(config.cc.in ${CMAKE_BINARY_DIR}/config.cc)

set(LIB_SRC 
  xfsx/xfsx.cc
  xfsx/hex.cc
  xfsx/bcd.cc
  xfsx/ber2xml.cc
  xfsx/ber2lxml.cc
  xfsx/xml2lxml.cc
  xfsx/ber2ber.cc
  xfsx/xml2ber.cc
  xfsx/value.cc
  xfsx/comment.cc
  xfsx/xml.cc
  xfsx/byte.cc
  xfsx/ber_node.cc
  xfsx/integer.cc
  ${CMAKE_BINARY_DIR}/config.cc
  xfsx/tap.cc
  xfsx/ber_writer.cc
  xfsx/s_pair.cc
  xfsx/ber_writer_arguments.cc
  xfsx/lxml2ber.cc
  xfsx/xml_writer_arguments.cc
  xfsx/detector.cc
  xfsx/character.cc
  xfsx/path.cc
  xfsx/tap/traverser.cc
  )
add_library(xfsx SHARED
  ${LIB_SRC}
  )
target_link_libraries(xfsx
  #${Boost_FILESYSTEM_LIBRARY}
  #${Boost_SYSTEM_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${XML2_LIB}
  cppformat
  grammar
  ixxxutil
  ixxx
  xxxml
  )
add_library(xfsx_static STATIC
  ${LIB_SRC}
  )

# under windows shared/static libraries have the same extension ...
if(${UNIX})
  set_target_properties(xfsx_static PROPERTIES OUTPUT_NAME xfsx)
endif()

IF(${CMAKE_PROJECT_NAME} STREQUAL "xfsx")

  add_executable(ut
    test/main.cc
    test/xfsx.cc
    test/ber2xml.cc
    test/ber2ber.cc
    test/xml2ber.cc
    test/integer.cc
    test/bcd.cc
    test/hex.cc
    test/test.cc
    test/comment.cc
    test/xml.cc
    test/byte.cc
    test/bed/command.cc
    test/bed/command/compute_aci.cc
    test/detector.cc
    test/character.cc
    test/xml2lxml.cc
    test/tap.cc
    test/path.cc
    test/xfsx/tap/traverser.cc

    bed/arguments.cc
    bed/command.cc
    bed/command/arguments.cc
    bed/command/ber_commands.cc
    bed/command/compute_aci.cc
    bed/command/edit.cc

    ${CMAKE_SOURCE_DIR}/cppformat/format.cc
  )
  set_property(TARGET ut PROPERTY INCLUDE_DIRECTORIES
    ${XML2_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libgrammar
    ${CMAKE_CURRENT_SOURCE_DIR}/libixxx
    ${CMAKE_CURRENT_SOURCE_DIR}/libixxxutil
    ${CMAKE_CURRENT_SOURCE_DIR}/libxxxml
  )

  target_link_libraries(ut
    xfsx_static
    grammar_static
    ixxxutil_static
    ixxx_static
    xxxml_static
    # using own main, thus, we don't need:
    # ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    ${XML2_LIB}
  )

endif() # CMAKE_PROJECT_NAME

add_executable(bed
  bed/main.cc
  bed/arguments.cc
  bed/command.cc
  bed/command/arguments.cc
  bed/command/ber_commands.cc
  bed/command/compute_aci.cc
  bed/command/edit.cc
  
  ${CMAKE_SOURCE_DIR}/cppformat/format.cc
  )

set_property(TARGET xfsx xfsx_static bed PROPERTY INCLUDE_DIRECTORIES
  ${XML2_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/libixxx
  ${CMAKE_SOURCE_DIR}/libixxxutil
  ${CMAKE_SOURCE_DIR}/libxxxml
  ${CMAKE_SOURCE_DIR}/libgrammar
)

target_link_libraries(bed
  xfsx_static
  grammar_static
  ixxxutil_static
  ixxx_static
  xxxml_static
  ${XML2_LIB}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_REGEX_LIBRARY}
)

add_executable(bcd_speed
  test/bcd_speed.cc
)
set_property(TARGET bcd_speed PROPERTY INCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/libixxx
  ${CMAKE_CURRENT_SOURCE_DIR}/libixxxutil
)
target_link_libraries(bcd_speed
  ixxxutil_static
  ixxx_static
  xfsx_static
)

# guard from super-projects, i.e. when it is added as subdirectory
IF(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_custom_target(check COMMAND ut)
endif()


