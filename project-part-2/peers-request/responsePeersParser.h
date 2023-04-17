#include <string>
#include <unordered_map>
#include <vector>

std::vector<Peer> parsePeers(const std::string& peersHash){
    std::vector<Peer> parsedPeers;
    for (int i = 0; i < peersHash.size(); i += 6) {
        std::string ip_string;
        for (int j = 0; j < 4; j++){
            ip_string += std::to_string(static_cast<unsigned char>(peersHash[i+j]));
            if (j < 3) {
                ip_string += ".";
            }
        }
        auto port = (static_cast<uint16_t>((peersHash[i+4])) << 8) + static_cast<uint16_t>(peersHash[i+5]);
        std::unordered_map<std::string, std::string> peerMap;
        parsedPeers.push_back({ip_string, port});
    }
    return parsedPeers;
}
void get_peers(std::vector<Peer>& peers_, std::string info){
    for(size_t i = 0; i < info.size();i += 6){
        std::string Ip = "";
        int Port = 0;
        for(int j=0; j < 4;j++){
            Ip += std::to_string(uint8_t(static_cast<unsigned char>(info[i+j])));
            if(j!=3)
                Ip += '.';
        }
        Port += uint16_t(static_cast<unsigned char>(info[i + 4]));
        Port <<= 8;
        Port += uint16_t(static_cast<unsigned char>(info[i + 5]));
        Peer a;
        a.port = Port;
        a.ip = Ip;
        peers_.emplace_back(a);
    }
}