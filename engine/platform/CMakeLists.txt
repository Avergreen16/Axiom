add_library(AxiomPlatform
    platform.hpp
)

message(STATUS "WIN32 = ${WIN32}")

if(WIN32)
    target_sources(AxiomPlatform PUBLIC
        platform-windows.cpp
    )
elseif(APPLE)
    target_sources(AxiomPlatform PUBLIC
    
    )
elseif(UNIX)
    target_sources(AxiomPlatform PUBLIC
        platform-unix.cpp
    )
endif()

target_link_libraries(AxiomPlatform
    PUBLIC
        glad
        glfw
        glm

        atomic
)

if(WIN32)
    target_link_libraries(AxiomPlatform
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
    target_link_libraries(AxiomPlatform
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

target_compile_options(AxiomPlatform PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)