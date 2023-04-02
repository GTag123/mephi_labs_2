#pragma once

#include "iostream"
#include "peer.h"
#include "torrent_file.h"
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <cpr/cpr.h>
#include "getInfoHash.h"
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
        if (res.text.find("failure reason") != std::string::npos) {
            std::cerr << "Error. Server responded '" << res.text << "'" << std::endl;
            return;
        }

        size_t pivot = 0;
        std::shared_ptr<BencodeDictionary> dict = std::dynamic_pointer_cast<BencodeDictionary>(parse_bencode(res.text.c_str(), res.text.size(), pivot));
        std::string peers = std::dynamic_pointer_cast<BencodeString>(dict->get("peers"))->get_str();
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

TorrentFile LoadTorrentFile(const std::string& filename){
    std::ifstream file(filename, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string contents = buffer.str();
    file.close();

    TorrentFile torrent;
    size_t pivot = 0;
    std::shared_ptr<BencodeDictionary> dict = std::dynamic_pointer_cast<BencodeDictionary>(parse_bencode(contents.c_str(), contents.size(), pivot));
    torrent.announce = std::dynamic_pointer_cast<BencodeString>(dict->get("announce"))->get_str();
    torrent.comment = std::dynamic_pointer_cast<BencodeString>(dict->get("comment"))->get_str();
    torrent.pieceLength = std::dynamic_pointer_cast<BencodeInteger>(std::dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("piece length"))->get_int();
    torrent.length = std::dynamic_pointer_cast<BencodeInteger>(std::dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("length"))->get_int();
    std::string piecesHash = std::dynamic_pointer_cast<BencodeString>(std::dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("pieces"))->get_str();
    for (int i = 0; i < piecesHash.length(); i += 20) {
        torrent.pieceHashes.push_back(piecesHash.substr(i, 20));
    }
    torrent.infoHash = hashing(*dict);
    return torrent;
}
