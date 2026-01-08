#include "Utils.h"

#include <chrono>
#include <ctime>

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

double clampDouble(double value, double minValue, double maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
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

void normalizeBudgetCategories(nlohmann::json& payload, const CategoryManager& categories) {
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
