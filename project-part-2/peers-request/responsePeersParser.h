#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include "map"
#include <utility>
#include <vector>

std::vector<Peer> parsePeers(const std::string& peersHash){
    std::vector<Peer> parsedPeers;
    for (int i = 0; i < 300; i += 6) {
        std::string ip_string;
        for (int j = 0; j < 4; j++){
            ip_string += std::to_string(static_cast<unsigned char>(peersHash[i+j]));
            if (j < 3) {
                ip_string += ".";
            }
        }
        uint16_t port = static_cast<uint16_t>(static_cast<uint16_t>(peersHash[i+5] << 8) |  static_cast<uint16_t>(peersHash[i+4]));
        std::unordered_map<std::string, std::string> peerMap;
        parsedPeers.push_back({ip_string, port});
    }
    return parsedPeers;
}