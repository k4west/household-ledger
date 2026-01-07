#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "AccountManager.h"
#include "json.hpp"

enum class ScheduleType {
    INCOME,
    EXPENSE,
    TRANSFER
};

NLOHMANN_JSON_SERIALIZE_ENUM(ScheduleType, {
    {ScheduleType::INCOME, "INCOME"},
    {ScheduleType::EXPENSE, "EXPENSE"},
    {ScheduleType::TRANSFER, "TRANSFER"},
})

struct ScheduleItem {
    long long id{};
    std::string name;
    ScheduleType type{ ScheduleType::EXPENSE };
    long long amount{};
    std::string category;
    std::string frequency;
    int day{};
    std::string startDate;
    std::string endDate;
    std::string lastGenerated;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ScheduleItem, id, name, type, amount, category, frequency, day, startDate, endDate, lastGenerated)

class ScheduleManager {
public:
    using json = nlohmann::json;

    std::vector<ScheduleItem> getSchedules();
    ScheduleItem addSchedule(const ScheduleItem& item);
    bool updateSchedule(const ScheduleItem& item);
    bool deleteSchedule(long long id);
    void generateDueTransactions(AccountManager& manager);

private:
    void ensureDataDir() const;
    std::vector<ScheduleItem> loadSchedules() const;
    void saveSchedules(const std::vector<ScheduleItem>& schedules) const;
    static bool parseDate(const std::string& date, int& year, int& month, int& day);
    static int daysInMonth(int year, int month);
    static std::string formatDate(int year, int month, int day);
    static bool isBeforeOrEqual(const std::string& lhs, const std::string& rhs);
    static bool isBefore(const std::string& lhs, const std::string& rhs);
    static std::string addMonths(const std::string& date, int months, int dayOverride);
    static std::string nextDueDate(const ScheduleItem& item);

    mutable std::mutex mtx_;
};
