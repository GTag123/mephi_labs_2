#pragma once

#include "torrent_file.h"
#include "peer.h"

void CheckPeers(const TorrentFile& tf, const std::vector<Peer>& peers) {
    // При посылке домашнего задания тут будет настоящая проверка.
    // Проверка заключается в том, чтобы подключиться к пирам с помощью реализованного студентом класса `TcpConnect`
}