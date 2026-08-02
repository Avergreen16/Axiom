
target_include_directories(Game PUBLIC ${CMAKE_SOURCE_DIR}/engine)

add_subdirectory(window)
add_subdirectory(math)
add_subdirectory(ecs)
add_subdirectory(utilities)
add_subdirectory(assets)
add_subdirectory(render)
add_subdirectory(ui)
add_subdirectory(platform)
add_subdirectory(input)
add_subdirectory(scene)