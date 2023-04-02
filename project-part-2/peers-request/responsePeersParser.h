#include <string>
#include <unordered_map>
#include <vector>

std::vector<Peer> parsePeers(const std::string& peersHash){
    std::vector<Peer> parsedPeers;
    for (int i = 0; i < peersHash.size(); i += 6) {
        std::string ip_string;
        for (int j = 0; j < 4; j++){
            ip_string += std::to_string(static_cast<unsigned char>((int) peersHash[i+j]));
            if (j < 3) {
                ip_string += ".";
            }
        }
        auto port = static_cast<uint16_t>(((int) peersHash[i+4]) << 8) |  static_cast<uint16_t>((int) peersHash[i+5]);
        std::unordered_map<std::string, std::string> peerMap;
        parsedPeers.push_back({ip_string, port});
    }
    return parsedPeers;
}