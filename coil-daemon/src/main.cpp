#include <iostream>
#include <cstdlib>

#include "configParser.h"

// Define the configuration environmental variables
constexpr const char* c_base_config_env = "COIL_BASE_CONFIG";
constexpr const char* c_user_config_env = "COIL_USER_CONFIG";

int main() {
    // Get the configuration paths from the environment
    const char* base_path = getenv(c_base_config_env);
    const char* user_path = getenv(c_user_config_env);

    coil::ConfigParser parser(base_path, user_path);

    int i = parser.getConfig({"video", "dpi"}).value().get<int>();
    std::cout << i << std::endl;

    parser.setConfig({"video", "dpi"}, (int)(i + 1));

    i = parser.getConfig({"video", "dpi"}).value().get<int>();
    std::cout << i << std::endl;
    
    bool b = parser.getConfig({"general", "mirroring"}).value().get<bool>();
    std::cout << b << std::endl;

    parser.setConfig({"general", "mirroring"}, !b);

    b = parser.getConfig({"general", "mirroring"}).value().get<bool>();
    std::cout << b << std::endl;

    if (parser.wasUpdated()) {
        for (auto& i : parser.updatedConfigs()) {
            std::cout << i.getCategory() << ":" << i.getName() << std::endl;
        }
    }

    return 0;
}
