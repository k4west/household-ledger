#include "ConsoleUi.h"

#include <iostream>
#include <limits>

#include "Utils.h"

void printTransactions(const std::vector<Transaction>& transactions) {
    if (transactions.empty()) {
        std::cout << "No transactions available.\n";
        return;
    }
    for (const auto& tx : transactions) {
        std::cout << "[" << tx.id << "] " << tx.date << " " << formatTransactionType(tx.type) << " "
            << tx.category << " " << tx.memo << " " << tx.amount << "\n";
    }
}

void runConsoleLoop(AccountManager& manager, CategoryManager& categories) {
    bool running = true;
    while (running) {
        std::cout << "\n=== Household Ledger ===\n";
        std::cout << "1. View transactions\n";
        std::cout << "2. Add transaction\n";
        std::cout << "3. Exit\n";
        std::cout << "Select option: ";

        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (choice == 1) {
            auto all = manager.getAllTransactions();
            printTransactions(all);
        } else if (choice == 2) {
            Transaction tx{};
            tx.id = generateId();
            std::cout << "Date (YYYY-MM-DD): ";
            std::cin >> tx.date;
            std::cout << "Type (income/expense/saving/transfer): ";
            std::string typeValue;
            std::cin >> typeValue;
            tx.type = parseTransactionTypeInput(typeValue);
            std::cout << "Category: ";
            std::cin >> tx.category;
            tx.category = categories.normalizeCategory(tx.category);
            std::cout << "Memo: ";
            std::getline(std::cin >> std::ws, tx.memo);
            std::cout << "Amount: ";
            std::cin >> tx.amount;
            manager.addTransaction(tx);
            std::cout << "Transaction added.\n";
        } else if (choice == 3) {
            running = false;
        }
    }
}
