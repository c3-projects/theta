cmake_minimum_required(VERSION 3.9)

set(PROJECT_NAME theta)

project(theta)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 17)

#ALL THE WARNINGS
add_compile_options("-Wall" "-Wextra") #"-pedantic"

find_package(PkgConfig REQUIRED)
pkg_search_module(BOTAN REQUIRED botan-2)
include_directories(${BOTAN_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE source src/*.cpp)

#add_library(${PROJECT_NAME} ${source})
add_executable(${PROJECT_NAME} ${source})

target_link_libraries(${PROJECT_NAME} ${BOTAN_LIBRARIES})

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

