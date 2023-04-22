#pragma once

#include "peer_connect.h"
#include "piece_storage.h"
#include "torrent_file.h"
#include "peer.h"
#include <iostream>
#include <unordered_set>
#include <cassert>

extern const std::string PeerId;

void CheckPeers(const TorrentFile& tf, const std::vector<Peer>& peers) {
    static std::vector<std::string> infoHashes;

    infoHashes.push_back(tf.infoHash);
    std::unordered_set<std::string> unique(infoHashes.begin(), infoHashes.end());
    assert(unique.size() == infoHashes.size());

    PieceStorage pieces(tf);
    if (pieces.QueueIsEmpty()) {
        std::cerr << "Torrent file has been parsed wrong. No pieces described" << std::endl;
    }
    assert(!pieces.QueueIsEmpty());

    for (const Peer& peer : peers) {
        PeerConnect peerConnect(peer, tf, PeerId, pieces);
        try {
            peerConnect.Run();
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            continue;
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            continue;
        } catch (...) {
            std::cerr << "Unknown error" << std::endl;
            continue;
        }

        if (pieces.QueueIsEmpty()) {
            std::cout << "Successfully downloaded a piece of file" << std::endl;
            break;
        }
    }
    assert(pieces.QueueIsEmpty());
}