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

    )
endif()

target_link_libraries(AxiomPlatform
    PUBLIC
        glad
        glfw
        glm
    
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

target_compile_options(AxiomPlatform PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)