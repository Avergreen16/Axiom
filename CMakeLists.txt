cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)

include_directories(src include include/asio)
link_directories(lib)

project(Engine VERSION 0.0)

add_executable(Client src/client_main.cpp icon.rc)

set_target_properties(Client PROPERTIES OUTPUT_NAME "Axiom")

add_library(Glad src/glad.c)

add_library(Client_files src/wrapper.cpp src/core.cpp src/pnum.cpp src/random.cpp src/utility.cpp src/model.cpp src/ecs.cpp src/physics.cpp src/gui.cpp src/particles.cpp src/render.cpp src/input.cpp src/billboard.cpp src/voxel.cpp src/building.cpp src/client_networking.cpp)

target_link_libraries(Client Glad)

target_link_libraries(Client Client_files)

target_link_libraries(Client dwmapi glfw3 gdi32 user32 shell32 opengl32 assimp zs ws2_32 wsock32 mingw32 mingwex stdc++ atomic lua)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O3 -fpermissive -march=native -static")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -fpermissive -march=native -static")
