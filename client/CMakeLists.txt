set(CLIENT_SOURCES
    main.cpp
    DeviceClient.cpp
)

set(CLIENT_HEADERS
    DeviceClient.h
)

add_executable(NPOKalibri_Client
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
)

target_link_libraries(NPOKalibri_Client
    PRIVATE
        Qt6::Core
        Qt6::Network
)

target_include_directories(NPOKalibri_Client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS NPOKalibri_Client RUNTIME DESTINATION bin)
