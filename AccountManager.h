#ifndef ACCOUNT_MANAGER_H
#define ACCOUNT_MANAGER_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <vector>

#include "Transaction.h"
#include "json.hpp"

class AccountManager {
 public:
  AccountManager() = default;

  void loadFromFile() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::filesystem::create_directories(dataDirectory());

    std::ifstream input(filePath());
    if (!input.is_open()) {
      return;
    }

    nlohmann::json data;
    input >> data;
    ledger_ = data.get<std::vector<Transaction>>();
  }

  void saveToFile() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::filesystem::create_directories(dataDirectory());

    std::ofstream output(filePath());
    nlohmann::json data = ledger_;
    output << data.dump(2);
  }

  void addTransaction(const Transaction &transaction) {
    std::lock_guard<std::mutex> lock(mtx_);
    ledger_.push_back(transaction);
    saveToFileUnlocked();
  }

  std::vector<Transaction> getAllTransactions() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return ledger_;
  }

  bool deleteTransaction(long long id) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = std::remove_if(ledger_.begin(), ledger_.end(),
                             [id](const Transaction &tx) { return tx.id == id; });
    if (it == ledger_.end()) {
      return false;
    }
    ledger_.erase(it, ledger_.end());
    saveToFileUnlocked();
    return true;
  }

  long long getBalance() const {
    std::lock_guard<std::mutex> lock(mtx_);
    long long balance = 0;
    for (const auto &tx : ledger_) {
      if (tx.type == "income") {
        balance += tx.amount;
      } else {
        balance -= tx.amount;
      }
    }
    return balance;
  }

 private:
  static std::filesystem::path dataDirectory() { return "data"; }

  static std::filesystem::path filePath() { return dataDirectory() / "ledger.json"; }

  void saveToFileUnlocked() const {
    std::filesystem::create_directories(dataDirectory());
    std::ofstream output(filePath());
    nlohmann::json data = ledger_;
    output << data.dump(2);
  }

  mutable std::mutex mtx_;
  std::vector<Transaction> ledger_;
};

#endif
