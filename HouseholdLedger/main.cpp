#include <chrono>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <unordered_map>
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
        if (value == "saving") {
            return TransactionType::SAVING;
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
        case TransactionType::SAVING:
            return "saving";
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

    struct RpgSummary {
        long long income{};
        long long expense{};
        long long saving{};
        long long budgetTotal{};
        long long savingGoal{};
        double goalMultiplier{ 1.0 };
        double spendBonus{ 1.0 };
        double saveBonus{ 1.0 };
        double totalMultiplier{ 1.0 };
        double totalExp{};
        int level{ 1 };
        double partyHp{ 100.0 };
        int hunterCount{};
        int guardianCount{};
        int clericCount{};
        int gremlinCount{};
        int gremlinLevel{};
    };

    double clampDouble(double value, double minValue, double maxValue) {
        if (value < minValue) {
            return minValue;
        }
        if (value > maxValue) {
            return maxValue;
        }
        return value;
    }

    long long getBudgetValue(const json& monthBudget, const std::string& category) {
        if (!monthBudget.is_object() || !monthBudget.contains("expenses")) {
            return 0;
        }
        const auto& expenses = monthBudget.at("expenses");
        if (!expenses.is_object() || !expenses.contains(category)) {
            return 0;
        }
        return expenses.at(category).get<long long>();
    }

    struct AnnualBudgetTotals {
        long long budgetTotal{};
        long long savingGoal{};
        std::unordered_map<std::string, long long> categoryLimits;
    };

    AnnualBudgetTotals buildAnnualBudgetTotals(const json& budgetYear, const CategoryManager& categories) {
        AnnualBudgetTotals totals{};
        if (!budgetYear.contains("monthly") || !budgetYear.at("monthly").is_object()) {
            return totals;
        }
        for (const auto& [monthKey, monthValue] : budgetYear.at("monthly").items()) {
            if (!monthValue.is_object() || !monthValue.contains("expenses")) {
                continue;
            }
            const auto& expenses = monthValue.at("expenses");
            if (!expenses.is_object()) {
                continue;
            }
            for (const auto& [category, amount] : expenses.items()) {
                const auto value = amount.get<long long>();
                const std::string normalized = categories.normalizeCategory(category);
                if (categories.isSavingCategory(normalized)) {
                    totals.savingGoal += value;
                } else {
                    totals.budgetTotal += value;
                    totals.categoryLimits[normalized] += value;
                }
            }
        }
        return totals;
    }

    RpgSummary computeRpgSummary(std::vector<Transaction> transactions,
                                 const json& budgetYear,
                                 int year,
                                 int month,
                                 const CategoryManager& categories) {
        RpgSummary summary{};
        std::string monthKey = month < 10 ? "0" + std::to_string(month) : std::to_string(month);
        json monthBudget = json::object();
        if (budgetYear.contains("monthly") && budgetYear.at("monthly").contains(monthKey)) {
            monthBudget = budgetYear.at("monthly").at(monthKey);
        }

        if (monthBudget.contains("expenses") && monthBudget.at("expenses").is_object()) {
            for (const auto& [category, amount] : monthBudget.at("expenses").items()) {
                const auto value = amount.get<long long>();
                if (categories.isSavingCategory(category)) {
                    summary.savingGoal += value;
                } else {
                    summary.budgetTotal += value;
                }
            }
        }

        for (const auto& tx : transactions) {
            if (tx.type == TransactionType::INCOME) {
                summary.income += tx.amount;
            } else if (tx.type == TransactionType::EXPENSE) {
                summary.expense += tx.amount;
            } else if (tx.type == TransactionType::SAVING) {
                summary.saving += tx.amount;
            }
        }

        double spendTight = 0.0;
        double saveTight = 0.0;
        if (summary.income > 0) {
            spendTight = clampDouble((0.90 - (static_cast<double>(summary.budgetTotal) / summary.income)) / 0.30, 0.0, 1.0);
            saveTight = clampDouble((static_cast<double>(summary.savingGoal) / summary.income) / 0.30, 0.0, 1.0);
        }
        summary.goalMultiplier = 1.0 + 0.10 * spendTight + 0.10 * saveTight;
        if (summary.budgetTotal > 0) {
            summary.spendBonus = summary.expense <= summary.budgetTotal ? 1.1 : 1.0;
        }
        if (summary.savingGoal > 0) {
            double saveAch = static_cast<double>(summary.saving) / summary.savingGoal;
            summary.saveBonus = 1.0 + 0.15 * clampDouble(saveAch, 0.0, 1.0);
        }
        summary.totalMultiplier = std::min(summary.goalMultiplier * summary.spendBonus * summary.saveBonus, 1.5);

        std::sort(transactions.begin(), transactions.end(), [](const Transaction& a, const Transaction& b) {
            if (a.date == b.date) {
                return a.id < b.id;
            }
            return a.date < b.date;
        });

        std::unordered_map<std::string, long long> categorySpent;
        double partyHp = 100.0;
        int gremlinLevel = 0;
        double scale = std::max(static_cast<double>(summary.income) / 100.0, 10000.0);

        for (const auto& tx : transactions) {
            if (tx.type == TransactionType::TRANSFER) {
                continue;
            }
            double multiplier = partyHp <= 0 ? 1.0 : summary.totalMultiplier;
            double baseLogExp = std::log(1.0 + (scale > 0 ? tx.amount / scale : 0.0));
            if (tx.type == TransactionType::INCOME) {
                summary.hunterCount += 1;
                summary.totalExp += 120.0 * baseLogExp * multiplier;
                continue;
            }
            if (tx.type == TransactionType::SAVING) {
                summary.guardianCount += 1;
                summary.totalExp += 150.0 * baseLogExp * multiplier;
                continue;
            }
            if (tx.type != TransactionType::EXPENSE) {
                continue;
            }

            const std::string category = categories.normalizeCategory(tx.category);
            const long long budgetLimit = getBudgetValue(monthBudget, category);
            auto& spent = categorySpent[category];
            spent += tx.amount;
            const bool overBudget = budgetLimit <= 0 || spent > budgetLimit;

            if (!overBudget) {
                summary.clericCount += 1;
                summary.totalExp += 80.0 * baseLogExp * multiplier;
                if (summary.budgetTotal > 0) {
                    double damage = (tx.amount / static_cast<double>(summary.budgetTotal)) * 100.0 * 0.5;
                    partyHp -= damage;
                }
            } else {
                summary.gremlinCount += 1;
                summary.totalExp += 260.0 * baseLogExp * multiplier;
                gremlinLevel += 1;
                if (summary.budgetTotal > 0) {
                    double damage = (tx.amount / static_cast<double>(summary.budgetTotal)) * 100.0 * 1.5;
                    partyHp -= damage;
                }
                partyHp -= gremlinLevel * 0.5;
            }
            if (partyHp < 0) {
                partyHp = 0;
            }
        }

        summary.partyHp = partyHp;
        summary.gremlinLevel = gremlinLevel;
        summary.level = static_cast<int>(summary.totalExp / 100.0) + 1;
        return summary;
    }

    RpgSummary computeRpgSummaryYear(std::vector<Transaction> transactions,
                                     const json& budgetYear,
                                     int year,
                                     const CategoryManager& categories) {
        RpgSummary summary{};
        const AnnualBudgetTotals totals = buildAnnualBudgetTotals(budgetYear, categories);
        summary.budgetTotal = totals.budgetTotal;
        summary.savingGoal = totals.savingGoal;

        for (const auto& tx : transactions) {
            if (tx.type == TransactionType::INCOME) {
                summary.income += tx.amount;
            } else if (tx.type == TransactionType::EXPENSE) {
                summary.expense += tx.amount;
            } else if (tx.type == TransactionType::SAVING) {
                summary.saving += tx.amount;
            }
        }

        double spendTight = 0.0;
        double saveTight = 0.0;
        if (summary.income > 0) {
            spendTight = clampDouble((0.90 - (static_cast<double>(summary.budgetTotal) / summary.income)) / 0.30, 0.0, 1.0);
            saveTight = clampDouble((static_cast<double>(summary.savingGoal) / summary.income) / 0.30, 0.0, 1.0);
        }
        summary.goalMultiplier = 1.0 + 0.10 * spendTight + 0.10 * saveTight;
        if (summary.budgetTotal > 0) {
            summary.spendBonus = summary.expense <= summary.budgetTotal ? 1.1 : 1.0;
        }
        if (summary.savingGoal > 0) {
            double saveAch = static_cast<double>(summary.saving) / summary.savingGoal;
            summary.saveBonus = 1.0 + 0.15 * clampDouble(saveAch, 0.0, 1.0);
        }
        summary.totalMultiplier = std::min(summary.goalMultiplier * summary.spendBonus * summary.saveBonus, 1.5);

        std::sort(transactions.begin(), transactions.end(), [](const Transaction& a, const Transaction& b) {
            if (a.date == b.date) {
                return a.id < b.id;
            }
            return a.date < b.date;
        });

        std::unordered_map<std::string, long long> categorySpent;
        double partyHp = 100.0;
        int gremlinLevel = 0;
        double scale = std::max(static_cast<double>(summary.income) * 0.01, 1.0);

        for (const auto& tx : transactions) {
            if (tx.type == TransactionType::TRANSFER) {
                continue;
            }
            double multiplier = partyHp <= 0 ? 1.0 : summary.totalMultiplier;
            double baseLogExp = std::log(1.0 + (scale > 0 ? tx.amount / scale : 0.0));
            if (tx.type == TransactionType::INCOME) {
                summary.hunterCount += 1;
                summary.totalExp += 120.0 * baseLogExp * multiplier;
                continue;
            }
            if (tx.type == TransactionType::SAVING) {
                summary.guardianCount += 1;
                summary.totalExp += 150.0 * baseLogExp * multiplier;
                continue;
            }
            if (tx.type != TransactionType::EXPENSE) {
                continue;
            }

            const std::string category = categories.normalizeCategory(tx.category);
            const long long budgetLimit = totals.categoryLimits.count(category) ? totals.categoryLimits.at(category) : 0;
            auto& spent = categorySpent[category];
            spent += tx.amount;
            const bool overBudget = budgetLimit <= 0 || spent > budgetLimit;

            if (!overBudget) {
                summary.clericCount += 1;
                summary.totalExp += 80.0 * baseLogExp * multiplier;
                if (summary.budgetTotal > 0) {
                    double damage = (tx.amount / static_cast<double>(summary.budgetTotal)) * 100.0 * 0.5;
                    partyHp -= damage;
                }
            } else {
                summary.gremlinCount += 1;
                summary.totalExp += 260.0 * baseLogExp * multiplier;
                gremlinLevel += 1;
                if (summary.budgetTotal > 0) {
                    double damage = (tx.amount / static_cast<double>(summary.budgetTotal)) * 100.0 * 1.5;
                    partyHp -= damage;
                }
                partyHp -= gremlinLevel * 0.5;
            }
            if (partyHp < 0) {
                partyHp = 0;
            }
        }

        summary.partyHp = partyHp;
        summary.gremlinLevel = gremlinLevel;
        summary.level = static_cast<int>(summary.totalExp / 100.0) + 1;
        return summary;
    }
    
    void register_routes(httplib::Server& svr,
                         AccountManager& manager,
                         CategoryManager& categories,
                         BudgetManager& budgets,
                         ScheduleManager& schedules) {
        svr.set_default_headers({ {"Access-Control-Allow-Origin", "*"} });

        svr.Get("/api/transactions", [&manager, &categories](const httplib::Request& req, httplib::Response& res) {
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
                std::string attributeValue = payload.value("attribute", "consumption");
                CategoryManager::CategoryAttribute attribute{};
                if (!CategoryManager::tryParseAttribute(attributeValue, attribute)) {
                    res.status = 400;
                    res.set_content(R"({"error":"invalid attribute"})", "application/json");
                    return;
                }
                if (!categories.addCategory(name, attribute)) {
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
        svr.Get("/api/category-attributes", [&categories](const httplib::Request&, httplib::Response& res) {
            json payload = json::object();
            for (const auto& [name, attribute] : categories.getCategoryAttributes()) {
                payload[name] = CategoryManager::attributeToString(attribute);
            }
            res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
            });

        svr.Post("/api/category-attributes", [&categories](const httplib::Request& req, httplib::Response& res) {
            try {
                auto payload = json::parse(req.body);
                std::string name = payload.value("name", "");
                std::string attributeValue = payload.value("attribute", "");
                if (name.empty()) {
                    res.status = 400;
                    res.set_content(R"({"error":"missing name"})", "application/json");
                    return;
                }
                CategoryManager::CategoryAttribute attribute{};
                if (!CategoryManager::tryParseAttribute(attributeValue, attribute)) {
                    res.status = 400;
                    res.set_content(R"({"error":"invalid attribute"})", "application/json");
                    return;
                }
                if (!categories.setCategoryAttribute(name, attribute)) {
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
        svr.Get("/api/rpg", [&manager, &budgets, &categories](const httplib::Request& req, httplib::Response& res) {
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

            auto current = getCurrentYearMonth();
            int year = yearParam.value_or(current.first);
            bool isYearOnly = req.has_param("year") && !req.has_param("month");

            auto budgetYear = budgets.getBudgetForYear(year);
            RpgSummary summary{};
            int payloadMonth = 0;
            if (monthParam) {
                int month = *monthParam;
                auto monthly = manager.getTransactionsForMonth(year, month);
                summary = computeRpgSummary(normalizeTransactions(monthly, categories), budgetYear, year, month, categories);
                payloadMonth = month;
            } else if (isYearOnly) {
                auto yearly = manager.getTransactionsForYear(year);
                summary = computeRpgSummaryYear(normalizeTransactions(yearly, categories), budgetYear, year, categories);
                payloadMonth = 0;
            } else {
                int month = current.second;
                auto monthly = manager.getTransactionsForMonth(current.first, month);
                summary = computeRpgSummary(normalizeTransactions(monthly, categories), budgetYear, current.first, month, categories);
                year = current.first;
                payloadMonth = month;
            }

            json payload{
                {"year", year},
                {"month", payloadMonth},
                {"income", summary.income},
                {"expense", summary.expense},
                {"saving", summary.saving},
                {"budget", summary.budgetTotal},
                {"saving_goal", summary.savingGoal},
                {"goal_multiplier", summary.goalMultiplier},
                {"spend_bonus", summary.spendBonus},
                {"save_bonus", summary.saveBonus},
                {"total_multiplier", summary.totalMultiplier},
                {"total_exp", summary.totalExp},
                {"level", summary.level},
                {"party_hp", summary.partyHp},
                {"roles", {
                    {"hunter", summary.hunterCount},
                    {"guardian", summary.guardianCount},
                    {"cleric", summary.clericCount},
                    {"gremlin", summary.gremlinCount}
                }},
                {"gremlin_level", summary.gremlinLevel}
            };
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
            std::cout << "Type (income/expense/saving/transfer): ";
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
