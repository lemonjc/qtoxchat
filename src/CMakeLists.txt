qt_add_executable(qtoxchat
    main.cpp
)

target_link_libraries(qtoxchat
    PRIVATE
        Qt6::Widgets
        CURL::libcurl
)

install(TARGETS qtoxchat)
