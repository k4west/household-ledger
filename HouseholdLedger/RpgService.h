#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "CategoryManager.h"
#include "Transaction.h"
#include "json.hpp"

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

struct AnnualBudgetTotals {
    long long budgetTotal{};
    long long savingGoal{};
    std::unordered_map<std::string, long long> categoryLimits;
};

long long getBudgetValue(const nlohmann::json& monthBudget, const std::string& category);
AnnualBudgetTotals buildAnnualBudgetTotals(const nlohmann::json& budgetYear, const CategoryManager& categories);

RpgSummary computeRpgSummary(std::vector<Transaction> transactions,
                             const nlohmann::json& budgetYear,
                             int year,
                             int month,
                             const CategoryManager& categories);

RpgSummary computeRpgSummaryYear(std::vector<Transaction> transactions,
                                 const nlohmann::json& budgetYear,
                                 int year,
                                 const CategoryManager& categories);
