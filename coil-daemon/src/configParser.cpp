#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>

#include "nlohmann/json_fwd.hpp"
#include "spdlog/spdlog.h"

#include "configParser.h"

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

    // Store the last update time of the user config file if it exist
    if (std::filesystem::exists(m_user_config_path)) {
        m_last_write = std::filesystem::last_write_time(m_user_config_path);
    }

    // Clear the updated config vector
    m_updated_config.clear();
    m_updated = false;
}

// Return the json object associated with the requested setting.
// Settings are retrieved from the configuration files according
// to they name and category
//
// Return nullopt if the requested setting is not found 
// in the base template configuration
std::optional<nlohmann::json> ConfigParser::getConfig(
    const ConfigPath& config_path
) {
    // Check for config file updates and parse the file if necessary
    checkConfigFileUpdate();

    // Check if settings is in the user settings table
    if (m_user_config.find(config_path) != m_user_config.end()) {
        return m_user_config[config_path];
    }

    // Return the base template default value otherwise
    if (m_base_config.find(config_path) != m_base_config.end()) {
        return m_base_config[config_path].getDefault();
    }

    spdlog::warn(
        "getConfig failed: setting not found ({}:{})",
        config_path.getCategory(),
        config_path.getName()
    );

    // Return nullopt if the setting was not found
    return std::nullopt;
}

// Return the json object associated with the requested setting.
// Settings are retrieved from the configuration files according
// to they name and category.
//
// Return Ok if the operation was successful 
// Return NotFound if the requested setting is not found 
// in the base template configuration.
// Return TypeMismatch if the type of the given json object doesn't match
// the type of the settings in the base template configuration.
ConfigParser::SetStatus ConfigParser::setConfig(
    const ConfigPath& config_path,
    nlohmann::json data
) {
    // Check for config file updates and parse the file if necessary
    checkConfigFileUpdate();

    // Check if the setting is in the base template table
    if (m_base_config.find(config_path) == m_base_config.end()) {
        spdlog::warn(
            "setConfig failed: setting not found ({}:{})",
            config_path.getCategory(),
            config_path.getName()
        );

        return SetStatus::NotFound;
    }

    // Check if the provided data type matches the expected one
    if (m_base_config[config_path].getType() != getConfigType(data)) {
        spdlog::warn(
            "setConfig failed: type mismatch (expected: {}; got: {})",
            configTypeStr(m_base_config[config_path].getType()),
            configTypeStr(getConfigType(data))
        );

        return SetStatus::TypeMismatch;
    }

    if (m_user_config.find(config_path) == m_user_config.end()) {
        m_user_config[config_path] = data;

        try {
            storeUserCofig();

        // TODO: catch proper exception type
        } catch (std::exception& e) {
            // Revert any changes to the stored configuration
            m_user_config.erase(config_path);
           
            spdlog::warn(
                "setConfig failed: file error ({})",
                e.what()
            ); 

            return SetStatus::FileError;
        }
    } else {
        // Store the old data to revert changes in case of write failure
        nlohmann::json old_data = m_user_config[config_path];
        
        m_user_config[config_path] = data;

        try {
            storeUserCofig();

        // TODO: catch proper exception type
        } catch (std::exception& e) {
            // Revert any changes to the stored configuration
            m_user_config[config_path] = old_data;
            
            spdlog::warn(
                "setConfig failed: file error ({})",
                e.what()
            ); 

            return SetStatus::FileError;
        }
    }

    // Set was update to true and return Ok
    m_updated = true;
    m_updated_config.push_back(config_path);

    return SetStatus::Ok;
}

// If the user configuration file was update since last calling this
// function, return a vector with all the config path that were updated 
std::vector<ConfigParser::ConfigPath> ConfigParser::updatedConfigs()
{
    std::vector<ConfigPath> updated = m_updated_config;
    m_updated_config.clear();

    return std::move(updated);
}

// Return true if the any configuration was updated since 
// last calling this function
bool ConfigParser::wasUpdated() 
{
    // Check if the config file was updated and parse the config
    // file if necessary
    checkConfigFileUpdate();

    bool was_updated = m_updated;
    m_updated = false;

    return was_updated;
}

// Store the content of m_user_config to the user configuration file
//
// Raise an exception if the write fail
void ConfigParser::storeUserCofig()
{
    // Generate the json object
    nlohmann::json json_config;

    for (const auto& setting : m_user_config) {
        const ConfigPath& path = setting.first;

        // Store the configuration data at the appropriate path
        json_config[path.getCategory()][path.getName()] = setting.second;
    }

    // write prettified JSON to another file
    // Use a dedicated scope to grantee writing to the file before 
    // filesystem last_write_time is run
    {
        std::string json_str = json_config.dump(4);

        // If the configuration file doesn't exist warn that 
        // a new one will be created
        if (!std::filesystem::exists(m_user_config_path)) {
            spdlog::warn(
                "Configuration file not found, creating one at: {}",
                m_user_config_path.c_str()
            );
        }
        
        // Enable exceptions for write failure
        std::ofstream user_config_f(m_user_config_path);
        user_config_f.exceptions(
            std::ofstream::failbit | std::ofstream::badbit 
        );

        user_config_f << json_str << std::endl;
    }

    // Update the last write time
    m_last_write = std::filesystem::last_write_time(m_user_config_path);
}

// Parse the base configuration and populate m_base_config
void ConfigParser::parseBaseConfig()
{
    nlohmann::json base_config;

    try {
        // If the file doesn't exist throw an exception
        if (std::filesystem::exists(m_base_path)) {
            // Parse the base template json
            std::ifstream file(m_base_path);
            base_config = nlohmann::json::parse(file);
        } else {
            throw std::runtime_error(
                "Base config file not found"
            );
        }

    } catch (std::exception& e) {
        spdlog::error(
            "Exception while opening base config file: {}",
            e.what()
        );

        throw e;
    }

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
                    "Ignoring \"{}: ({}:{})\"; default can't be an onject",
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
        // If the file doesn't exist throw an exception
        if (std::filesystem::exists(m_user_config_path)) {
            // Parse the user config json
            std::ifstream file(m_user_config_path);
            user_config = nlohmann::json::parse(file);
        } else {
            throw std::runtime_error(
                "User config file not found"
            );
        }
        
    // TODO: catch proper exception type
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
            if (base_data.getType() != getConfigType(setting_data)) {
                spdlog::warn(
                   "Ignoring \"{}: ({}:{})\"; wrong type (expected: {})",
                    m_user_config_path.c_str(),
                    category_name,
                    setting_name,
                    configTypeStr(getConfigType(setting_data))
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
                    m_updated = true;
                }
            } else {
                // If the setting is new push it to updated config
                m_updated_config.push_back(setting_path);
                m_updated = true;
            }

            // Update the user config table
            m_user_config[setting_path] = setting_data;
        }
    }
}

// Check if the user configuration file was updated since the last
// read and parse it again if necessary
void ConfigParser::checkConfigFileUpdate()
{
    // Only check for updates if the user config file exist
    // if the file was deleted update will be ignored
    if (std::filesystem::exists(m_user_config_path)) {
        std::filesystem::file_time_type write_time = 
            std::filesystem::last_write_time(m_user_config_path);

        // If the write time don't match an updated occurred
        // and parse the config file
        if (m_last_write != write_time) {
            parseUserConfig();
            m_last_write = write_time;
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

// Return the setting type
ConfigParser::ConfigType ConfigParser::getConfigType(
    nlohmann::json data
) {
    switch (data.type()) {
        case nlohmann::json::value_t::number_integer:
            return ConfigType::Int;
        case nlohmann::json::value_t::number_unsigned:
            return ConfigType::Int;
        case nlohmann::json::value_t::number_float:
            return ConfigType::Float;
        case nlohmann::json::value_t::string:
            return ConfigType::String;
        case nlohmann::json::value_t::array:
            return ConfigType::Array;

        default:
            return ConfigType::None;
    }
}

// Return a string representation of the type 
std::string_view ConfigParser::configTypeStr(ConfigType type)
{
    switch (type) {
        case ConfigType::Int:
            return "Int";
        case ConfigType::Float:
            return "Float";
        case ConfigType::String:
            return "String";
        case ConfigType::Array:
            return "Array";

        default:
            return "Unknow type";
    }
}

} // namespace coil
