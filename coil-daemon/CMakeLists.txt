set(DAEMON_NAME "coild")

add_executable(${DAEMON_NAME}
    "src/main.cpp"
    "src/configParser.cpp"
    "src/dbusServer.cpp"
)

target_include_directories(${DAEMON_NAME} 
    PRIVATE "src"
)

target_link_libraries(${DAEMON_NAME} 
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE SDBusCpp::sdbus-c++
    PRIVATE spdlog
) 

# Installation command
install(TARGETS ${DAEMON_NAME} DESTINATION "usr/bin")
install(
    FILES "${CMAKE_SOURCE_DIR}/example-files/etc/coil/default.json" 
    DESTINATION "etc/coil"
)

