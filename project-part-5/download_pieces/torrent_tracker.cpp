#include "torrent_tracker.h"
#include "bencode.h"
#include "byte_tools.h"
#include <cpr/cpr.h>

const std::vector<Peer> &TorrentTracker::GetPeers() const {
    return peers_;
}
