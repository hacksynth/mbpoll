enable_language(CXX)

find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6Widgets_FOUND)
  message(STATUS "Qt6 Widgets not found - mbpoll-desktop target disabled")
  return()
endif()

add_executable(mbpoll-desktop
  main.cpp
  mainwindow.cpp
  mainwindow.h)

set_target_properties(mbpoll-desktop PROPERTIES
  AUTOMOC ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON)

if(WIN32)
  set_target_properties(mbpoll-desktop PROPERTIES WIN32_EXECUTABLE ON)
endif()

target_include_directories(mbpoll-desktop PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR})

target_link_libraries(mbpoll-desktop PRIVATE
  Qt6::Widgets
  mbpoll-common)

if(MSVC)
  target_compile_definitions(mbpoll-desktop PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(mbpoll-desktop PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wformat-security
    -Wno-unused-parameter)
endif()

install(TARGETS mbpoll-desktop
  RUNTIME DESTINATION bin
  PERMISSIONS ${PROGRAM_PERMISSIONS})
