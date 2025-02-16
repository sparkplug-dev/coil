#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <utility>

#include "configParser.h"
#include "nlohmann/json_fwd.hpp"
#include "spdlog/spdlog.h"

namespace coil {

// Create a configuration parser from the given template 
// and user configuration files
//
// Raise exception if the base file is not found
ConfigParser::ConfigParser(
    std::filesystem::path base, 
    std::filesystem::path config 
) : 
    m_base_path(base),
    m_user_config_path(config)
{
    parseBaseConfig();
    parseUserConfig();

    auto r = updatedConfigs();
    for (auto& a: r) {
        std::cout << a.getName() << std::endl;
    }
}

// If the user configuration file was update since last calling this
// function, return a vector with all the config path that were updated 
std::vector<ConfigParser::ConfigPath> ConfigParser::updatedConfigs()
{
    std::vector<ConfigPath> updated = m_updated_config;
    m_updated_config.clear();

    return std::move(updated);
}

// Parse the base configuration and populate m_base_config
void ConfigParser::parseBaseConfig()
{
    // Parse the base template json
    std::ifstream base_file(m_base_path);
    nlohmann::json base_config = nlohmann::json::parse(base_file);

    // Iterate over all categories
    for (auto& category: base_config.items()) {
        std::string category_name = category.key();

        // Iterate over all settings
        for (auto& setting: category.value().items()) {
            std::string setting_name = setting.key();

            // Parse the setting field 
            nlohmann::json default_data = setting.value()["default"];
            nlohmann::json displayed_name = setting.value()["displayed_name"];
            nlohmann::json description = setting.value()["description"];

            // Check if all the field exist
            if (default_data.is_null()) {
                spdlog::warn(
                    "Ignoring \"{}: ({}:{})\"; missing default field",
                    m_base_path.c_str(), category_name, setting_name
                );

                continue;
            }
            if (displayed_name.is_null()) {
                spdlog::warn(
                    "Ignoring \"{}: ({}:{})\"; missing displayed_name field",
                    m_base_path.c_str(), category_name, setting_name
                );

                continue;
            }
            if (description.is_null()) {
                spdlog::warn(
                    "Ignoring \"{}: ({}:{})\"; missing description field",
                    m_base_path.c_str(), category_name, setting_name
                );

                continue;
            }

            // Check the type of each field
            if (default_data.is_object()) {
                spdlog::warn(
                    "Ignoring \"{}: ({}:{})\"; default field can't be an onject",
                    m_base_path.c_str(), category_name, setting_name
                );

                continue;
            }
            if (!displayed_name.is_string()) {
                spdlog::warn(
                    "Ignoring \"{}: ({}:{})\"; displayed_name has wrong type",
                    m_base_path.c_str(), category_name, setting_name
                );

                continue;
            }
            if (!description.is_string()) {
                spdlog::warn(
                    "Ignoring \"{}: ({}:{})\"; description has wrong type",
                    m_base_path.c_str(), category_name, setting_name
                );

                continue;
            }

            // Create the config path object
            ConfigPath setting_path(category_name, setting_name);

            // Create the base storage object
            ConfigBaseData setting_data(
                default_data,
                default_data.type(),
                displayed_name.get<std::string>(),
                description.get<std::string>()
            );

            // Create the base template table entry
            m_base_config[setting_path] = setting_data;
        }
    }
}

// Parse the user configuration data using the data stored in 
// the base configuration map
void ConfigParser::parseUserConfig()
{
    nlohmann::json user_config;

    try {
        // Parse the base template json
        std::ifstream user_config_path(m_user_config_path);
        user_config = nlohmann::json::parse(user_config_path);
        
    } catch (std::exception& e) {
        spdlog::error(
            "Exception while opening user config file: {}",
            e.what()
        );

        return;
    }

    // Iterate over all categories
    for (auto& category: user_config.items()) {
        std::string category_name = category.key();

        // Iterate over all settings
        for (auto& setting: category.value().items()) {
            std::string setting_name = setting.key();
            nlohmann::json setting_data = setting.value();

            ConfigPath setting_path(category_name, setting_name);

            // Check if the setting exist in the base config
            if (m_base_config.find(setting_path) == m_base_config.end()) {
                spdlog::warn(
                    "Ignoring \"{}: ({}:{})\"; setting not in base config",
                    m_user_config_path.c_str(), category_name, setting_name
                );

                continue;
            }

            // Retrieve the base config data
            const ConfigBaseData& base_data = m_base_config[setting_path]; 

            // Check if the type of the setting is correct
            if (base_data.getType() != setting_data.type()) {

                spdlog::warn(
                   "Ignoring \"{}: ({}:{})\"; wrong setting type",
                    m_user_config_path.c_str(), category_name, setting_name
                );

                continue;
            }

            // Check if the setting was already in the user config map
            if (m_user_config.find(setting_path) != m_user_config.end()) {
                // Check if the setting was updated and push the path 
                // to m_updated_config
                nlohmann::json old_data = m_user_config[setting_path];

                if (old_data != setting_data) {
                    m_updated_config.push_back(setting_path);
                }
            } else {
                // If the setting is new push it to updated config
                m_updated_config.push_back(setting_path);
            }

            // Update the user config table
            m_user_config[setting_path] = setting_data;
        }
    }
}

// Return a constant reference to an entry of the base template 
// configuration table if it exist.
// Return nullopt if it doesn't 
std::optional<std::reference_wrapper<const ConfigParser::ConfigBaseData>> 
ConfigParser::getBaseConfig(
    const ConfigPath& config_path
) {
    if (m_base_config.find(config_path) != m_base_config.end()) {
        return m_base_config[config_path];
    }

    return std::nullopt;
}

} // namespace coil
