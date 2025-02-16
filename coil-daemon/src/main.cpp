#include <iostream>

#include "configParser.h"

int main() {
    coil::ConfigParser parser(
        "./example-files/etc/coil/default.json",
        "./example-files/.config/coil/config.json"
    );

    std::cout << parser.getConfig({"video", "dpi"}).value() << std::endl;

    nlohmann::json j;
    uint i = 101;
    j = i;

    std::cout << (int)j.type() << std::endl;

    coil::ConfigParser::SetStatus status = 
        parser.setConfig({"video", "dpi"}, j);

    if (status == coil::ConfigParser::SetStatus::Ok) 
        std::cout << "Ok" << std::endl;
    if (status == coil::ConfigParser::SetStatus::NotFound) 
        std::cout << "NotFound" << std::endl;
    if (status == coil::ConfigParser::SetStatus::TypeMismatch) 
        std::cout << "TypeMismatch" << std::endl;
    if (status == coil::ConfigParser::SetStatus::FileError) 
        std::cout << "FileError" << std::endl;
        

    return 0;
}
