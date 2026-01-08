#pragma once

#include <filesystem>
#include <mutex>
#include <string>

#include "json.hpp"

class BudgetManager {
public:
    using json = nlohmann::json;

    json getBudgetForYear(int year);
    void setAnnualGoal(int year, const json& goalData);
    void setMonthlyBudget(int year, const std::string& month, const std::string& category, long long amount);
    void upsertBudget(int year, const json& payload);

private:
    void ensureDataDir() const;
    std::filesystem::path budgetDir() const;
    std::filesystem::path budgetFilePath(int year) const;
    json loadBudget(int year) const;
    void saveBudget(int year, const json& payload) const;

    mutable std::mutex mtx_;
};
