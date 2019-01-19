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

