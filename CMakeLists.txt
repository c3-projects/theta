cmake_minimum_required(VERSION 3.9)

project(c3-theta VERSION 0.0.1)

find_package(c3-upsilon REQUIRED)
find_package(c3-nu REQUIRED)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 17)

add_compile_options("-Wall" "-Wextra") #"-pedantic"

find_package(Threads REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE source src/*.cpp)

add_library(${PROJECT_NAME} ${source})

target_link_libraries(${PROJECT_NAME} c3-upsilon c3-nu ${CMAKE_THREAD_LIBS_INIT})

file(GLOB_RECURSE tests tests/*.cxx)

foreach(test ${tests})
  file(RELATIVE_PATH test_rel ${CMAKE_SOURCE_DIR}/tests/ ${test})
  get_filename_component(test_fname ${test_rel} NAME_WE)
  get_filename_component(test_dir ${test_rel} DIRECTORY)
  #string(REGEX REPLACE "/" "_" test_dir ${test_dir})

  set(test_name ${test_dir}_${test_fname})

  add_executable(${test_name} ${test})

  target_link_libraries(${test_name} ${PROJECT_NAME})

  add_test(${test_name} ${test_name})
endforeach()

enable_testing()

include(GNUInstallDirs)
install(
  TARGETS ${PROJECT_NAME}
  COMPONENT lib
  EXPORT ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT dev
)

install(
  EXPORT ${PROJECT_NAME}
  FILE ${PROJECT_NAME}-config.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  COMPONENT dev
)

if(NOT DEFINED CPACK_GENERATOR)
  # TODO: platform logic
  set(CPACK_GENERATOR "DEB")
endif()

if(CPACK_GENERATOR STREQUAL "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "c3-projects")
  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
  set(CPACK_DEBIAN_LIB_PACKAGE_NAME "${PROJECT_NAME}")
  set(CPACK_DEBIAN_DEV_PACKAGE_NAME "${PROJECT_NAME}-dev")
  SET(CPACK_COMPONENT_UNSPECIFIED_HIDDEN TRUE)
  set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS " \
    libstdc++-7-dev | libc++-7-dev, \
    c3-upsilon-dev (>= 0.2.0), c3-upsilon-dev (<< 0.3)\
    c3-nu-dev (>= 0.2.0), c3-nu-dev (<< 0.3) \
  ")
endif()

set(CPACK_COMPONENT_DEV_DEPENDS lib)

INCLUDE(CPack)
