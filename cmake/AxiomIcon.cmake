function(axiom_set_icon target)
    if(NOT WIN32)
        return()
    endif()

    if(ARGC GREATER 1)
        set(ICON_FILE ${ARGV1})
    else()
        set(ICON_FILE ${CMAKE_SOURCE_DIR}/resources/axiom.ico)
    endif()

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/icon.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/${target}_icon.rc
        @ONLY
    )

    target_sources(${target}
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/${target}_icon.rc
    )
endfunction()