#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include "Transaction.h"

class AccountManager {
private:
    void ensureDataDir() const;
    std::filesystem::path dataDir() const;
    std::filesystem::path ledgerFilePath(int year, int month) const;
    bool parseYearMonthFromFilename(const std::filesystem::path& path, int& year, int& month) const;
    std::vector<Transaction> readLedgerFile(const std::filesystem::path& path) const;
    void writeLedgerFile(const std::filesystem::path& path, const std::vector<Transaction>& ledger) const;
    std::vector<std::filesystem::path> listLedgerFiles() const;
    bool parseYearMonth(const std::string& date, int& year, int& month) const;
    std::mutex mtx_;

public:
    std::vector<Transaction> getTransactionsForMonth(int year, int month);
    std::vector<Transaction> getTransactionsForYear(int year);
    std::vector<Transaction> getAllTransactions();
    void addTransaction(const Transaction& transaction);
    bool deleteTransaction(long long id);
    bool updateTransaction(const Transaction& transaction);
};
