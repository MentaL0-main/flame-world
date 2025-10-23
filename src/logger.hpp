#pragma once

#include <iostream>

class Logger
{
public:
    inline void message(const char* msg_) {
        std::cout << "[*] " << msg_ << std::endl;
    }

    inline void error(const char* msg_) {
        std::cout << "[!] " << msg_ << '\n' << "Aborting..." << std::endl;
    }
};