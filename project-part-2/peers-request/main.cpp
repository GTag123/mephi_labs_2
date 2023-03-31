#include "task.h"
#include "check_peers.h"
#include <cassert>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void RequestPeers(const TorrentFile& tf) {
    const std::string peerId = "TEST0APP1DONT2WORRY3";
    assert(peerId.size() == 20);

    std::cout << "Connecting to tracker " << tf.announce << std::endl;
    TorrentTracker tracker(tf.announce);
    tracker.UpdatePeers(tf, peerId, 12345);

    assert(!tracker.GetPeers().empty());

    std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
    for (const Peer& peer : tracker.GetPeers()) {
        std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
    }

    CheckPeers(tf, tracker.GetPeers());
}

void TestTorrentFile(const fs::path& file) {
    std::cout << 13232 << std::endl;
    TorrentFile tf = LoadTorrentFile(file);
    std::cout << "Loaded torrent file. " << tf.comment << std::endl;

    RequestPeers(tf);
}

int main() {
    for (const auto& entry : fs::directory_iterator("../resources")) {
        TestTorrentFile(entry.path());
    }
    return 0;
}
