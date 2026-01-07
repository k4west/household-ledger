#include "CategoryManager.h"

#include <algorithm>
#include <fstream>

#include "json.hpp"

using json = nlohmann::json;

void CategoryManager::ensureDataDir() const {
    std::filesystem::create_directories(dataDir());
}

std::filesystem::path CategoryManager::dataDir() const {
    return std::filesystem::path("data");
}

std::filesystem::path CategoryManager::dataFilePath() const {
    return dataDir() / "categories.json";
}

void CategoryManager::saveToFile() const {
    ensureDataDir();
    json payload = categories_;
    std::ofstream out(dataFilePath());
    out << payload.dump(2, ' ', false, json::error_handler_t::replace);
}

void CategoryManager::loadFromFile() {
    std::lock_guard<std::mutex> lock(mtx_);
    ensureDataDir();
    std::ifstream in(dataFilePath());
    if (in.is_open()) {
        json payload;
        in >> payload;
        categories_ = payload.get<std::vector<std::string>>();
        return;
    }
    categories_ = { "월급", "식비", "교통", "쇼핑", "주거", "기타" };
    saveToFile();
}

std::vector<std::string> CategoryManager::getCategories() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return categories_;
}

bool CategoryManager::addCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (category.empty()) {
        return false;
    }
    auto exists = std::find(categories_.begin(), categories_.end(), category);
    if (exists != categories_.end()) {
        return false;
    }
    categories_.push_back(category);
    saveToFile();
    return true;
}
