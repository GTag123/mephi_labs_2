#include "check_peers.h"
#include "torrent_tracker.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

std::string RandomString(size_t length) {
    std::random_device random;
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(random() % ('Z' - 'A') + 'A');
    }
    return result;
}

const std::string PeerId = "TESTAPPDONTWORRY" + RandomString(4);

void RequestPeers(const TorrentFile& tf) {
    assert(PeerId.size() == 20);

    std::cout << "Connecting to tracker " << tf.announce << std::endl;
    TorrentTracker tracker(tf.announce);
    tracker.UpdatePeers(tf, PeerId, 12345);

    assert(!tracker.GetPeers().empty());

    std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
    for (const Peer& peer : tracker.GetPeers()) {
        std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
    }

    CheckPeers(tf, tracker.GetPeers());
}

void TestTorrentFile(const fs::path& file) {
    TorrentFile tf;
    try {
        tf = LoadTorrentFile(file);
        std::cout << "Loaded torrent file " << file << ". " << tf.comment << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return;
    }

    RequestPeers(tf);
}

int main() {
    for (const auto& entry : fs::directory_iterator("resources")) {
        TestTorrentFile(entry.path());
    }
    return 0;
}
