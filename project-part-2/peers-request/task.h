#pragma once

#include "peer.h"
#include "torrent_file.h"
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <cpr/cpr.h>
#include "TorrentFileParser.h"
#include "iostream"
#include "parser.h"
#include "responsePeersParser.h"

class TorrentTracker {
public:
    /*
     * url - адрес трекера, берется из поля announce в .torrent-файле
     */
    explicit TorrentTracker(const std::string &url) : url_(url) {
    };

    /*
     * Получить список пиров у трекера и сохранить его для дальнейшей работы.
     * Запрос пиров происходит посредством HTTP GET запроса, данные передаются в формате bencode.
     * Такой же формат использовался в .torrent файле.
     * Подсказка: посмотрите, что было написано в main.cpp в домашнем задании torrent-file
     *
     * tf: структура с разобранными данными из .torrent файла из предыдущего домашнего задания.
     * peerId: id, под которым представляется наш клиент.
     * port: порт, на котором наш клиент будет слушать входящие соединения (пока что мы не слушаем и на этот порт никто
     *  не сможет подключиться).
     */
    void UpdatePeers(const TorrentFile &tf, std::string peerId, int port) {
        cpr::Response res = cpr::Get(
                cpr::Url{url_},
                cpr::Parameters{
                        {"info_hash",  tf.infoHash},
                        {"peer_id",    peerId},
                        {"port",       std::to_string(port)},
                        {"uploaded",   std::to_string(0)},
                        {"downloaded", std::to_string(0)},
                        {"left",       std::to_string(tf.length)},
                        {"compact",    std::to_string(1)}
                },
                cpr::Timeout{20000}
        );
        if (res.status_code != 200) {
            std::cerr << "Error: failed to connect to the tracker (status code " << res.status_code << ")"
                      << std::endl;
            return;
        }
        size_t pivot = 0;
        shared_ptr<BencodeDictionary> dict = dynamic_pointer_cast<BencodeDictionary>(parse_bencode(res.text.c_str(), res.text.size(), pivot));
        std::string peers = dynamic_pointer_cast<BencodeString>(dict->get("peers"))->get_str();
        std::vector<Peer> parsedPeers = parsePeers(peers);
        peers_ = std::move(parsedPeers);
    };

    /*
     * Отдает полученный ранее список пиров
     */
    const std::vector<Peer> &GetPeers() const {
        return peers_;
    };

private:
    std::string url_;
    std::vector<Peer> peers_;
};

TorrentFile LoadTorrentFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string contents = buffer.str();
    file.close();

    TorrentFile torrent;
    unsigned char infohash[20];
    if (parse_info_dict(contents, infohash, torrent.announce, torrent.comment, torrent.pieceHashes, torrent.pieceLength,
                        torrent.length)) {
        torrent.infoHash = std::string((char *) infohash, 20);
        return torrent;
    } else {
        throw std::runtime_error("Invalid torrent file");
    }
}
