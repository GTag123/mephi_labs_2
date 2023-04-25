#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>

using namespace std::chrono_literals;

PeerPiecesAvailability::PeerPiecesAvailability() {}

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) : bitfield_(std::move(bitfield)) {}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const {
    return bitfield_[pieceIndex / 8] & (1 << (7 - pieceIndex % 8));
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex) {
    bitfield_[pieceIndex / 8] |= (1 << (7 - pieceIndex % 8));
}

size_t PeerPiecesAvailability::Size() const {
    return bitfield_.size() * 8;
}

PeerConnect::PeerConnect(const Peer &peer, const TorrentFile &tf, std::string selfPeerId) :
        tf_(tf),
        socket_(peer.ip, peer.port, 300ms, 300ms),
        selfPeerId_(std::move(selfPeerId)),
        terminated_(false),
        choked_(true) {}

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}

/*
     * Функция производит handshake.
     * - Подключиться к пиру по протоколу TCP
     * - Отправить пиру сообщение handshake
     * - Проверить правильность ответа пира
     * https://wiki.theory.org/BitTorrentSpecification#Handshake
     */
void PeerConnect::PerformHandshake() {
    this->socket_.EstablishConnection();
    std::string handshake = "BitTorrent protocol00000000" + this->tf_.infoHash +
            CalculateSHA1(this->selfPeerId_);
    handshake = ((char) 19) + handshake;
    this->socket_.SendData(handshake);
    std::cout << handshake << std::endl;
    std::string response = this->socket_.ReceiveData(68); // ждёт 68 чаров или нихуя?
    std::cout << "123dwefswfsdfsf" << std::endl;
    if (response[0] != '\x13' || response.substr(1, 19) != "BitTorrent protocol") {
        std::cout << response << std::endl;
        throw std::runtime_error("Handshake failed");
    }
}

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
                  socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}

/*
     * Функция читает из сокета bitfield с информацией о наличии у пира различных частей файла.
     * Полученную информацию надо сохранить в поле `piecesAvailability_`.
     * Также надо учесть, что сообщение тип Bitfield является опциональным, то есть пиры необязательно будут слать его.
     * Вместо этого они могут сразу прислать сообщение Unchoke, поэтому надо быть готовым обработать его в этой функции.
     * Обработка сообщения Unchoke заключается в выставлении флага `choked_` в значение `false`
*/
void PeerConnect::ReceiveBitfield() {
    std::string response = this->socket_.ReceiveData(5);
    size_t length = BytesToInt(response.substr(0, 4));
    MessageId id = static_cast<MessageId>(response[4]);
    if (id == MessageId::BitField) {
        std::string bitfield = this->socket_.ReceiveData(length - 1);
        this->piecesAvailability_ = PeerPiecesAvailability(bitfield);
    } else if (id == MessageId::Unchoke) {
        this->choked_ = false;
    }
}

void PeerConnect::SendInterested() {
    std::string interested = IntToBytes(1) + static_cast<char>(MessageId::Interested);
    this->socket_.SendData(interested);
}

void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

void PeerConnect::MainLoop() {
    /*
     * При проверке вашего решения на сервере этот метод будет переопределен.
     * Если вы провели handshake верно, то в этой функции будет работать обмен данными с пиром
     */
    std::cout << "Dummy main loop" << std::endl;
    Terminate();
}
