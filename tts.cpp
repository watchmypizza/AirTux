#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <regex>
#include <speech-dispatcher/speechd_types.h>
#include <string>
#include <unistd.h>
#include <speech-dispatcher/libspeechd.h>
#include "include/tts.h"

static SPDConnection* g_conn = nullptr;

void sayNotification(std::string text) {
    SPDConnection* conn = spd_open("simple-tts", "main", NULL, SPD_MODE_SINGLE);
    if(!conn) return;

    spd_say(conn, SPD_NOTIFICATION, text.c_str());

    spd_close(conn);
    return;
}

void watchDesktopNotifications() {
    std::string cmd = "dbus-monitor --session \"interface='org.freedesktop.Notifications',member='Notify'\" 2>/dev/null";
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        std::cerr << "Failed to attach notification event watcher!\n";
        return;
    }

    std::array<char, 512> buffer;
    std::string appName, summary, body;
    int stringCounter = -1;

    std::regex quoteRegex("^\\s*string\\s*\"(.*)\"\\s*$");
    std::smatch match;

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string line(buffer.data());

        if (line.find("member=Notify") != std::string::npos) {
            stringCounter = 0; 
            appName = summary = body = "";
            continue;
        }

        if (stringCounter >= 0) {
            if (std::regex_search(line, match, quoteRegex)) {
                std::string extractedContent = match[1].str(); 

                if (stringCounter == 0) {
                    appName = extractedContent; 
                } 
                else if (stringCounter == 2) {
                    summary = extractedContent; 
                } 
                else if (stringCounter == 3) {
                    body = extractedContent;
                    
                    if (!summary.empty() || !body.empty()) {
                        std::string fullAnnouncement = appName + " notification from " + summary + ": " + body;
                        std::cout << "[Event Logged] " << fullAnnouncement << std::endl;
                        sayNotification(fullAnnouncement); 
                    }
                    
                    stringCounter = -1; 
                    continue;
                }
                
                stringCounter++; 
            }
        }
    }
}