#include <sstream>
#include "message.h"

Message Message::Parse(const std::string& messageString) {
    if (messageString.empty()) {
        return Message::Init(MessageId::KeepAlive, "");
    }
    auto messageId = (MessageId)(uint8_t)messageString[0];
    size_t length = messageString.length();
    return {.id=messageId, .messageLength=length, .payload=messageString.substr(1)};
}

Message Message::Init(MessageId id, const std::string &payload) {
    size_t length = payload.length() + 1;
    return {.id=id, .messageLength=length, .payload=payload};
}

std::string Message::ToString() const {
    std::stringstream buffer;
    char* messageLengthAddr = (char*) &messageLength;
    std::string messageLengthStr;
    // Bytes are pushed in reverse order, assuming the data
    // is stored in little-endian order locally.
    for (int i = 0; i < 4; i++) {
        messageLengthStr.push_back((char) messageLengthAddr[3 - i]);
    }
    buffer << messageLengthStr;
    buffer << (char) id;
    buffer << payload;
    return buffer.str();
}
