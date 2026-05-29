#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <vector>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>
#include <sstream>
#include "include/tts.h"

std::string sanitizePlayerName();

// Load YAML into memory
YAML::Node tryLoadConfig() {
    try {
        YAML::Node config = YAML::LoadFile("devices.yaml");
        return config;
    } catch (const YAML::Exception& e) {
        std::cout << "Warning! No YAML file found! Exiting." << std::endl;
        return YAML::Node();
    }
}

bool isPlaying() {
    int result = system(
        "playerctl status -a 2>/dev/null | grep -q Playing"
    );

    return result == 0;
}

bool tryConnect(std::string airpods) {
    if(isPlaying()) {
        std::string init = "bluetoothctl connect ";
        std::string argument = airpods;
        std::string command = init + argument;
        int result = system(
            command.c_str()
        );

        return result == 0;
    } else {
        return false;
    }
}

std::string runCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");

    if(!pipe) {
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe)) {
        result += buffer.data();
    }

    pclose(pipe);
    return result;
}

std::string getPlayingTab() {
    std::string output = runCommand("playerctl -a status --format '{{playerName}} {{status}}'");

    std::istringstream stream(output);
    std::string line;

    while(std::getline(stream, line)) {
        if(line.find("Playing") != std::string::npos) {
            return line.substr(0, line.find(' '));
        }
    }

    return "";
}

std::string sanitizePlayerName(std::string name) {
    name.erase(std::remove_if(name.begin(), name.end(), [](char c) {
        return !std::isalnum(c) && c != '-' && c != '_';
    }), name.end());
    return name;
}

std::vector<std::string> getDevices() {
    std::string deviceList = runCommand("bluetoothctl devices");
    std::regex macRegEx("([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}");
    std::vector<std::string> macAddresses;

    auto words_begin = std::sregex_iterator(deviceList.begin(), deviceList.end(), macRegEx);
    auto words_end = std::sregex_iterator();
    
    for(std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        macAddresses.push_back(match.str());
    }

    return macAddresses;
}

std::string isListeningDevice() {
    std::vector<std::string> macAddresses = getDevices();

    for(const std::string& i : macAddresses) {
        std::string info = runCommand("bluetoothctl info " + i);
        bool isHeadphone = info.find("Icon: audio-headphones") != std::string::npos;
        bool isPaired = info.find("Paired: yes") != std::string::npos;
        bool isTrusted = info.find("Trusted: yes") != std::string::npos;

        if(isHeadphone && isPaired && isTrusted) {
            return i;
        }
    }

    return "";
}

bool isConnected() {
    std::vector<std::string> macAddresses = getDevices();

    for(const std::string& i : macAddresses) {
        std::string info = runCommand("bluetoothctl info " + i);
        bool isHeadphone = info.find("Icon: audio-headphones") != std::string::npos;
        bool isPaired = info.find("Paired: yes") != std::string::npos;
        bool isTrusted = info.find("Trusted: yes") != std::string::npos;
        bool connected = info.find("Connected: yes") != std::string::npos;

        if(connected && isHeadphone && isPaired && isTrusted) {
            std::cout << "Connected!" << std::endl;
            return true;
        }
    }

    return false;
}

bool waitUntilConnected(const std::string& mac)
{
    for (int i = 0; i < 10; i++)
    {
        std::string info = runCommand("bluetoothctl info " + mac);

        if (info.find("Connected: yes") != std::string::npos)
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return false;
}

enum class availableAudioModes {
    strict,
    tab,
    open,
    unknown
};

availableAudioModes hashString(const std::string& str) {
    if (str == "strict") return availableAudioModes::strict;
    if (str == "tab") return availableAudioModes::tab;
    if (str == "open") return availableAudioModes::open;
    return availableAudioModes::unknown;
}

int main() {
    bool disconnected = true;
    bool isPlayingAtLaunch = isPlaying();
    YAML::Node config = tryLoadConfig();
    std::string airpods;
    std::string headphones;
    std::string audio_mode;
    bool autoStandBy = true;
    bool alreadyConnected = false;
    bool read_notifications_out = false;

    if(!config || !config["devices"]) {
        airpods = isListeningDevice();
        audio_mode = "strict";
    } else {
        YAML::Node devicesNode = config["devices"];
        
        if(devicesNode.begin() != devicesNode.end()) {
            headphones = devicesNode.begin()->first.as<std::string>();
        } else {
            headphones = "airpods";
        }

        airpods = config["devices"][headphones]["mac"].as<std::string>();
        autoStandBy = config["auto-standby"].as<bool>(false);
        audio_mode = config["audio_mode"].as<std::string>("strict");
        read_notifications_out = config["read_notifications_out"].as<bool>(false);
    }

    //TEST NOTIFICATION
    if(read_notifications_out) {
        std::thread notificationThread(watchDesktopNotifications);
        notificationThread.detach();
    }

    if(isPlayingAtLaunch) {
        tryConnect(airpods);
    }

    while (true) {
        if (isPlaying()) {
            if (airpods.empty())
            {
                std::cerr << "No AirPods MAC found!\n";
                return 1;
            }
            if(disconnected && !isConnected()) {
                std::string player = sanitizePlayerName(getPlayingTab());
                
                switch (hashString(audio_mode)) {
                    case availableAudioModes::strict:
                        system("playerctl -a pause");
                        break;
                    case availableAudioModes::tab:
                        if(!player.empty()) {
                            system(("playerctl -p " + player + " pause").c_str());
                        }
                        break;
                    case availableAudioModes::open:
                        break;
                    case availableAudioModes::unknown:
                        system("playerctl -a pause");
                        break;
                }

                bool res = tryConnect(airpods);
                if(res && waitUntilConnected(airpods)) {
                    if(!player.empty()) {
                        system(("playerctl -p " + player + " play").c_str());
                    }
                    disconnected = false;
                }
            } else {
                disconnected = false;
            }
        } else {
            if(!disconnected && autoStandBy) {
                std::string init = "bluetoothctl disconnect ";
                std::string cmd = init + airpods;
                system(cmd.c_str());
                disconnected = true;
                alreadyConnected = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}