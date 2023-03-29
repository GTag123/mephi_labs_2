#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include "parser.h"
//#include "BencodeHash.h"

int main() {
    std::ifstream file("../debian.iso.torrent", std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string contents = buffer.str();
    file.close();
    std::string announce, comment;
    std::vector<std::string> piece_hashes;
    size_t piece_length;
    size_t length;
    string infoHash;

    size_t pivot = 0;
    shared_ptr<BencodeDictionary> dict = dynamic_pointer_cast<BencodeDictionary>(parse_bencode(contents.c_str(), contents.size(), pivot));
    announce = dynamic_pointer_cast<BencodeString>(dict->get("announce"))->get_str();
    comment = dynamic_pointer_cast<BencodeString>(dict->get("comment"))->get_str();
    piece_length = dynamic_pointer_cast<BencodeInteger>(dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("piece length"))->get_int();
    length = dynamic_pointer_cast<BencodeInteger>(dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("length"))->get_int();
    string piecesHash = dynamic_pointer_cast<BencodeString>(dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("pieces"))->get_str();
    for (int i = 0; i < piecesHash.length(); i += 20) {
        piece_hashes.push_back(piecesHash.substr(i, 20));
    }
//    infoHash = hashing(*dynamic_pointer_cast<BencodeDictionary>(dict->get("info")));
    string encoded = dict->encode();
    return 0;
}