#include "BudgetManager.h"

#include <filesystem>
#include <fstream>

void BudgetManager::ensureDataDir() const {
    std::filesystem::create_directories("data");
}

BudgetManager::json BudgetManager::loadAll() const {
    ensureDataDir();
    std::ifstream in("data/budget.json");
    if (!in.is_open()) {
        return json::object();
    }
    json payload;
    in >> payload;
    if (!payload.is_object()) {
        return json::object();
    }
    return payload;
}

void BudgetManager::saveAll(const json& payload) const {
    ensureDataDir();
    std::ofstream out("data/budget.json");
    out << payload.dump(2, ' ', false, json::error_handler_t::replace);
}

std::string BudgetManager::yearKey(int year) const {
    return std::to_string(year);
}

BudgetManager::json BudgetManager::getBudgetForYear(int year) {
    std::lock_guard<std::mutex> lock(mtx_);
    json payload = loadAll();
    auto key = yearKey(year);
    if (!payload.contains(key)) {
        return json::object();
    }
    return payload.at(key);
}

void BudgetManager::setAnnualGoal(int year, const json& goalData) {
    std::lock_guard<std::mutex> lock(mtx_);
    json payload = loadAll();
    auto key = yearKey(year);
    if (!payload.contains(key) || !payload.at(key).is_object()) {
        payload[key] = json::object();
    }
    payload[key]["annual_goals"] = goalData;
    saveAll(payload);
}

void BudgetManager::setMonthlyBudget(int year, const std::string& month, const std::string& category, long long amount) {
    std::lock_guard<std::mutex> lock(mtx_);
    json payload = loadAll();
    auto key = yearKey(year);
    if (!payload.contains(key) || !payload.at(key).is_object()) {
        payload[key] = json::object();
    }
    auto& yearData = payload[key];
    yearData["monthly"][month]["expenses"][category] = amount;
    saveAll(payload);
}

void BudgetManager::upsertBudget(int year, const json& payloadUpdate) {
    std::lock_guard<std::mutex> lock(mtx_);
    json payload = loadAll();
    auto key = yearKey(year);
    if (!payload.contains(key) || !payload.at(key).is_object()) {
        payload[key] = json::object();
    }
    auto& yearData = payload[key];
    if (payloadUpdate.contains("annual_goals")) {
        yearData["annual_goals"] = payloadUpdate.at("annual_goals");
    }
    if (payloadUpdate.contains("monthly")) {
        for (const auto& [monthKey, monthValue] : payloadUpdate.at("monthly").items()) {
            if (!yearData.contains("monthly") || !yearData["monthly"].is_object()) {
                yearData["monthly"] = json::object();
            }
            if (!yearData["monthly"].contains(monthKey) || !yearData["monthly"][monthKey].is_object()) {
                yearData["monthly"][monthKey] = json::object();
            }
            auto& targetMonth = yearData["monthly"][monthKey];
            if (monthValue.contains("expenses")) {
                targetMonth["expenses"] = monthValue.at("expenses");
            }
            if (monthValue.contains("goals")) {
                targetMonth["goals"] = monthValue.at("goals");
            }
        }
    }
    saveAll(payload);
}
