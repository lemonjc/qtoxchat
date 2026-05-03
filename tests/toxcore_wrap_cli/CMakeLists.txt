add_executable(toxcore_wrap_cli
    main.cpp
)

target_link_libraries(toxcore_wrap_cli
    PRIVATE
        qtoxchat_core
)

if(WIN32)
    add_custom_command(TARGET toxcore_wrap_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_FILE_DIR:toxcore_shared> $<TARGET_FILE_DIR:toxcore_wrap_cli>
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:toxcore_wrap_cli> $<TARGET_RUNTIME_DLLS:toxcore_wrap_cli>
        COMMAND_EXPAND_LISTS
    )
endif()

if(BUILD_TESTING)
    add_test(NAME toxcore_wrap_cli_help COMMAND toxcore_wrap_cli --help)
    set_tests_properties(toxcore_wrap_cli_help PROPERTIES
        PASS_REGULAR_EXPRESSION "--bootstrap"
    )
endif()
