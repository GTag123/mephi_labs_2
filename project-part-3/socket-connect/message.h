#pragma once

#include <cstdint>
#include <string>

/*
 * Тип сообщения в протоколе торрента.
 * https://wiki.theory.org/BitTorrentSpecification#Messages
 */
enum class MessageId : uint8_t {
    Choke = 0,
    Unchoke = 1,
    Interested = 2,
    NotInterested = 3,
    Have = 4,
    BitField = 5,
    Request = 6,
    Piece = 7,
    Cancel = 8,
    Port = 9,
    KeepAlive = (uint8_t)-1,
};

struct Message {
    MessageId id;
    size_t messageLength;
    std::string payload;

    /*
     * Выделяем тип сообщения и длину и создаем объект типа Message
     */
    static Message Parse(const std::string& messageString);

    /*
     * Создаем сообщение с заданным типом и содержимым. Длина вычисляется автоматически
     */
    static Message Init(MessageId id, const std::string& payload);

    /*
     * Формируем строку с сообщением, которую можно будет послать пиру в соответствии с протоколом
     */
    std::string ToString() const;
};
