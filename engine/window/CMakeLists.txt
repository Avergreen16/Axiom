add_library(AxiomWindow STATIC
    window.cpp
)

target_include_directories(AxiomWindow
    PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/engine
        ${AXIOM_ROOT}/external/glm
)

target_link_libraries(AxiomWindow
    PUBLIC
        AxiomPlatform
        AxiomInput
        AxiomUtilities

        glad
        glfw
        glm
        assimp
        lua
        asio
        stb
        xsimd

        atomic
)

if(WIN32)
    target_link_libraries(AxiomWindow
        PRIVATE
            dwmapi
            gdi32
            user32
            shell32
            opengl32
            ws2_32
            wsock32
            mingw32
            mingwex 
            atomic
    )
elseif(UNIX)
    target_link_libraries(AxiomWindow
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

target_compile_options(AxiomWindow PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)