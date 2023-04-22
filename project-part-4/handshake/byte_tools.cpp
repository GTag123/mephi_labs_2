#include "byte_tools.h"
#include <openssl/sha.h>

int BytesToInt(std::string_view bytes) {
    int result = 0;

    for (const char& byte : bytes) {
        result = (result << 8) + (int)(unsigned char)(byte);
    }

    return result;
}

std::string CalculateSHA1(const std::string& msg){
    unsigned char hash[20];
    SHA1((const unsigned char *) msg.c_str(), msg.length(), hash);
    std::string result;
    for (int i = 0; i < 20; i++) {
        result += hash[i];
    }
    return result;
}