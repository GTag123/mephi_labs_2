#include "piece_storage.h"
#include <iostream>

PieceStorage::PieceStorage(const TorrentFile &tf) {
    for (size_t i = 0; i < tf.length / tf.pieceLength; ++i) {
        size_t length = (i == tf.length / tf.pieceLength - 1) ? tf.length % tf.pieceLength : tf.pieceLength;
        remainPieces_.push(std::make_shared<Piece>(i, length, tf.pieceHashes[i]));
    }
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    if (remainPieces_.empty()) {
        return nullptr;
    }
    auto piece = remainPieces_.front();
    remainPieces_.pop();
    return piece;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    // хз, что будет если пир постоянно будет отправлять бракованный кусок,peer_connect будет же крутиться в бесконечном цикле
    if (!piece->HashMatches()) {
        piece->Reset();
        std::cerr << "Piece " << piece->GetIndex() << " hash doesn't match" << std::endl;
        return;
    }
    SavePieceToDisk(piece);
    while (!remainPieces_.empty()) {
        remainPieces_.pop();
    }
}

bool PieceStorage::QueueIsEmpty() const {
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    // TODO хз тут оставшихся частей или всего?
    return remainPieces_.size();
}

void PieceStorage::SavePieceToDisk(PiecePtr piece) {
    // Эта функция будет переопределена при запуске вашего решения в проверяющей системе
    // Вместо сохранения на диск там пока что будет проверка, что часть файла скачалась правильно
    std::cout << "Downloaded piece " << piece->GetIndex() << std::endl;
    std::cerr << "Clear pieces list, don't want to download all of them" << std::endl;
    while (!remainPieces_.empty()) {
        remainPieces_.pop();
    }
}
