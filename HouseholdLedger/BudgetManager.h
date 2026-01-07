#pragma once

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
    json loadAll() const;
    void saveAll(const json& payload) const;
    std::string yearKey(int year) const;

    mutable std::mutex mtx_;
};
