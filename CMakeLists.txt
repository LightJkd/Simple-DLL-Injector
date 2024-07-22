cmake_minimum_required(VERSION 3.16)
project(Injector)

set(CMAKE_CXX_STANDARD 17)

add_executable(Injector main.cpp)

target_link_libraries(Injector wininet)
