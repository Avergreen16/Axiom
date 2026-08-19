add_library(AxiomUtilities STATIC
    utilities.cpp
)

target_include_directories(AxiomUtilities
    PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/engine
        ${AXIOM_ROOT}/external/glm
)

message(STATUS "GLM = ${AXIOM_ROOT}/external/glm")

target_link_libraries(AxiomUtilities 
    PUBLIC
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

target_compile_options(AxiomUtilities PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)