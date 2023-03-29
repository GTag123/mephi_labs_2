#pragma once

#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include "iostream"
#include "BencodeHash.h"


struct TorrentFile {
    std::string announce;
    std::string comment;
    std::vector<std::string> pieceHashes;
    size_t pieceLength;
    size_t length;
    std::string name;
    std::string infoHash;
};

/*
 * Функция парсит .torrent файл и загружает информацию из него в структуру `TorrentFile`. Как устроен .torrent файл, можно
 * почитать в открытых источниках (например http://www.bittorrent.org/beps/bep_0003.html).
 * После парсинга файла нужно также заполнить поле `infoHash`, которое не хранится в файле в явном виде и должно быть
 * вычислено. Алгоритм вычисления этого поля можно найти в открытых источника, как правило, там же,
 * где описание формата .torrent файлов.
 * Данные из файла и infoHash будут использованы для запроса пиров у торрент-трекера. Если структура `TorrentFile`
 * была заполнена правильно, то трекер найдет нужную раздачу в своей базе и ответит списком пиров. Если данные неверны,
 * то сервер ответит ошибкой.
 */


TorrentFile LoadTorrentFile(const std::string& filename){
    std::ifstream file(filename, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string contents = buffer.str();
    file.close();

    TorrentFile torrent;
    size_t pivot = 0;
    shared_ptr<BencodeDictionary> dict = dynamic_pointer_cast<BencodeDictionary>(parse_bencode(contents.c_str(), contents.size(), pivot));
    torrent.announce = dynamic_pointer_cast<BencodeString>(dict->get("announce"))->get_str();
    torrent.comment = dynamic_pointer_cast<BencodeString>(dict->get("comment"))->get_str();
    torrent.pieceLength = dynamic_pointer_cast<BencodeInteger>(dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("piece length"))->get_int();
    torrent.length = dynamic_pointer_cast<BencodeInteger>(dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("length"))->get_int();
    string piecesHash = dynamic_pointer_cast<BencodeString>(dynamic_pointer_cast<BencodeDictionary>(dict->get("info"))->get("pieces"))->get_str();
    for (int i = 0; i < piecesHash.length(); i += 20) {
        torrent.pieceHashes.push_back(piecesHash.substr(i, 20));
    }
    torrent.infoHash = hashing(*dict);
    return torrent;
}
