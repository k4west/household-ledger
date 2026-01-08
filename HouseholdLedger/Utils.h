#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "CategoryManager.h"
#include "Transaction.h"
#include "httplib.h"
#include "json.hpp"

long long generateId();
std::pair<int, int> getCurrentYearMonth();
std::optional<int> parseIntParam(const httplib::Request& req, const std::string& key);

double clampDouble(double value, double minValue, double maxValue);

TransactionType parseTransactionTypeInput(const std::string& value);
std::string formatTransactionType(TransactionType type);

Transaction normalizeTransactionCategory(const Transaction& tx, const CategoryManager& categories);
std::vector<Transaction> normalizeTransactions(const std::vector<Transaction>& list,
                                               const CategoryManager& categories);
void normalizeBudgetCategories(nlohmann::json& payload, const CategoryManager& categories);
