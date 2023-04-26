#include "piece_storage.h"
#include <iostream>

PiecePtr PieceStorage::GetNextPieceToDownload() {
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
}

bool PieceStorage::QueueIsEmpty() const {
}

size_t PieceStorage::TotalPiecesCount() const {
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
