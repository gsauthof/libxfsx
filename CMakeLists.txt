cmake_minimum_required(VERSION 2.8)
project(xfsx CXX)

set(XFSX_VERSION "0.6.1" CACHE STRING "Library version")
set(XFSX_DATE "2016-06-11" CACHE STRING "Date of last significant change")

option(XFSX_USE_LUA  "Enable Lua based features like pretty printing" ON)
option(XFSX_USE_LUAJIT "Use Luajit" OFF)

# guard from super-projects, i.e. when it is added as subdirectory
IF(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  include (${CMAKE_SOURCE_DIR}/cmake/modules/coverage.cmake)
  include (${CMAKE_SOURCE_DIR}/cmake/modules/sanitize.cmake)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
  message(FATAL_ERROR "You need at least version 4.9 when compiling with GCC")
endif()

# guard from super-projects, i.e. when it is added as subdirectory
IF(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # Remove existing -Wall in debug flags because we unconditionally supply
  # -Wall and we don't want a trailing -Wall re-enable the silenced
  # warnings
  string(REPLACE "-Wall" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
      AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.7")
    # clang 3.6 does not know this option
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Without that clang issues some questionable warnings
    # See also:
    # http://stackoverflow.com/questions/13905200/is-it-wise-to-ignore-gcc-clangs-wmissing-braces-warning
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
  endif()
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


# output is relative to CMAKE_CURRENT_BINARY_DIR, by default
configure_file(config.cc.in config.cc)
configure_file(config.hh.in xfsx_config.hh)

message("CMAKE_SYSTEM_PREFIX_PATH: ${CMAKE_SYSTEM_PREFIX_PATH}")
message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
if (XFSX_USE_LUA)
  if(XFSX_USE_LUAJIT)
    find_library(LUA_LIB NAMES luajit-5.1)
    # e.g. Fedora 23 luajit does not come with lua.hpp
    #find_path(LUAXX_INCLUDE_DIR lua.hpp)
    find_path(LUA_INCLUDE_DIR lua.h
      PATHS /usr/include/luajit-2.0 NO_DEFAULT_PATH)
    #message(${LUA_INCLUDE_DIR})
  else() # XFSX_USE_LUAJIT
    if(WIN32)
      # on windows we use the lua included as submodule
      add_subdirectory(lua)
      set(LUA_LIB liblua)
      set(LUAXX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua/etc)
      set(LUA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua/src)
      set(LUA_INCLUDE_DIR2 ${CMAKE_CURRENT_BINARY_DIR}/lua)
    else()
      # some hints for Mac OSX ...
      find_library(LUA_LIB NAMES lua)
      find_path(LUAXX_INCLUDE_DIR lua.hpp)
      find_path(LUA_INCLUDE_DIR lua.h)
    endif()
  endif()
endif()
message("LUA_LIB: ${LUA_LIB}")
message("LUA_INCLUDE_DIR: ${LUA_INCLUDE_DIR}")
message("LUAXX_INCLUDE_DIR: ${LUAXX_INCLUDE_DIR}")

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
  xfsx/search.cc
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
  ${LUA_LIB}
  )
add_library(xfsx_static STATIC
  ${LIB_SRC}
  )

# under windows shared/static libraries have the same extension ...
if(${UNIX})
  set_target_properties(xfsx_static PROPERTIES OUTPUT_NAME xfsx)
endif()

set(BED_SRC
    bed/arguments.cc
    bed/command.cc
    bed/command/arguments.cc
    bed/command/ber_commands.cc
    bed/command/compute_aci.cc
    bed/command/edit.cc
    bed/command/write_aci.cc
  )

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
    test/bed/command/edit.cc
    test/bed/command/write_aci.cc
    test/bed/command/write_id.cc
    test/bed/command/write_def.cc
    test/bed/command/write_indef.cc
    test/bed/command/write_ber.cc
    test/bed/command/search.cc
    test/bed/command/validate.cc
    test/bed/command/write_xml.cc
    test/bed/command/mk_bash_comp.cc
    test/detector.cc
    test/character.cc
    test/xml2lxml.cc
    test/tap.cc
    test/path.cc
    test/xfsx/tap/traverser.cc
    test/xfsx/traverser/matcher.cc
    test/xfsx/search.cc

    ${BED_SRC}

    ${CMAKE_SOURCE_DIR}/cppformat/format.cc
  )
  set_property(TARGET ut PROPERTY INCLUDE_DIRECTORIES
    ${XML2_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
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
    ${LUA_LIB}
  )

endif() # CMAKE_PROJECT_NAME

add_executable(bed
  bed/main.cc
  ${BED_SRC}
  
  ${CMAKE_SOURCE_DIR}/cppformat/format.cc
  )
target_compile_definitions(bed PRIVATE -DSOL_USE_BOOST=1)
target_compile_definitions(xfsx PRIVATE -DSOL_USE_BOOST=1)
target_compile_definitions(xfsx_static PRIVATE -DSOL_USE_BOOST=1)
set_property(TARGET xfsx xfsx_static bed PROPERTY INCLUDE_DIRECTORIES
  ${XML2_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${LUAXX_INCLUDE_DIR}
  ${LUA_INCLUDE_DIR}
  ${LUA_INCLUDE_DIR2}
  ${CMAKE_CURRENT_SOURCE_DIR}
  #  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libixxx
  ${CMAKE_SOURCE_DIR}/libixxxutil
  ${CMAKE_SOURCE_DIR}/libxxxml
  ${CMAKE_SOURCE_DIR}/libgrammar
  ${CMAKE_SOURCE_DIR}/sol2
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
  ${LUA_LIB}
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

  # it is relative to build directory, by default
  # add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bed.bash
  add_custom_command(OUTPUT bed.bash
    COMMAND bed
    ARGS    mk-bash-comp -o ${CMAKE_CURRENT_BINARY_DIR}/bed.bash
    # target-level dependency is automatically added, but
    # file-level dependency is needed such that file is regenerated
    # when bed is re-built
    DEPENDS bed
    COMMENT "generating bed bash completion file"
    # we don't need a special wd
    # WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

  add_custom_target(completion DEPENDS bed.bash)

  add_custom_target(check-completion COMMAND bash bed.bash DEPENDS completion)

  add_custom_target(check-magic COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test/magic.bash ${CMAKE_CURRENT_SOURCE_DIR})
endif()


