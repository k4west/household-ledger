#include "ScheduleManager.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

void ScheduleManager::ensureDataDir() const {
    std::filesystem::create_directories("data");
}

std::vector<ScheduleItem> ScheduleManager::loadSchedules() const {
    ensureDataDir();
    std::ifstream in("data/schedules.json");
    if (!in.is_open()) {
        return {};
    }
    json payload;
    in >> payload;
    if (!payload.is_array()) {
        return {};
    }
    return payload.get<std::vector<ScheduleItem>>();
}

void ScheduleManager::saveSchedules(const std::vector<ScheduleItem>& schedules) const {
    ensureDataDir();
    json payload = schedules;
    std::ofstream out("data/schedules.json");
    out << payload.dump(2, ' ', false, json::error_handler_t::replace);
}

std::vector<ScheduleItem> ScheduleManager::getSchedules() {
    std::lock_guard<std::mutex> lock(mtx_);
    return loadSchedules();
}

ScheduleItem ScheduleManager::addSchedule(const ScheduleItem& item) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto schedules = loadSchedules();
    schedules.push_back(item);
    saveSchedules(schedules);
    return item;
}

bool ScheduleManager::updateSchedule(const ScheduleItem& item) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto schedules = loadSchedules();
    auto it = std::find_if(schedules.begin(), schedules.end(),
                           [&item](const ScheduleItem& existing) { return existing.id == item.id; });
    if (it == schedules.end()) {
        return false;
    }
    *it = item;
    saveSchedules(schedules);
    return true;
}

bool ScheduleManager::deleteSchedule(long long id) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto schedules = loadSchedules();
    auto it = std::remove_if(schedules.begin(), schedules.end(),
                             [id](const ScheduleItem& item) { return item.id == id; });
    if (it == schedules.end()) {
        return false;
    }
    schedules.erase(it, schedules.end());
    saveSchedules(schedules);
    return true;
}

bool ScheduleManager::parseDate(const std::string& date, int& year, int& month, int& day) {
    if (date.size() < 10) {
        return false;
    }
    try {
        year = std::stoi(date.substr(0, 4));
        month = std::stoi(date.substr(5, 2));
        day = std::stoi(date.substr(8, 2));
    } catch (const std::exception&) {
        return false;
    }
    return year > 0 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

int ScheduleManager::daysInMonth(int year, int month) {
    static const int days[]{ 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return leap ? 29 : 28;
    }
    return days[month - 1];
}

std::string ScheduleManager::formatDate(int year, int month, int day) {
    std::ostringstream out;
    out << std::setw(4) << std::setfill('0') << year << "-";
    out << std::setw(2) << std::setfill('0') << month << "-";
    out << std::setw(2) << std::setfill('0') << day;
    return out.str();
}

bool ScheduleManager::isBeforeOrEqual(const std::string& lhs, const std::string& rhs) {
    return lhs <= rhs;
}

bool ScheduleManager::isBefore(const std::string& lhs, const std::string& rhs) {
    return lhs < rhs;
}

std::string ScheduleManager::addMonths(const std::string& date, int months, int dayOverride) {
    int year = 0;
    int month = 0;
    int day = 0;
    if (!parseDate(date, year, month, day)) {
        return date;
    }
    int totalMonths = (year * 12 + (month - 1)) + months;
    int newYear = totalMonths / 12;
    int newMonth = totalMonths % 12 + 1;
    int targetDay = dayOverride > 0 ? dayOverride : day;
    int maxDay = daysInMonth(newYear, newMonth);
    if (targetDay > maxDay) {
        targetDay = maxDay;
    }
    return formatDate(newYear, newMonth, targetDay);
}

std::string ScheduleManager::nextDueDate(const ScheduleItem& item) {
    const int dayOverride = item.day;
    if (!item.lastGenerated.empty()) {
        return addMonths(item.lastGenerated, 1, dayOverride);
    }
    if (item.startDate.empty()) {
        return "";
    }
    int year = 0;
    int month = 0;
    int day = 0;
    if (!parseDate(item.startDate, year, month, day)) {
        return "";
    }
    int targetDay = dayOverride > 0 ? dayOverride : day;
    int maxDay = daysInMonth(year, month);
    if (targetDay > maxDay) {
        targetDay = maxDay;
    }
    auto candidate = formatDate(year, month, targetDay);
    if (isBefore(candidate, item.startDate)) {
        candidate = addMonths(candidate, 1, dayOverride);
    }
    return candidate;
}

void ScheduleManager::generateDueTransactions(AccountManager& manager) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto schedules = loadSchedules();
    if (schedules.empty()) {
        return;
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    local = *std::localtime(&now);
#endif
    std::string today = formatDate(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
    bool updated = false;

    for (auto& item : schedules) {
        if (item.frequency != "MONTHLY_DATE") {
            continue;
        }
        std::string dueDate = nextDueDate(item);
        if (dueDate.empty()) {
            continue;
        }
        while (isBeforeOrEqual(dueDate, today)) {
            if (!item.endDate.empty() && isBefore(item.endDate, dueDate)) {
                break;
            }
            Transaction tx{};
            tx.id = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
            tx.date = dueDate;
            if (item.type == ScheduleType::INCOME) {
                tx.type = TransactionType::INCOME;
            } else if (item.type == ScheduleType::TRANSFER) {
                tx.type = TransactionType::TRANSFER;
            } else {
                tx.type = TransactionType::EXPENSE;
            }
            tx.category = item.category;
            tx.memo = item.name;
            tx.amount = item.amount;
            manager.addTransaction(tx);

            item.lastGenerated = dueDate;
            updated = true;
            dueDate = addMonths(dueDate, 1, item.day);
        }
    }

    if (updated) {
        saveSchedules(schedules);
    }
}
