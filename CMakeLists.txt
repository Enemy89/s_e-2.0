cmake_minimum_required(VERSION 3.25)
project(search_engine VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)

set (JSON_BuildTests OFF CACHE INTERNAL "")
set(VERSION_APP ${search_engine_VERSION_MAJOR}.${search_engine_VERSION_MINOR})
add_definitions(-DVERSION_APP="${VERSION_APP}")

set(MY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(nlohmann_json)
add_executable(search_engine main.cpp ConverterJSON.h ConverterJSON.cpp InvertedIndex.h InvertedIndex.cpp SearchServer.h SearchServer.cpp)

target_link_libraries(search_engine PRIVATE nlohmann_json::nlohmann_json)