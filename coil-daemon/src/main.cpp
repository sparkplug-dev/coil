#include "configParser.h"

int main() {
    coil::ConfigParser parser(
        "./example-files/etc/coil/default.json",
        "./example-files/.config/coil/config.json"
    );

    return 0;
}
