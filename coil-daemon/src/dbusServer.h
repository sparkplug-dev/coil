#ifndef COIL_DBUS_SERVER_H
#define COIL_DBUS_SERVER_H

#include <filesystem>
#include <memory>

#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IObject.h>

#include "configParser.h"

namespace coil {

// D-Bus service name
constexpr const char* c_dbus_service_name = "org.sparkplug.coil";
// D-Bus service version  
constexpr const char* c_dbus_service_version = "1";

// D-Bus root object name
constexpr const char* c_dbus_root_object = "/org/sparkplug/coil";

// D-Bus config interface name
constexpr const char* c_dbus_interface_name = "org.sparkplug.coil.config";
// D-Bus config interface version  
constexpr const char* c_dbus_interface_version = "1";

class DbusServer {
public:
    // Create the D-Bus server and parse the config files at the
    // given paths.
    // 
    // Raise an exception if the base config file is not found.
    DbusServer(
        std::filesystem::path base, 
        std::filesystem::path config 
    );

    // Run the D-Bus service main loop 
    void run();

private:
    // Create a object representing the given category and populate it 
    // with a property for each configuration
    void createCategoryObject(std::string_view category_name);

    // Create a property in the category object on the config interface
    // representing a config at the given path.
    void createConfigProperty(
        sdbus::IObject* category_object_p,
        const ConfigParser::ConfigMetadata& config_metadata
    );

    // Add a property to the object v-table on the config interface
    template <typename Type>
    void addPropertyToVTable(
        sdbus::IObject* category_object_p,
        const ConfigParser::ConfigMetadata& config_metadata
    );

private:
    // D-Bus connection 
    std::unique_ptr<sdbus::IConnection> m_connection;
    // D-Bus root object
    std::unique_ptr<sdbus::IObject> m_root_object;

    // Store the D-Bus object associated to each category
    std::map<std::string, std::unique_ptr<sdbus::IObject>> m_category_objects;

    // Store the configuration data
    ConfigParser m_config_parser;
};


// Add a property to the object v-table on the config interface
template <typename Type>
void DbusServer::addPropertyToVTable(
    sdbus::IObject* category_object_p,
    const ConfigParser::ConfigMetadata& config_metadata
) {
    // The interface name will remain unchanged during the entire execution
    // of the program, declaring it as static to avoid recalculating it
    // every time the function is called
    static sdbus::InterfaceName interface_name{
        std::string(c_dbus_interface_name) +
        c_dbus_interface_version
    };

    // Get the config path and name from the metadata
    ConfigParser::ConfigPath config_path = config_metadata.getPath();
    std::string config_name = std::string(config_path.getName());

    // Add the object to the v-table using the getter and setter
    // of the appropriate type
    category_object_p->addVTable(
        sdbus::registerProperty(config_name)
            .withGetter([&, config_path]() { 
                return m_config_parser.get<Type>(config_path);
        })
            .withSetter([&, config_path](const Type& data) {
                m_config_parser.set<Type>(config_path, data);
        })
    ).forInterface(interface_name);
}

} // namespace coil

#endif
