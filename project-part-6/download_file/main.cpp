#include "torrent_tracker.h"
#include "piece_storage.h"
#include "peer_connect.h"
#include "byte_tools.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <random>
#include <thread>
#include <algorithm>
#include "mutex"
#include "atomic"
namespace fs = std::filesystem;
std::atomic<int> cntstart = 0;
std::atomic<int> cntbegin = 0;
std::atomic<int> cntend = 0;
std::mutex mtx;
int counter = 0;
std::atomic<int> peerconnectsremais = 0;
std::string RandomString(size_t length) {
    std::random_device random;
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(random() % ('Z' - 'A') + 'A');
    }
    return result;
}

const std::string PeerId = "TESTAPPDONTWORRY" + RandomString(4);
constexpr size_t PiecesToDownload = 20;

void CheckDownloadedPiecesIntegrity(const std::filesystem::path& outputFilename, const TorrentFile& tf, PieceStorage& pieces) {
    pieces.CloseOutputFile();
    std::cout << "Check path: " << outputFilename.string() << std::endl;
    if (std::filesystem::file_size(outputFilename) != tf.length) {
        std::cerr << "Wrong size: " << std::filesystem::file_size(outputFilename) << "\nWaited size: " << tf.length << std::endl;
        throw std::runtime_error("Output file has wrong size");
    }

    if (pieces.GetPiecesSavedToDiscIndices().size() != pieces.PiecesSavedToDiscCount()) {
        throw std::runtime_error("Cannot determine real amount of saved pieces");
    }

    if (pieces.PiecesSavedToDiscCount() < PiecesToDownload) {
        throw std::runtime_error("Downloaded pieces amount is not enough");
    }

    if (pieces.TotalPiecesCount() != tf.pieceHashes.size() || pieces.TotalPiecesCount() < 200) {
        throw std::runtime_error("Wrong amount of pieces");
    }

    std::vector<size_t> pieceIndices = pieces.GetPiecesSavedToDiscIndices();
    std::sort(pieceIndices.begin(), pieceIndices.end());

    std::ifstream file(outputFilename, std::ios_base::binary);
    for (size_t pieceIndex : pieceIndices) {
        const std::streamoff positionInFile = pieceIndex * tf.pieceLength;
        file.seekg(positionInFile);
        if (!file.good()) {
            throw std::runtime_error("Cannot read from file");
        }
        std::string pieceDataFromFile(tf.pieceLength, '\0');
        file.read(pieceDataFromFile.data(), tf.pieceLength);
        const size_t readBytesCount = file.gcount();
        pieceDataFromFile.resize(readBytesCount);
        const std::string realHash = CalculateSHA1(pieceDataFromFile);
        if (realHash != tf.pieceHashes[pieceIndex]) {
            std::cerr << "File piece with index " << pieceIndex << " started at position " << positionInFile <<
                      " with length " << pieceDataFromFile.length() << " has wrong hash " << HexEncode(realHash) <<
                      ". Expected hash is " << HexEncode(tf.pieceHashes[pieceIndex]) << std::endl;
            throw std::runtime_error("Wrong piece hash");
        }
//        std::cout << "Correct data:\n" << pieceDataFromFile << std::endl;
    }
}

void DeleteDownloadedFile(const std::filesystem::path& outputFilename) {
    std::filesystem::remove(outputFilename);
}

std::filesystem::path PrepareDownloadDirectory(const std::string& randomString) {
    std::filesystem::path outputDirectory = "./downloads";
    outputDirectory /=  randomString;
    std::filesystem::create_directories(outputDirectory);
    return outputDirectory;
}

bool RunDownloadMultithread(PieceStorage& pieces, const TorrentFile& torrentFile, const std::string& ourId, const TorrentTracker& tracker) {
    using namespace std::chrono_literals;

    std::vector<std::thread> peerThreads;
    std::vector<PeerConnect> peerConnections;

    for (const Peer& peer : tracker.GetPeers()) {
        peerConnections.emplace_back(peerconnectsremais, peer, torrentFile, ourId, pieces);
    }
    std::cout << "Created " << peerConnections.size() << " peer connections" << std::endl;
    for (PeerConnect& peerConnect : peerConnections) {
        peerThreads.emplace_back(
                [&peerConnect] () {
                    bool tryAgain = true;
                    int attempts = 0;
                    do {
                        try {
                            ++attempts;
                            mtx.lock();
                            std::cout << std::endl << "Ip: " << peerConnect.peerinfo.ip << " Port: " << peerConnect.peerinfo.port << std::endl;
                            mtx.unlock();
                            peerConnect.Run(cntstart, cntend, cntbegin);
                            mtx.lock();
                            std::cout << "Run finished " << counter << std::endl << std::endl;
                            mtx.unlock();
                        } catch (const std::runtime_error& e) {
                            std::cerr << "Runtime error: " << e.what() << std::endl;
                        } catch (const std::exception& e) {
                            std::cerr << "Exception: " << e.what() << std::endl;
                        } catch (...) {
                            std::cerr << "Unknown error" << std::endl;
                        }
                        tryAgain = peerConnect.Failed() && attempts < 3;
                    } while (tryAgain);
                    mtx.lock();
                    counter++;
//                    std::cout << std::endl << "---------!!!!!Peer thread finished. Total finished: " << counter << std::endl;
                    mtx.unlock();
                }
        );
    }

    std::this_thread::sleep_for(10s);
    while (pieces.PiecesSavedToDiscCount() < PiecesToDownload) {
        if (pieces.PiecesInProgressCount() == 0) {
            std::cerr << "Want to download more pieces but all peer connections are not working. Let's request new peers" << std::endl;
            int termcount = 0;
            for (PeerConnect& peerConnect : peerConnections) {
                termcount++;
                std::cout << "Terminate count: " << termcount << std::endl;
                peerConnect.Terminate();
            }
            for (std::thread& thread : peerThreads) {
                thread.join();
            }
            return true;
        }
        std::cerr << "WHILE WHILE WHILE" << std::endl;
        std::this_thread::sleep_for(1s);
    }
    std::cout << "!!!!!!!!!!!-----------------------" << std::endl << std::endl;
    for (PeerConnect& peerConnect : peerConnections) {
        peerConnect.Terminate();
    }

    for (std::thread& thread : peerThreads) {
        thread.join();
    }

    return false;
}

void DownloadTorrentFile(const TorrentFile& torrentFile, PieceStorage& pieces, const std::string& ourId) {
    std::cout << "Connecting to tracker " << torrentFile.announce << std::endl;
    TorrentTracker tracker(torrentFile.announce);
    bool requestMorePeers = false;
    do {
        tracker.UpdatePeers(torrentFile, ourId, 12345);

        if (tracker.GetPeers().empty()) {
            std::cerr << "No peers found. Cannot download a file" << std::endl;
        }

        std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
        for (const Peer& peer : tracker.GetPeers()) {
            std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
        }

        requestMorePeers = RunDownloadMultithread(pieces, torrentFile, ourId, tracker);
    } while (requestMorePeers);
}

void TestTorrentFile(const fs::path& file) {
    TorrentFile torrentFile;
    try {
        torrentFile = LoadTorrentFile(file);
        std::cout << "Loaded torrent file " << file << ". " << torrentFile.comment << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return;
    }

    const std::filesystem::path outputDirectory = PrepareDownloadDirectory(PeerId);
    PieceStorage pieces(torrentFile, outputDirectory);

    DownloadTorrentFile(torrentFile, pieces, PeerId);
    std::cout << "Downloaded" << std::endl;
    CheckDownloadedPiecesIntegrity(outputDirectory / torrentFile.name, torrentFile, pieces);
    std::cout << "Checked" << std::endl;
    DeleteDownloadedFile(outputDirectory / torrentFile.name);
}

int main() {
    for (const auto& entry : fs::directory_iterator("resources")) {
        TestTorrentFile(entry.path());
    }
    return 0;
}
