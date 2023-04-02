#include <cstring>
#include <vector>
#include <openssl/sha.h>
#include "bencodeParser.h"

void sha1(const char *data, size_t length, unsigned char *hash) {
    SHA1((const unsigned char *) data, length, hash);
}

std::string hashing(const BencodeDictionary& dict){
    if (dict.dict_val.find("info") == dict.dict_val.end()) {
        throw std::runtime_error("Invalid bencode: no info dictionary");
    }
    std::string toHashing = dict.get("info")->encode();
    unsigned char hash[20];
    sha1(toHashing.c_str(), toHashing.length(), hash);
    std::string result;
    for (int i = 0; i < 20; i++) {
        result += hash[i];
    }
    return result;
}