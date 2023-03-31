#pragma once
#include <string>
#include <vector>
#include <openssl/sha.h>

void sha1(const char *data, size_t length, unsigned char *hash) {
    SHA1((const unsigned char *) data, length, hash);
}

size_t parseInt(const std::string &contents, size_t index, size_t &value) {
    size_t i = index;
    value = 0;
    while (contents[i] >= '0' && contents[i] <= '9') {
        value = value * 10 + (contents[i] - '0');
        i++;
    }
    return i;
}

size_t getNextNode(const std::string &contents, size_t index) {
    if (contents[index] == 'd') {
        index++;
        while (contents[index] != 'e') {
            index = getNextNode(contents, index);
            index = getNextNode(contents, index);
        }
        return index + 1;
    } else if (contents[index] == 'l') {
        index++;
        while (contents[index] != 'e') {
            index = getNextNode(contents, index);
        }
        return index + 1;
    } else if (contents[index] == 'i') {
        index++;
        while (contents[index] != 'e') {
            index++;
        }
        return index + 1;
    } else {
        size_t length = 0;
        index = parseInt(contents, index, length);
        if (contents[index] != ':') {
            return 0;
        }
        return index + length + 1;
    }
}

// Parse the "info" dictionary and extract the infohash and other fields
bool parse_info_dict(const std::string &contents, unsigned char *infohash,
                     std::string &announce, std::string &comment,
                     std::vector<std::string> &pieces_hashes, size_t &piece_length,
                     size_t &length) {
    size_t index = 0;
    if (contents[index] != 'd') {
        return false;
    }
    index = contents.find("8:announce");
    if (index == std::string::npos) {
        return false;
    }
    index += 10;
    size_t announce_length = 0;
    index = parseInt(contents, index, announce_length);
    if (contents[index] != ':') {
        return false;
    }
    index++;
    announce = contents.substr(index, announce_length);
    index += announce_length;

    index = contents.find("7:comment");
    if (index != std::string::npos) {
        index += 9;
        size_t comment_length = 0;
        index = parseInt(contents, index, comment_length);
        if (contents[index] != ':') {
            return false;
        }
        index++;
        comment = contents.substr(index, comment_length);
        index += comment_length;
    }

    index = contents.find("6:lengthi");
    if (index != std::string::npos) {
        index += 9;
        index = parseInt(contents, index, length);
        if (contents[index] != 'e') {
            return false;
        }
        index++;
    }

    index = contents.find("12:piece lengthi");
    if (index == std::string::npos) {
        return false;
    }
    index += 16;
    index = parseInt(contents, index, piece_length);
    if (contents[index] != 'e') {
        return false;
    }
    index++;

    index = contents.find("6:pieces");
    if (index == std::string::npos) {
        return false;
    }
    index += 8;
    size_t pieces_length = 0;
    index = parseInt(contents, index, pieces_length);
    if (contents[index] != ':') {
        return false;
    }
    index++;
    for (size_t i = 0; i < pieces_length; i += 20) {
        pieces_hashes.push_back(contents.substr(index + i, 20));
    }
    index += pieces_length;

    index = contents.find("4:infod");
    if (index == std::string::npos) {
        return false;
    }
    index += 6;
    size_t info_end = getNextNode(contents, index);
    if (info_end == 0) {
        return false;
    }
    std::string info = contents.substr(index, info_end - index);
    sha1(info.c_str(), info.length(), infohash);
    return true;
}