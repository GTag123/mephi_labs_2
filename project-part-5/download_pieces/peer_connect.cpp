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
    for (char c: bitfield_) {
        size += __builtin_popcount(c);
    }
    return size;
}

PeerConnect::PeerConnect(const Peer &peer, const TorrentFile &tf, std::string selfPeerId, PieceStorage &pieceStorage) :
        tf_(tf),
        socket_(peer.ip, peer.port, 500ms, 500ms),
        selfPeerId_(std::move(selfPeerId)),
        // peer id?
        terminated_(false),
        choked_(true),
        pieceStorage_(pieceStorage) {}

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
    while (!terminated_) {
        auto message = socket_.ReceiveData();
        if (message.empty()) {
            continue;
        }

        // Обрабатываем сообщение
        MessageId id = static_cast<MessageId>(message[0]);
        std::string payload = message.substr(1, message.size() - 1);
        switch (id) {
            case MessageId::Have:
            {
                std::cout << "Have" << std::endl;
                size_t pieceIndex = BytesToInt(payload.substr(0, 4));
                piecesAvailability_.SetPieceAvailability(pieceIndex);
            }
                break;
            case MessageId::Piece:
            {
//                std::cout << "Piece" << std::endl;
                auto pieceIndex = BytesToInt(payload.substr(0, 4));
                auto offset = BytesToInt(payload.substr(4, 4));
                auto data = payload.substr(8, payload.size() - 8);
                if (pieceInProgress_ && pieceInProgress_->GetIndex() == pieceIndex) {
                    // надо бы чекать хэш и размер даты
                    pieceInProgress_->SaveBlock(offset, data);
                    if (pieceInProgress_->AllBlocksRetrieved()) {
                        Terminate();
//                        pieceInProgress_ = nullptr;
                    }
                }
                pendingBlock_ = false;
            }
                break;
            case MessageId::Choke:
                std::cout << "Choke" << std::endl;
                Terminate();
                break;
            case MessageId::Unchoke:
                std::cout << "Unchoke" << std::endl;
                choked_ = false;
                break;
            default:
                std::cout << "Default" << std::endl;
                break;
        }
        if (!choked_ && !pendingBlock_) {
            RequestPiece();
        }
    }
}

/*
     * Функция отправляет пиру сообщение типа request. Это сообщение обозначает запрос части файла у пира.
     * За одно сообщение запрашивается не часть целиком, а блок данных размером 2^14 байт или меньше.
     * Если в данный момент мы не знаем, какую часть файла надо запросить у пира, то надо получить эту информацию у
     * PieceStorage
     */
void PeerConnect::RequestPiece() {
    std::string request = IntToBytes(13) + static_cast<char>(MessageId::Request);
    if (!pieceInProgress_) {
        auto piece = pieceStorage_.GetNextPieceToDownload();
//        while (piece != nullptr && !piecesAvailability_.IsPieceAvailable(piece->GetIndex())) {
//            piece = pieceStorage_.GetNextPieceToDownload();
//        }
        pieceInProgress_ = piece;
        if (!pieceInProgress_) {
            return;
        }
    }
    if (!pendingBlock_) {
        pendingBlock_ = true;
        auto block = pieceInProgress_->FirstMissingBlock();
        if (block) {
            request += IntToBytes(pieceInProgress_->GetIndex()) + IntToBytes(block->offset) + IntToBytes(block->length);
            socket_.SendData(request);
        } else {
            pieceInProgress_ = nullptr;
            pendingBlock_ = false;
        }
    }
}