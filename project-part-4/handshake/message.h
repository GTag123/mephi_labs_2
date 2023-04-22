#pragma once

#include <cstdint>
#include <string>

/*
 * Тип сообщения в протоколе торрента.
 * https://wiki.theory.org/BitTorrentSpecification#Messages
 */
enum class MessageId : uint8_t {
    Choke = 0,
    Unchoke,
    Interested,
    NotInterested,
    Have,
    BitField,
    Request,
    Piece,
    Cancel,
    Port,
    KeepAlive,
};

//struct Message {
//    MessageId id;
//    size_t messageLength;
//    std::string payload;
//
//    /*
//     * Выделяем тип сообщения и длину и создаем объект типа Message.
//     * Подразумевается, что здесь в качестве `messageString` будет приниматься строка, прочитанная из TCP-сокета
//     */
//    static Message Parse(const std::string& messageString) {
//        MessageId id = static_cast<MessageId>(messageString[4]);
//        size_t messageLength = 0;
//        for (int i = 0; i < 4; ++i) {
//            messageLength = messageLength * 256 + static_cast<uint8_t>(messageString[i]);
//        }
//        return {id, messageLength, messageString.substr(5)};
//    };
//
//    /*
//     * Создаем сообщение с заданным типом и содержимым. Длина вычисляется автоматически
//     */
//    static Message Init(MessageId id, const std::string& payload) {
//        return {id, payload.size(), payload};
//    };
//
//    /*
//     * Формируем строку с сообщением, которую можно будет послать пиру в соответствии с протоколом.
//     * Получается строка вида "<1 + payload length><message id><payload>"
//     * Секция с длиной сообщения занимает 4 байта и представляет собой целое число в формате big-endian
//     * id сообщения занимает 1 байт и может принимать значения от 0 до 9 включительно
//     */
//    std::string ToString() const;
//};