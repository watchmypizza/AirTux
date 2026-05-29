#include "include/watcher.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

void watchBluetoothDisconnect(const std::string &target_mac) {
    if (target_mac.empty()) return;

    std::string dbusMac = target_mac;
    std::replace(dbusMac.begin(), dbusMac.end(), ":", "_");
    std::string target_device_path = "dev_" + dbusMac;

    std::string cmd = "dbus-monitor --system \"type='signal',sender='org.bluez',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'\" 2>/dev/null";

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

    if(!pipe) {
        std::cerr << "Failed to attach hardware disconnect listener!\n";
        return;
    }

    std::array<char, 512> buffer;
    bool is_target_device = false;

    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line(buffer.data());

        if(line.find("PropertiesChanged") != std::string::npos) {
            if(line.find(target_device_path) != std::string::npos) {
                is_target_device = true;
            } else {
                is_target_device = false;
            }
            continue;
        }

        if(is_target_device) {
            if(line.find("string \"Connected\"") != std::string::npos) {
                if(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                    std::string variant_line(buffer.data());

                    if(variant_line.find("boolean false") != std::string::npos) {
                        std::cout << "[CRIT DISCONNECT] Headphones dropped. Pausing everything." << std::endl;

                        system("playerctl -a pause");

                        is_target_device = false;
                    }
                }
            }
        }
    }
}