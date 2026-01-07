#include <chrono>
#include <ctime>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include "AccountManager.h"
#include "BudgetManager.h"
#include "CategoryManager.h"
#include "ScheduleManager.h"
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

    Transaction normalizeTransactionCategory(const Transaction& tx, const CategoryManager& categories) {
        Transaction normalized = tx;
        normalized.category = categories.normalizeCategory(tx.category);
        return normalized;
    }

    std::vector<Transaction> normalizeTransactions(const std::vector<Transaction>& list,
        const CategoryManager& categories) {
        std::vector<Transaction> normalized;
        normalized.reserve(list.size());
        for (const auto& tx : list) {
            normalized.push_back(normalizeTransactionCategory(tx, categories));
        }
        return normalized;
    }

    void normalizeBudgetCategories(json& payload, const CategoryManager& categories) {
        if (!payload.is_object() || !payload.contains("monthly")) {
            return;
        }
        auto& monthly = payload["monthly"];
        if (!monthly.is_object()) {
            return;
        }
        for (auto& [monthKey, monthValue] : monthly.items()) {
            if (!monthValue.is_object() || !monthValue.contains("expenses")) {
                continue;
            }
            auto& expenses = monthValue["expenses"];
            if (!expenses.is_object()) {
                continue;
            }
            std::vector<std::string> toRemove;
            long long otherTotal = 0;
            for (const auto& [category, amount] : expenses.items()) {
                if (!categories.isCategoryValid(category)) {
                    otherTotal += amount.get<long long>();
                    toRemove.push_back(category);
                }
            }
            for (const auto& category : toRemove) {
                expenses.erase(category);
            }
            if (otherTotal > 0) {
                expenses["기타"] = expenses.value("기타", 0) + otherTotal;
            }
        }
    }
    
    void register_routes(httplib::Server& svr,
                         AccountManager& manager,
                         CategoryManager& categories,
                         BudgetManager& budgets,
                         ScheduleManager& schedules) {
        svr.set_default_headers({ {"Access-Control-Allow-Origin", "*"} });

        svr.Get("/api/transactions", [&manager, &categories](const httplib::Request& req, httplib::Response& res) {            auto yearParam = parseIntParam(req, "year");
        auto yearParam = parseIntParam(req, "year");    
        auto monthParam = parseIntParam(req, "month");
            if (req.has_param("year") && !yearParam) {
                res.status = 400;
                res.set_content(R"({"error":"invalid year"})", "application/json");
                return;
            }
            if (req.has_param("month") && !monthParam) {
                res.status = 400;
                res.set_content(R"({"error":"invalid month"})", "application/json");
                return;
            }

            if (yearParam && monthParam) {
                auto monthly = manager.getTransactionsForMonth(*yearParam, *monthParam);
                json payload = normalizeTransactions(monthly, categories);
                res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
                return;
            }

            if (yearParam) {
                auto yearly = manager.getTransactionsForYear(*yearParam);
                json payload = normalizeTransactions(yearly, categories);
                res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
                return;
            }

            auto current = getCurrentYearMonth();
            auto monthly = manager.getTransactionsForMonth(current.first, current.second);
            json payload = normalizeTransactions(monthly, categories);
            res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            });

        svr.Post("/api/transactions", [&manager, &categories](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                Transaction tx = payload.get<Transaction>();
                tx.category = categories.normalizeCategory(tx.category);
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

        svr.Put("/api/transactions", [&manager, &categories](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                Transaction tx = payload.get<Transaction>();
                tx.category = categories.normalizeCategory(tx.category);
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

        svr.Delete("/api/categories", [&categories](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_param("name")) {
                res.status = 400;
                res.set_content(R"({"error":"missing name"})", "application/json");
                return;
            }
            std::string name = req.get_param_value("name");
            if (!categories.removeCategory(name)) {
                res.status = 409;
                res.set_content(R"({"error":"cannot remove category"})", "application/json");
                return;
            }
            res.set_content(R"({"status":"ok"})", "application/json");
            });

        svr.Get("/api/budget", [&budgets, &categories](const httplib::Request& req, httplib::Response& res) {
            auto yearParam = parseIntParam(req, "year");
            if (!yearParam) {
                res.status = 400;
                res.set_content(R"({"error":"missing year"})", "application/json");
                return;
            }
            json payload = budgets.getBudgetForYear(*yearParam);
            normalizeBudgetCategories(payload, categories);
            res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            });

        svr.Post("/api/budget", [&budgets, &categories](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                int year = payload.value("year", 0);
                if (year <= 0) {
                    res.status = 400;
                    res.set_content(R"({"error":"invalid year"})", "application/json");
                    return;
                }
                normalizeBudgetCategories(payload, categories);
                budgets.upsertBudget(year, payload);
                res.set_content(R"({"status":"ok"})", "application/json");
            } catch (const std::exception& ex) {
                json err{ {"error", ex.what()} };
                res.status = 400;
                res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            }
            });

        svr.Get("/api/schedules", [&schedules](const httplib::Request&, httplib::Response& res) {
            json payload = schedules.getSchedules();
            res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            });

        svr.Post("/api/schedules", [&schedules](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                ScheduleItem item = payload.get<ScheduleItem>();
                if (item.id == 0) {
                    item.id = generateId();
                }
                schedules.addSchedule(item);
                res.status = 201;
                res.set_content(R"({"status":"ok"})", "application/json");
            } catch (const std::exception& ex) {
                json err{ {"error", ex.what()} };
                res.status = 400;
                res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            }
            });

        svr.Put("/api/schedules", [&schedules](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                ScheduleItem item = payload.get<ScheduleItem>();
                if (!schedules.updateSchedule(item)) {
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

        svr.Delete("/api/schedules", [&schedules](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_param("id")) {
                res.status = 400;
                res.set_content(R"({"error":"missing id"})", "application/json");
                return;
            }
            long long id = 0;
            try {
                id = std::stoll(req.get_param_value("id"));
            } catch (const std::exception&) {
                res.status = 400;
                res.set_content(R"({"error":"invalid id"})", "application/json");
                return;
            }
            if (!schedules.deleteSchedule(id)) {
                res.status = 404;
                res.set_content(R"({"error":"not found"})", "application/json");
                return;
            }
            res.set_content(R"({"status":"ok"})", "application/json");
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
    BudgetManager budgets;
    ScheduleManager schedules;
    categories.loadFromFile();
    schedules.generateDueTransactions(manager);

    httplib::Server svr;
    register_routes(svr, manager, categories, budgets, schedules);

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
            tx.category = categories.normalizeCategory(tx.category);
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
