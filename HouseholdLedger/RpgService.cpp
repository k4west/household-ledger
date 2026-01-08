#include "RpgService.h"

#include <algorithm>
#include <cmath>

#include "Utils.h"

long long getBudgetValue(const nlohmann::json& monthBudget, const std::string& category) {
    if (!monthBudget.is_object() || !monthBudget.contains("expenses")) {
        return 0;
    }
    const auto& expenses = monthBudget.at("expenses");
    if (!expenses.is_object() || !expenses.contains(category)) {
        return 0;
    }
    return expenses.at(category).get<long long>();
}

AnnualBudgetTotals buildAnnualBudgetTotals(const nlohmann::json& budgetYear, const CategoryManager& categories) {
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
                             const nlohmann::json& budgetYear,
                             int year,
                             int month,
                             const CategoryManager& categories) {
    RpgSummary summary{};
    std::string monthKey = month < 10 ? "0" + std::to_string(month) : std::to_string(month);
    nlohmann::json monthBudget = nlohmann::json::object();
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
                                 const nlohmann::json& budgetYear,
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
