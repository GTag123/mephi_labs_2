#include "task.h"
#include <cassert>
#include <iostream>
#include <cpr/cpr.h>
#include <filesystem>

namespace fs = std::filesystem;

void RequestPeers(const TorrentFile& tf) {
    const std::string peerId = "TEST0APP1DONT2WORRY3";
    assert(peerId.size() == 20);

    cpr::Response res = cpr::Get(
            cpr::Url{tf.announce},
            cpr::Parameters {
                    {"info_hash", tf.infoHash},
                    {"peer_id", peerId},
                    {"port", std::to_string(12345)},
                    {"uploaded", std::to_string(0)},
                    {"downloaded", std::to_string(0)},
                    {"left", std::to_string(tf.length)},
                    {"compact", std::to_string(1)}
            },
            cpr::Timeout{20000}
    );

    if (res.status_code == 200) {
        if (res.text.find("failure reason") == std::string::npos) {
            std::cout << "Successfully requested peers from " << tf.announce << std::endl;
            return;
        } else {
            std::cerr << "Server responded '" << res.text << "'" << std::endl;
        }
    }

    std::cerr << "Possibly you have parsed a .torrent file wrong hence the request for peers is wrong too" << std::endl;
    std::cerr << "Tried GET " << res.url << std::endl;
    std::cerr << "Error: " << (int)res.error.code << " - " << res.error.message << std::endl;
    if (res.error.code != cpr::ErrorCode::EMPTY_RESPONSE) {
        std::cerr << "Tracker response: " << res.status_code << " " << res.status_line << ": " << res.text << std::endl;
    }

    std::cerr << "Info hash: ";
    for (char c : tf.infoHash) {
        std::cerr << (unsigned int)(unsigned char)c << " ";
    }
    std::cerr << std::endl;

    exit(1);
}

void TestTorrentFile(const fs::path& file) {
    TorrentFile tf = LoadTorrentFile(file);
    std::cout << "Loaded torrent file. " << tf.comment << std::endl;

    RequestPeers(tf);
}

int main() {
    for (const auto& entry : fs::directory_iterator("resources")) {
        TestTorrentFile(entry.path());
    }
    return 0;
}
