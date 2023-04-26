#include "byte_tools.h"
#include <openssl/sha.h>

size_t BytesToInt(std::string_view bytes) {
    int result = 0;

    for (const char& byte : bytes) {
        result = (result << 8) + (int)(unsigned char)(byte);
    }

    return result;
}

std::string IntToBytes(size_t value) {
    std::string result;
    result.resize(4);

    for (int i = 3; i >= 0; i--) {
        result[i] = (char)(value & 0xFF);
        value >>= 8;
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