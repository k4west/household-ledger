#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "Transaction.h"
#include "json.hpp"

using json = nlohmann::json;

class AccountManager {
private:
    void ensureDataDir() {
        std::filesystem::create_directories(dataDir());
    }

    std::filesystem::path dataDir() const { return std::filesystem::path("data"); }

    std::filesystem::path dataFilePath() const {
        return dataDir() / "ledger.json";
    }

    std::vector<Transaction> ledger_;
    std::mutex mtx_;

public:
    void loadFromFile() {
        std::lock_guard<std::mutex> lock(mtx_);
        ensureDataDir();
        std::ifstream in(dataFilePath());
        if (!in.is_open()) {
            return;
        }
        json payload;
        in >> payload;
        ledger_ = payload.get<std::vector<Transaction>>();
    }
    
    void saveToFile() {
        ensureDataDir();
        json payload = ledger_;
        std::ofstream out(dataFilePath());
        out << payload.dump(2);
    }

    void addTransaction(const Transaction& transaction) {
        std::lock_guard<std::mutex> lock(mtx_);
        ledger_.push_back(transaction);
        saveToFile();
    }

    std::vector<Transaction> getAllTransactions() {
        std::lock_guard<std::mutex> lock(mtx_);
        return ledger_;
    }

    bool deleteTransaction(long long id) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = remove_if(ledger_.begin(), ledger_.end(),
            [id](const Transaction& tx) { return tx.id == id; });
        if (it == ledger_.end()) {
            return false;
        }
        ledger_.erase(it, ledger_.end());
        saveToFile();
        return true;
    }

    long long getBalance() {
        std::lock_guard<std::mutex> lock(mtx_);
        long long balance = 0;
        for (const auto& tx : ledger_) {
            if (tx.type == "income") {
                balance += tx.amount;
            } else {
                balance -= tx.amount;
            }
        }
        return balance;
    }
};
