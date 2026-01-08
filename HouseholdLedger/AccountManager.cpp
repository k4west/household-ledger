#include "AccountManager.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>

#include "json.hpp"

using json = nlohmann::json;

void AccountManager::ensureDataDir() const {
    std::filesystem::create_directories(dataDir());
}

std::filesystem::path AccountManager::dataDir() const {
    return std::filesystem::path("data");
}

std::filesystem::path AccountManager::ledgerYearDir(int year) const {
    std::ostringstream dirname;
    dirname << "ledger_" << std::setw(4) << std::setfill('0') << year;
    return dataDir() / dirname.str();
}

std::filesystem::path AccountManager::ledgerFilePath(int year, int month) const {
    std::ostringstream filename;
    filename << "ledger_" << std::setw(4) << std::setfill('0') << year << "-"
             << std::setw(2) << std::setfill('0') << month << ".json";
    return ledgerYearDir(year) / filename.str();
}

bool AccountManager::parseYearMonthFromFilename(const std::filesystem::path& path, int& year, int& month) const {
    auto name = path.filename().string();
    if (name.rfind("ledger_", 0) != 0 || path.extension() != ".json") {
        return false;
    }
    if (name.size() < 15) {
        return false;
    }
    try {
        year = std::stoi(name.substr(7, 4));
        month = std::stoi(name.substr(12, 2));
    } catch (const std::exception&) {
        return false;
    }
    auto parentName = path.parent_path().filename().string();
    if (parentName.rfind("ledger_", 0) == 0 && parentName.size() >= 11) {
        try {
            int dirYear = std::stoi(parentName.substr(7, 4));
            if (dirYear != year) {
                return false;
            }
        } catch (const std::exception&) {
            return false;
        }
    }
    return year > 0 && month >= 1 && month <= 12;
}

std::vector<Transaction> AccountManager::readLedgerFile(const std::filesystem::path& path) const {
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    json payload;
    in >> payload;
    return payload.get<std::vector<Transaction>>();
}

void AccountManager::writeLedgerFile(const std::filesystem::path& path,
                                     const std::vector<Transaction>& ledger) const {
    std::filesystem::create_directories(path.parent_path());
    json payload = ledger;
    std::ofstream out(path);
    out << payload.dump(2, ' ', false, json::error_handler_t::replace);
}

std::vector<std::filesystem::path> AccountManager::listLedgerFiles() const {
    std::vector<std::filesystem::path> files;
    ensureDataDir();
    for (const auto& entry : std::filesystem::directory_iterator(dataDir())) {
        if (!entry.is_directory()) {
            continue;
        }
        const auto& dir = entry.path();
        const auto dirName = dir.filename().string();
        if (dirName.rfind("ledger_", 0) != 0) {
            continue;
        }
        for (const auto& fileEntry : std::filesystem::directory_iterator(dir)) {
            if (!fileEntry.is_regular_file()) {
                continue;
            }
            const auto& path = fileEntry.path();
            const auto name = path.filename().string();
            if (name.rfind("ledger_", 0) == 0 && path.extension() == ".json") {
                files.push_back(path);
            }
        }
    }
    return files;
}

bool AccountManager::parseYearMonth(const std::string& date, int& year, int& month) const {
    if (date.size() < 7) {
        return false;
    }
    try {
        year = std::stoi(date.substr(0, 4));
        month = std::stoi(date.substr(5, 2));
    } catch (const std::exception&) {
        return false;
    }
    return year > 0 && month >= 1 && month <= 12;
}

std::vector<Transaction> AccountManager::getTransactionsForMonth(int year, int month) {
    std::lock_guard<std::mutex> lock(mtx_);
    return readLedgerFile(ledgerFilePath(year, month));
}

std::vector<Transaction> AccountManager::getTransactionsForYear(int year) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<Transaction> all;
    for (const auto& file : listLedgerFiles()) {
        int fileYear = 0;
        int fileMonth = 0;
        if (!parseYearMonthFromFilename(file, fileYear, fileMonth)) {
            continue;
        }
        if (fileYear != year) {
            continue;
        }
        auto ledger = readLedgerFile(file);
        all.insert(all.end(), ledger.begin(), ledger.end());
    }
    return all;
}

std::vector<Transaction> AccountManager::getAllTransactions() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<Transaction> all;
    for (const auto& file : listLedgerFiles()) {
        auto ledger = readLedgerFile(file);
        all.insert(all.end(), ledger.begin(), ledger.end());
    }
    return all;
}

void AccountManager::addTransaction(const Transaction& transaction) {
    std::lock_guard<std::mutex> lock(mtx_);
    int year = 0;
    int month = 0;
    if (!parseYearMonth(transaction.date, year, month)) {
        year = 0;
        month = 0;
    }
    auto path = ledgerFilePath(year, month);
    auto ledger = readLedgerFile(path);
    ledger.push_back(transaction);
    writeLedgerFile(path, ledger);
}

bool AccountManager::deleteTransaction(long long id) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& file : listLedgerFiles()) {
        auto ledger = readLedgerFile(file);
        auto it = std::remove_if(ledger.begin(), ledger.end(),
                                 [id](const Transaction& tx) { return tx.id == id; });
        if (it == ledger.end()) {
            continue;
        }
        ledger.erase(it, ledger.end());
        writeLedgerFile(file, ledger);
        return true;
    }
    return false;
}

bool AccountManager::updateTransaction(const Transaction& transaction) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::optional<std::filesystem::path> sourceFile;
    std::vector<Transaction> sourceLedger;
    for (const auto& file : listLedgerFiles()) {
        auto ledger = readLedgerFile(file);
        auto it = std::find_if(ledger.begin(), ledger.end(),
                               [id = transaction.id](const Transaction& tx) { return tx.id == id; });
        if (it == ledger.end()) {
            continue;
        }
        ledger.erase(it);
        sourceFile = file;
        sourceLedger = std::move(ledger);
        break;
    }

    if (!sourceFile) {
        return false;
    }

    int year = 0;
    int month = 0;
    std::filesystem::path targetFile = *sourceFile;
    if (parseYearMonth(transaction.date, year, month)) {
        targetFile = ledgerFilePath(year, month);
    }

    if (targetFile == *sourceFile) {
        sourceLedger.push_back(transaction);
        writeLedgerFile(*sourceFile, sourceLedger);
        return true;
    }

    writeLedgerFile(*sourceFile, sourceLedger);
    auto targetLedger = readLedgerFile(targetFile);
    targetLedger.push_back(transaction);
    writeLedgerFile(targetFile, targetLedger);
    return true;
}
