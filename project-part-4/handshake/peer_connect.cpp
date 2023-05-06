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
    size_t size = 0;
    for (char c : bitfield_) {
        size += __builtin_popcount(c);
    }
    return size;
}

PeerConnect::PeerConnect(const Peer &peer, const TorrentFile &tf, std::string selfPeerId) :
        tf_(tf),
        socket_(peer.ip, peer.port, 500ms, 500ms),
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

void PeerConnect::PerformHandshake() {
    this->socket_.EstablishConnection();
    std::string handshake = "BitTorrent protocol00000000" + this->tf_.infoHash +
                            this->selfPeerId_;
    handshake = ((char) 19) + handshake;
    this->socket_.SendData(handshake);
    std::string response = this->socket_.ReceiveData(68);
    if (response[0] != '\x13' || response.substr(1, 19) != "BitTorrent protocol") {
        throw std::runtime_error("Handshake failed");
    }
    if (response.substr(28, 20) != this->tf_.infoHash) {
        throw std::runtime_error("Peer infohash another");
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

void PeerConnect::ReceiveBitfield() {
    std::string response = this->socket_.ReceiveData();
    if ((int) response[0] == 20) {
        response = this->socket_.ReceiveData();
    }

    MessageId id = static_cast<MessageId>((int) response[0]);

    if (id == MessageId::BitField) {
        std::string bitfield = response.substr(1, response.length() - 1);
        this->piecesAvailability_ = PeerPiecesAvailability(bitfield);
    } else if (id == MessageId::Unchoke) {
        this->choked_ = false;
    } else {
        throw std::runtime_error("Wrong BitField message");
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
