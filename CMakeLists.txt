cmake_minimum_required (VERSION 2.6)
project (hingybot)
enable_testing()

set (CMAKE_CXX_STANDARD 17)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)

set(EXECUTABLE_OUTPUT_PATH "out/")
set(LIBRARY_OUTPUT_PATH "out/lib/")

INCLUDE(FindPkgConfig)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_package(SDL2 COMPONENTS SDL2GFX SDL2NET SDL2IMAGE REQUIRED)

include_directories("${PROJECT_BINARY_DIR}" "src/")

set(SRCS_NOMAIN src/hingy_math.cpp
  src/main.cpp src/driver.cpp src/hingy_track.cpp
  src/torcs_integration.cpp src/utils.cpp)

set(SRCS ${SRCS_NOMAIN} src/main.cpp)

add_executable(hingybot ${SRCS})

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS} ${SDL2NET_INCLUDE_DIRS} ${SDL2GFX_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(hingybot ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES} ${SDL2NET_LIBRARIES} ${SDL2GFX_LIBRARIES})

include_directories (${Boost_INCLUDE_DIRS})

file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/*.cpp)

foreach(testSrc ${TEST_SRCS})
  get_filename_component(testName ${testSrc} NAME_WE)
  set(testName "test_${testName}")
  message(----)
  message("${testSrc} ${SRCS_NOMAIN}")
  add_executable(${testName} "${testSrc};${SRCS_NOMAIN}")
  target_link_libraries(${testName} ${Boost_LIBRARIES} ${SDL2_LIBRARIES} ${SDL2GFX_LIBRARIES} ${SDL2NET_LIBRARIES} ${SDL2IMAGE_LIBRARIES} ${SDL2TTF_LIBRARIES})

  set_target_properties(${testName} PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/out/tests)

  add_test(NAME ${testName} 
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/out/tests/${testName} )
endforeach(testSrc)