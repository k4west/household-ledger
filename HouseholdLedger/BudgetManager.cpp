#include "BudgetManager.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

void BudgetManager::ensureDataDir() const {
    std::filesystem::create_directories(budgetDir());
}

std::filesystem::path BudgetManager::budgetDir() const {
    return std::filesystem::path("data") / "budget";
}

std::filesystem::path BudgetManager::budgetFilePath(int year) const {
    std::ostringstream filename;
    filename << "budget_" << std::setw(4) << std::setfill('0') << year << ".json";
    return budgetDir() / filename.str();
}

BudgetManager::json BudgetManager::loadBudget(int year) const {
    ensureDataDir();
    std::ifstream in(budgetFilePath(year));
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

void BudgetManager::saveBudget(int year, const json& payload) const {
    ensureDataDir();
    std::ofstream out(budgetFilePath(year));
    out << payload.dump(2, ' ', false, json::error_handler_t::replace);
}

BudgetManager::json BudgetManager::getBudgetForYear(int year) {
    std::lock_guard<std::mutex> lock(mtx_);
    return loadBudget(year);
}

void BudgetManager::setAnnualGoal(int year, const json& goalData) {
    std::lock_guard<std::mutex> lock(mtx_);
    json payload = loadBudget(year);
    if (!payload.is_object()) {
        payload = json::object();
    }
    payload["annual_goals"] = goalData;
    saveBudget(year, payload);
}

void BudgetManager::setMonthlyBudget(int year, const std::string& month, const std::string& category, long long amount) {
    std::lock_guard<std::mutex> lock(mtx_);
    json payload = loadBudget(year);
    if (!payload.is_object()) {
        payload = json::object();
    }
    payload["monthly"][month]["expenses"][category] = amount;
    saveBudget(year, payload);
}

void BudgetManager::upsertBudget(int year, const json& payloadUpdate) {
    std::lock_guard<std::mutex> lock(mtx_);
    json payload = loadBudget(year);
    if (!payload.is_object()) {
        payload = json::object();
    }
    auto& yearData = payload;
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
    saveBudget(year, payload);
}
