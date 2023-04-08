#include "check_peers.h"
#include "torrent_tracker.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

std::string RandomString(size_t size) {
    return std::to_string(time(nullptr) % 10000);
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
