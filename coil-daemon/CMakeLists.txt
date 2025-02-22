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
