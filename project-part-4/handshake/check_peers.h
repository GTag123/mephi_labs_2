#pragma once

#include "peer_connect.h"
#include "torrent_file.h"
#include "peer.h"
#include <iostream>
#include <unordered_set>
#include <cassert>

extern const std::string PeerId;

void CheckPeers(const TorrentFile& tf, const std::vector<Peer>& peers) {
    // При посылке домашнего задания тут будет настоящая проверка.
    // Проверка заключается в том, чтобы успешно обмениваться сообщениями с пирами после хендшейка
    static std::vector<std::string> infoHashes;

    infoHashes.push_back(tf.infoHash);
    std::unordered_set<std::string> unique(infoHashes.begin(), infoHashes.end());
    assert(unique.size() == infoHashes.size());

    size_t successfulConnections = 0;
    for (const Peer& peer : peers) {
        PeerConnect peerConnect(peer, tf, PeerId);
        try {
            peerConnect.Run();
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            continue;
        }
        ++successfulConnections;
    }

    std::cout << "Made " << successfulConnections << " successful connections" << std::endl;

    assert(successfulConnections >= 0.2 * peers.size());
}