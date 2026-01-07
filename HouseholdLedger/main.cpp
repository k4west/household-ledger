#include <chrono>
#include <ctime>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include "AccountManager.h"
#include "CategoryManager.h"
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

namespace {

    long long generateId() {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    }

    std::pair<int, int> getCurrentYearMonth() {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        local = *std::localtime(&now);
#endif
        return { local.tm_year + 1900, local.tm_mon + 1 };
    }

    std::optional<int> parseIntParam(const httplib::Request& req, const std::string& key) {
        if (!req.has_param(key)) {
            return std::nullopt;
        }
        try {
            return std::stoi(req.get_param_value(key));
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    TransactionType parseTransactionTypeInput(const std::string& value) {
        if (value == "income") {
            return TransactionType::INCOME;
        }
        if (value == "transfer") {
            return TransactionType::TRANSFER;
        }
        return TransactionType::EXPENSE;
    }

    std::string formatTransactionType(TransactionType type) {
        switch (type) {
        case TransactionType::INCOME:
            return "income";
        case TransactionType::TRANSFER:
            return "transfer";
        default:
            return "expense";
        }
    }

    void register_routes(httplib::Server& svr, AccountManager& manager, CategoryManager& categories) {
        svr.set_default_headers({ {"Access-Control-Allow-Origin", "*"} });

        svr.Get("/api/transactions", [&manager](const httplib::Request& req, httplib::Response& res) {
            auto yearParam = parseIntParam(req, "year");
            auto monthParam = parseIntParam(req, "month");
            int year = 0;
            int month = 0;
            if (yearParam && monthParam) {
                year = *yearParam;
                month = *monthParam;
            } else {
                auto current = getCurrentYearMonth();
                year = current.first;
                month = current.second;
            }
            auto monthly = manager.getTransactionsForMonth(year, month);
            json payload = monthly;
            res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            });

        svr.Post("/api/transactions", [&manager](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                Transaction tx = payload.get<Transaction>();
                manager.addTransaction(tx);
                res.status = 201;
                res.set_content(R"({"status":"ok"})", "application/json");
            }
            catch (const std::exception& ex) {
                json err{ {"error", ex.what()} };
                res.status = 400;
                res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            }
            });

        svr.Put("/api/transactions", [&manager](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                Transaction tx = payload.get<Transaction>();
                if (!manager.updateTransaction(tx)) {
                    res.status = 404;
                    res.set_content(R"({"error":"not found"})", "application/json");
                    return;
                }
                res.set_content(R"({"status":"ok"})", "application/json");
            } catch (const std::exception& ex) {
                json err{ {"error", ex.what()} };
                res.status = 400;
                res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            }
            });

        svr.Delete("/api/transactions", [&manager](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_param("id")) {
                res.status = 400;
                res.set_content(R"({"error":"missing id"})", "application/json");
                return;
            }
            long long id = 0;
            try {
                id = std::stoll(req.get_param_value("id"));
            }
            catch (const std::exception&) {
                res.status = 400;
                res.set_content(R"({"error":"invalid id"})", "application/json");
                return;
            }
            bool removed = manager.deleteTransaction(id);
            if (!removed) {
                res.status = 404;
                res.set_content(R"({"error":"not found"})", "application/json");
                return;
            }
            res.set_content(R"({"status":"ok"})", "application/json");
            });

        svr.Get("/api/categories", [&categories](const httplib::Request&, httplib::Response& res) {
            json payload = categories.getCategories();
            res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            });

        svr.Post("/api/categories", [&categories](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                std::string name = payload.value("name", "");
                if (!categories.addCategory(name)) {
                    res.status = 409;
                    res.set_content(R"({"error":"duplicate or invalid category"})", "application/json");
                    return;
                }
                res.status = 201;
                res.set_content(R"({"status":"ok"})", "application/json");
            } catch (const std::exception& ex) {
                json err{ {"error", ex.what()} };
                res.status = 400;
                res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            }
            });

        svr.set_mount_point("/", "./www");
    }

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

}  // namespace

int main() {
    AccountManager manager;
    CategoryManager categories;
    categories.loadFromFile();

    httplib::Server svr;
    register_routes(svr, manager, categories);

    std::thread server([&svr]() {
        std::cout << "\nServer listening on http://localhost:8888\n";
        svr.listen("0.0.0.0", 8888);
        });

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
        }
        else if (choice == 2) {
            Transaction tx{};
            tx.id = generateId();
            std::cout << "Date (YYYY-MM-DD): ";
            std::cin >> tx.date;
            std::cout << "Type (income/expense/transfer): ";
            std::string typeValue;
            std::cin >> typeValue;
            tx.type = parseTransactionTypeInput(typeValue);
            std::cout << "Category: ";
            std::cin >> tx.category;
            std::cout << "Memo: ";
            //std::cin >> tx.memo;
            std::getline(std::cin >> std::ws, tx.memo);
            std::cout << "Amount: ";
            std::cin >> tx.amount;
            manager.addTransaction(tx);
            std::cout << "Transaction added.\n";
        }
        else if (choice == 3) {
            running = false;
        }
    }

    std::cout << "Stopping server...\n";
    svr.stop();
    server.join();
    return 0;
}
