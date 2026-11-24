file(GLOB HEADER_FILES CONFIGURE_DEPENDS RELATIVE "${AXIOM_ROOT}/src" "widgets/*.hpp")
file(GLOB CPP_FILES CONFIGURE_DEPENDS "widgets/*.cpp")

# 2. Start the generated file content
set(GENERATED_CONTENT "#pragma once\n\n")

# 3. Loop through the globbed list and append each #include
foreach(HEADER ${HEADER_FILES})
    string(APPEND GENERATED_CONTENT "#include \"${HEADER}\"\n")
endforeach()

# 4. Write the content to a file in the build directory
file(WRITE "${AXIOM_ROOT}/src/ui/widgets/umbrella.hpp" "${GENERATED_CONTENT}")

add_library(AxiomUI STATIC
    ui.cpp
    system.cpp
    widget_base.cpp
    cursor.cpp
    text.cpp

    #[[
    widgets/panel_widget.cpp
    widgets/window_widget.cpp
    widgets/split_widget.cpp
    widgets/row_widget.cpp
    widgets/column_widget.cpp
    widgets/grid_widget.cpp
    widgets/debug_widget.cpp
    widgets/button_widget.cpp
    widgets/slider_widget.cpp
    widgets/tab_widget.cpp
    widgets/text_widget.cpp
    widgets/spacer_widget.cpp
    widgets/drop_widget.cpp
    widgets/menu_widget.cpp
    widgets/relative_widget.cpp
    ##widgets/input_box_widget.cpp
    widgets/text_box_widget.cpp
    widgets/scroll_widget.cpp
    widgets/message_widget.cpp
    widgets/match_widget.cpp
    widgets/checkbox_widget.cpp
    ]]

    font/font_base.cpp
    font/bdf.cpp
    font/font.cpp
)

target_sources(AxiomUI PRIVATE ${CPP_FILES})

target_include_directories(AxiomUI PUBLIC
    ${AXIOM_ROOT}/src
    ${AXIOM_ROOT}
)

target_link_libraries(AxiomUI PUBLIC
    AxiomCore
    AxiomRender
)

if(WEB) 

elseif(WIN32)
    target_link_libraries(AxiomUI PRIVATE
        dwmapi
        gdi32
        user32
        shell32
        opengl32
        ws2_32
        wsock32
        mingw32
        mingwex 
        glfw
        atomic
    )
elseif(UNIX)
    target_link_libraries(AxiomUI PRIVATE
        GL
        X11
        dl
        pthread
    )
endif()

target_compile_options(AxiomUI PUBLIC
    -g
    -O3
    -fpermissive
    -static
    -Wtemplate-body
    -Wchanges-meaning
    -Wnarrowing
)