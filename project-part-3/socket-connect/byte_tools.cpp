#include "byte_tools.h"

int BytesToInt(std::string_view bytes) {
    int result = 0;

    for (const char& byte : bytes) {
        result = (result << 8) + (int)(unsigned char)(byte);
    }

    return result;
}
