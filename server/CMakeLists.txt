cmake_minimum_required(VERSION 3.20)
project(ludo_live LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CROW_ENABLE_SSL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG v1.3.4
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(crow)

add_executable(ludo_server src/main.cpp)
target_include_directories(ludo_server PRIVATE include)
target_link_libraries(ludo_server PRIVATE Crow::Crow)

enable_testing()
add_executable(ludo_engine_tests tests/test_engine.cpp)
target_include_directories(ludo_engine_tests PRIVATE include)
add_test(NAME ludo_engine_tests COMMAND ludo_engine_tests)
