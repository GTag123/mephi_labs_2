#include "piece_storage.h"
#include <iostream>

PieceStorage::PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory) :
    tf_(tf),
    outputDirectory_(outputDirectory),
    out_(outputDirectory_ / tf_.name, std::ios::binary | std::ios::in | std::ios::out),
    isOutputFileOpen_(true) {
    for (size_t i = 0; i < tf.length / tf.pieceLength; ++i) {
        size_t length = (i == tf.length / tf.pieceLength - 1) ? tf.length % tf.pieceLength : tf.pieceLength;
        remainPieces_.push(std::make_shared<Piece>(i, length, tf.pieceHashes[i]));
    }
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (remainPieces_.empty()) {
        return nullptr;
    }
    auto piece = remainPieces_.front();
    remainPieces_.pop();
    return piece;
}
void PieceStorage::AddPieceToQueue(const PiecePtr &piece) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    remainPieces_.push(piece);
}
void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    // хз, что будет если пир постоянно будет отправлять бракованный кусок,peer_connect будет же крутиться в бесконечном цикле
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (!piece->HashMatches()) {
        piece->Reset();
        std::cerr << "Piece " << piece->GetIndex() << " hash doesn't match" << std::endl;
        return;
    }
    SavePieceToDiskImpl(piece);
}

bool PieceStorage::QueueIsEmpty() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return remainPieces_.empty();
}

size_t PieceStorage::TotalPiecesCount() const {
    // TODO хз тут оставшихся частей или всего?
//    return remainPieces_.size();
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return tf_.length / tf_.pieceLength;
}

size_t PieceStorage::PiecesInProgressCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return TotalPiecesCount() - remainPieces_.size();
}

void PieceStorage::CloseOutputFile() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (!isOutputFileOpen_) {
        std::cerr << "Output file is already closed" << std::endl;
        return;
    }
    out_.close();
    isOutputFileOpen_ = false;
}

const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const { // проблемный метод
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::cout << "---GetPiecesSavedToDiscIndices---" << std::endl;
    return piecesSavedToDiscIndicesVector_;
}

size_t PieceStorage::PiecesSavedToDiscCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return piecesSavedToDiscIndicesSet_.size();
}

void PieceStorage::SavePieceToDisk(const PiecePtr &piece) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    SavePieceToDiskImpl(piece);
}

void PieceStorage::SavePieceToDiskImpl(const PiecePtr &piece) {
    if (!isOutputFileOpen_) {
        std::cerr << "Output file is already closed" << std::endl;
        return;
    }
    if(piecesSavedToDiscIndicesSet_.find(piece->GetIndex()) != piecesSavedToDiscIndicesSet_.end()) {
        std::cerr << "Piece " << piece->GetIndex() << " is already saved to disc" << std::endl;
        return;
    }
    piecesSavedToDiscIndicesSet_.insert(piece->GetIndex());
    piecesSavedToDiscIndicesVector_.push_back(piece->GetIndex());
    out_.seekp(piece->GetIndex() * tf_.pieceLength);
    out_.write(piece->GetData().data(), piece->GetData().size());
}
