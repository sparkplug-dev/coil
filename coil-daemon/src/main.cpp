#include <cstdlib>

#include "dbusServer.h"
#include "spdlog/spdlog.h"

// Define the configuration environmental variables
constexpr const char* c_base_config_env = "COIL_BASE_CONFIG";
constexpr const char* c_user_config_env = "COIL_USER_CONFIG";
constexpr const char* c_debug_env = "COIL_DEBUG";

int main() {
    // Set the logging level
    if (getenv(c_debug_env))
        spdlog::set_level(spdlog::level::debug);

    // Get the configuration paths from the environment
    const char* base_path = getenv(c_base_config_env);
    const char* user_path = getenv(c_user_config_env);

    coil::DbusServer server(base_path, user_path);

    server.run();

    return 0;
}
