add_library(AxiomUtilities 
    utilities.cpp
)

target_include_directories(AxiomUtilities
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine
)

target_link_libraries(AxiomUtilities 
    PUBLIC
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

target_compile_options(AxiomUtilities PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)