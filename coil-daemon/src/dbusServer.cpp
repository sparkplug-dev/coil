#include <poll.h>
#include <sdbus-c++/Types.h>
#include <sys/eventfd.h>

#include <sdbus-c++/IObject.h>
#include <thread>

#include "dbusServer.h"
#include "spdlog/spdlog.h"

//#include <chrono>
//#include <thread>

namespace coil {

constexpr int c_loop_sleep_millis = 10;

// Create the dbus server and parse the config files at the
// given paths
// 
// Raise an exception if the base config file is not found
DbusServer::DbusServer(
    std::filesystem::path base, 
    std::filesystem::path config 
) :
    m_config_parser(base, config)
{
    // Create D-Bus connection to bus and requests a well-known name on it.
    sdbus::ServiceName service_name{
        std::string(c_dbus_service_name) +
        c_dbus_service_version
    };
    m_connection = sdbus::createBusConnection(service_name);

    // Create the root D-Bus object.
    sdbus::ObjectPath root_object_path{c_dbus_root_object};
    m_root_object = sdbus::createObject(
        *m_connection,
        std::move(root_object_path)
    );

    // Create the configuration objects
    for (auto& category : m_config_parser.getCategories()) {
        createCategoryObject(category);
    }
}

// Run the D-Bus service main loop 
void DbusServer::run()
{
    while (true) {
        // Poll the file descriptor 
        sdbus::IConnection::PollData poll_data =
            m_connection->getEventLoopPollData();

        struct pollfd fds[] = {
            {poll_data.fd, poll_data.events, 0},
            {poll_data.eventFd, POLLIN, 0}
        };
        constexpr auto fds_count = sizeof(fds)/sizeof(fds[0]);

        int timeout = poll_data.getPollTimeout();

        // Perform the poll operation
        int r = poll(fds, fds_count, timeout);

        // A signal was caught during poll try again
        if (r < 0 && errno == EINTR) {

        }
        
        // Check if the poll was successful 
        if (r < 0) {
            spdlog::warn("Poll error in D-Bus main loop: {}", errno);
            continue;
        }

        // Process the pending event on the bus
        m_connection->processPendingEvent();

        // Send the property change signals if necessary
        sendChangeSignals();
    }
}

// Create a object representing the given category and populate it 
// with a property for each configuration
void DbusServer::createCategoryObject(std::string_view category_name)
{
    spdlog::debug("Creating object for category: {}", category_name);

    // Gather the settings metadata 
    const std::vector<ConfigParser::ConfigMetadata>& metadatas = 
        m_config_parser.getMetadatas(category_name);

    // If the metadata list is empty return without creating the object
    if (metadatas.empty())
        return;
    
    // Build the object path
    std::string object_path_str;
    object_path_str += c_dbus_root_object;
    object_path_str += "/";
    object_path_str += category_name;

    // Create the object 
    sdbus::ObjectPath object_path{object_path_str};
    std::unique_ptr<sdbus::IObject> object = sdbus::createObject(
        *m_connection,
        std::move(object_path)
    );

    // Populate the object with the configuration properties
    for (const auto& metadata: metadatas) {
        createConfigProperty(object.get(), metadata);
    }

    // Store the object in the category object map
    m_category_objects[std::string(category_name)] = std::move(object); 
}

// Create a property in the category object on the config interface
// representing a config at the given path.
void DbusServer::createConfigProperty(
    sdbus::IObject* category_object_p,
    const ConfigParser::ConfigMetadata& config_metadata
) {
    spdlog::debug(
        "Creating property for config: \"{}:{}\"",
        config_metadata.getPath().getCategory(),
        config_metadata.getPath().getName()
    );

    // Get the config type form the metadata
    ConfigParser::ConfigType type = config_metadata.getType();

    // Add the property to the v-table using the appropriate type
    switch (type) {
        case ConfigParser::ConfigType::Bool:
            addPropertyToVTable<bool>(category_object_p, config_metadata);
            break;
        
        case ConfigParser::ConfigType::Int:
            addPropertyToVTable<int>(category_object_p, config_metadata);
            break;

        case ConfigParser::ConfigType::Float:
            addPropertyToVTable<double>(category_object_p, config_metadata);
            break;

        case ConfigParser::ConfigType::String:
            addPropertyToVTable<std::string>(
                category_object_p, config_metadata
            );
            break;

        case ConfigParser::ConfigType::ArrayInt:
            addPropertyToVTable<std::vector<int>>(
                category_object_p, config_metadata
            );
            break;

        case ConfigParser::ConfigType::ArrayFloat:
            addPropertyToVTable<std::vector<double>>(
                category_object_p, config_metadata
            );
            break;

        case ConfigParser::ConfigType::ArrayString:
            addPropertyToVTable<std::vector<std::string>>(
                category_object_p, config_metadata
            );
            break;
        
        default:
            spdlog::error(
                "Unknown config type in DbusServer::createConfigProperty"
            );
    }
}

// Send property change signal if changes occurred in the config parser  
void DbusServer::sendChangeSignals()
{
    // The interface name will remain unchanged during the entire execution
    // of the program, declaring it as static to avoid recalculating it
    // every time the function is called
    static sdbus::InterfaceName interface_name{
        std::string(c_dbus_interface_name) +
        c_dbus_interface_version
    };

    if (m_config_parser.wasUpdated()) {
        // Send a signal for each update config
        for (auto& config : m_config_parser.updatedConfigs()) {
            // Get the configuration object
            std::string category(config.getCategory());
            std::string name(config.getName());
            auto& object = m_category_objects[category];

            // Emit the signal
            sdbus::PropertyName property_name {name};

            object->emitPropertiesChangedSignal(
                interface_name, 
                { property_name }
            );
        }
    }
}

// Property change 
void DbusServer::signalChangeLoop()
{
    while (true) {
        sendChangeSignals();

        // Sleep before the next iteration
        std::this_thread::sleep_for(
            std::chrono::milliseconds(c_loop_sleep_millis)
        );    
    }
}

} // namespace coil
