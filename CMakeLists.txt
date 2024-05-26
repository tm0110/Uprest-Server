cmake_minimum_required(VERSION 3.5)
project(uprest-server)
add_executable(uprest-server main.cpp Server.cpp Modules.cpp)
target_link_libraries(uprest-server pthread)
