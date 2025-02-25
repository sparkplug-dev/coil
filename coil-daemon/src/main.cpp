#include <cstdlib>

#include "dbusServer.h"
#include "spdlog/spdlog.h"

// Define the configuration path environmental variables
constexpr const char *c_base_config_env = "COIL_BASE_CONFIG";
constexpr const char* c_user_config_env = "COIL_USER_CONFIG";
// If the variable is set enable debug logging
constexpr const char* c_debug_env = "COIL_DEBUG";

// Define the configuration path default parameter
constexpr const char* c_base_config_default = "/etc/coil/default.json";
// This default path is relative to the user home folder 
constexpr const char* c_user_config_default = ".config/coil/config.json";

int main() {
    // Set the logging level
    if (std::getenv(c_debug_env))
        spdlog::set_level(spdlog::level::debug);

    // Get the configuration paths from the environment
    const char* base_path_env = std::getenv(c_base_config_env);
    const char* user_path_env = std::getenv(c_user_config_env);

    // Store the path to the configuration file
    std::string base_path;
    std::string user_path;

    // Check if the environmental variables are set,
    // if they aren't use the default
    if (base_path_env) {
        base_path = base_path_env;
    } else {
        base_path = c_base_config_default;

        spdlog::info(
            "COIL_BASE_CONFIG is not set, defaulting to: \"{}\"",
            base_path
        );
    }

    if (user_path_env) {
        user_path = user_path_env;
    } else {
        // Check if the HOME environmental variable is set and 
        // use it as base for the path.
        // If the HOME variable is not found default to /root
        if (const char* home = std::getenv("HOME")) {
            user_path = home;
        } else {
            user_path = "/root";
        }

        // Append the configuration path to the home folder
        user_path += "/";
        user_path += c_user_config_default;

        spdlog::info(
            "COIL_USER_CONFIG is not set, defaulting to: \"{}\"",
            user_path
        );
    }

    try {
        coil::DbusServer server(base_path, user_path);

        try {
            spdlog::info("Starting D-Bus server");
            server.run();

        } catch (std::exception& e) {
            spdlog::error(
                "Error during server execution: {}",
                e.what()
            );

            return 1;
        }
    } catch (std::exception& e) {
        spdlog::error(
            "Error during server starup: {}",
            e.what()
        );

        return 1;
    }

    return 0;
}
