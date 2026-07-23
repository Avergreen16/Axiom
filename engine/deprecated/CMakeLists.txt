add_executable(Game client_main.cpp)

# icon.rc

axiom_set_icon(Game)

set_target_properties(Game PROPERTIES OUTPUT_NAME "Axiom")

add_library(AxiomEngine
    wrapper.cpp 
    core.cpp 
    pnum.cpp 
    random.cpp 
    utility.cpp 
    model.cpp 
    ecs.cpp 
    physics.cpp 
    gui.cpp 
    particles.cpp 
    render.cpp 
    input.cpp 
    billboard.cpp 
    voxel.cpp 
    building.cpp 
)

target_link_libraries(AxiomEngine
    PUBLIC
        glad
        glfw
        glm
        assimp
        lua
        asio
        stb
        xsimd

        dwmapi
        gdi32
        user32
        shell32
        opengl32
        ws2_32
        wsock32
        mingw32
        mingwex 
        stdc++ 
        atomic
)

target_link_libraries(Game 
    PRIVATE
        AxiomEngine
)

target_compile_options(AxiomEngine PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)

target_compile_options(Game PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)