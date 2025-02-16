#ifndef COIL_CONFIG_PARSER_H
#define COIL_CONFIG_PARSER_H

#include <filesystem>
#include <optional>
#include <string>
#include <map>

#include <nlohmann/json.hpp>

namespace coil {

// Json configuration parser
class ConfigParser {
public:
    // Point to the location of a setting in the configuration files
    struct ConfigPath {
        ConfigPath() : m_category(), m_name() { };
        ConfigPath(
            std::string_view category, std::string_view name
        ) : m_category(category), m_name(name) { };

        // Return the setting category
        std::string_view getCategory() const { return m_category; };
        // Return the setting name
        std::string_view getName() const { return m_name; };

        // Ordering operator for usage in map
        bool operator <(const ConfigPath& rhs) const
        {
            if (m_category == rhs.m_category)
                return m_name < rhs.m_name;

            return m_category < rhs.m_category;
        }

    private:
        // The category of the setting
        std::string m_category;
        // The name of the setting
        std::string m_name;
    };

    // Report the status of a set operation
    enum class SetStatus {
        Ok = 0,
        NotFound,
        TypeMismatch,
        FileError
    };

private:
    // Configuration base template data storage
    struct ConfigBaseData {
        ConfigBaseData() { };
        ConfigBaseData(
            nlohmann::json default_config,
            nlohmann::json::value_t type,
            std::string_view displayed_name,
            std::string_view description
        ) :
            m_default(default_config),
            m_type(type),
            m_displayed_name(displayed_name),
            m_description(description)
        { };

        // Return the settings type
        nlohmann::json getDefault() const { return m_default; }

        // Return the settings type
        nlohmann::json::value_t getType() const { return m_type; }

        // Return the settings displayed name
        std::string_view getDisplayedName() const { return m_displayed_name; }
        // Return the settings description
        std::string_view getDescription() const { return m_description; }

    private:
        // Store the default configuration
        nlohmann::json m_default;

        // The configuration json type
        nlohmann::json::value_t m_type;

        // Displayed name
        std::string m_displayed_name;
        // Description
        std::string m_description;
    };

public:
    // Create a configuration parser from the given template 
    // and user configuration files
    //
    // Raise exception if the base file is not found
    ConfigParser(
        std::filesystem::path base, 
        std::filesystem::path config 
    );

    // Return the json object associated with the requested setting.
    // Settings are retrieved from the configuration files according
    // to they name and category
    //
    // nullopt if the requested setting is not found 
    // in the base template configuration
    std::optional<nlohmann::json> getConfig(const ConfigPath& config_path);

    // Return the json object associated with the requested setting.
    // Settings are retrieved from the configuration files according
    // to they name and category.
    //
    // Return Ok if the operation was successful 
    // Return NotFound if the requested setting is not found 
    // in the base template configuration.
    // Return TypeMismatch if the type of the given json object doesn't match
    // the type of the settings in the base template configuration.
    // Return FileError if writing to the config file failed.
    SetStatus setConfig(const ConfigPath& config_path, nlohmann::json data);

    // Return true if the any configuration was updated since 
    // last calling this function
    bool wasUpdated();

    // If the user configuration file was update since last calling this
    // function, return a vector with all the config path that were updated 
    std::vector<ConfigPath> updatedConfigs();

private:
    // Parse the base configuration and populate m_base_config
    // Raise exception if the base config file is not found
    void parseBaseConfig();

    // Parse the user configuration data using the data stored in 
    // the base configuration map
    // If any configuration changed in from the last parse, the config
    // path is appended to the m_updated_config vector.
    // On first run all the setting are appended to m_updated_config.
    void parseUserConfig();

    // Store the content of m_user_config to the user configuration file
    //
    // Raise an exception if the write fail
    void storeUserCofig();

    // Return a constant reference to an entry of the base template 
    // configuration table if it exist.
    // Return nullopt if it doesn't 
    std::optional<std::reference_wrapper<const ConfigParser::ConfigBaseData>> 
    getBaseConfig(const ConfigPath& config_path);

    // Check if the user configuration file was updated since the last
    // read and parse it again if necessary
    void checkConfigFileUpdate();

private:
    // Store the base configuration data
    std::map<ConfigPath, ConfigBaseData> m_base_config;
    // Store the user configuration data
    std::map<ConfigPath, nlohmann::json> m_user_config;

    // Path to configuration base template and default configuration
    std::filesystem::path m_base_path;
    // Path to user configuration file
    std::filesystem::path m_user_config_path;

    // User configuration last edit time, used to check if the user
    // configuration file was update since the last read
    std::filesystem::file_time_type m_last_write;

    // Store the config path that were updated since
    // last calling updatedConfigs
    std::vector<ConfigPath> m_updated_config;
    // Store true if the configuration was updated since calling wasUpdated
    bool m_updated;
};

} // namespace coil

#endif
