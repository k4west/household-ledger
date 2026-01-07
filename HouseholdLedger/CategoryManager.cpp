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

std::filesystem::path CategoryManager::attributesFilePath() const {
    return dataDir() / "category_attributes.json";
}

void CategoryManager::saveToFile() const {
    ensureDataDir();
    json payload = categories_;
    std::ofstream out(dataFilePath());
    out << payload.dump(2, ' ', false, json::error_handler_t::replace);

    json attributesPayload = json::object();
    for (const auto& [name, attribute] : attributes_) {
        attributesPayload[name] = attributeToString(attribute);
    }
    std::ofstream attrOut(attributesFilePath());
    attrOut << attributesPayload.dump(2, ' ', false, json::error_handler_t::replace);
}

void CategoryManager::loadFromFile() {
    std::lock_guard<std::mutex> lock(mtx_);
    ensureDataDir();
    categories_.clear();
    attributes_.clear();
    std::ifstream in(dataFilePath());
    if (in.is_open()) {
        json payload;
        in >> payload;
        categories_ = payload.get<std::vector<std::string>>();
        auto it = std::find(categories_.begin(), categories_.end(), "기타");
        if (it == categories_.end()) {
            categories_.push_back("기타");
        }
    } else {
        categories_ = { "월급", "식비", "교통", "쇼핑", "주거", "기타" };
    }

    std::ifstream attrIn(attributesFilePath());
    if (attrIn.is_open()) {
        json payload;
        attrIn >> payload;
        if (payload.is_object()) {
            for (const auto& [name, value] : payload.items()) {
                CategoryAttribute attribute{};
                if (tryParseAttribute(value.get<std::string>(), attribute)) {
                    attributes_[name] = attribute;
                }
            }
        }
    }

    for (const auto& category : categories_) {
        if (attributes_.find(category) == attributes_.end()) {
            attributes_[category] = CategoryAttribute::Consumption;
        }
    }
    for (auto it = attributes_.begin(); it != attributes_.end();) {
        if (std::find(categories_.begin(), categories_.end(), it->first) == categories_.end()) {
            it = attributes_.erase(it);
        } else {
            ++it;
        }
    }

    saveToFile();
}

std::vector<std::string> CategoryManager::getCategories() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return categories_;
}

std::unordered_map<std::string, CategoryManager::CategoryAttribute>
CategoryManager::getCategoryAttributes() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return attributes_;
}

CategoryManager::CategoryAttribute CategoryManager::getCategoryAttribute(const std::string& category) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = attributes_.find(category);
    if (it == attributes_.end()) {
        return CategoryAttribute::Consumption;
    }
    return it->second;
}

bool CategoryManager::setCategoryAttribute(const std::string& category, CategoryAttribute attribute) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = std::find(categories_.begin(), categories_.end(), category);
    if (it == categories_.end()) {
        return false;
    }
    attributes_[category] = attribute;
    saveToFile();
    return true;
}

bool CategoryManager::addCategory(const std::string& category, CategoryAttribute attribute) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (category.empty()) {
        return false;
    }
    auto exists = std::find(categories_.begin(), categories_.end(), category);
    if (exists != categories_.end()) {
        return false;
    }
    categories_.push_back(category);
    attributes_[category] = attribute;
    saveToFile();
    return true;
}

bool CategoryManager::removeCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (category.empty() || category == "기타") {
        return false;
    }
    auto it = std::find(categories_.begin(), categories_.end(), category);
    if (it == categories_.end()) {
        return false;
    }
    categories_.erase(it);
    attributes_.erase(category);
    saveToFile();
    return true;
}

bool CategoryManager::isCategoryValid(const std::string& category) const {
    std::lock_guard<std::mutex> lock(mtx_);
    return std::find(categories_.begin(), categories_.end(), category) != categories_.end();
}

std::string CategoryManager::normalizeCategory(const std::string& category) const {
    if (isCategoryValid(category)) {
        return category;
    }
    return "기타";
}

bool CategoryManager::isSavingCategory(const std::string& category) const {
    return getCategoryAttribute(category) == CategoryAttribute::Saving;
}

std::string CategoryManager::attributeToString(CategoryAttribute attribute) {
    return attribute == CategoryAttribute::Saving ? "saving" : "consumption";
}

bool CategoryManager::tryParseAttribute(const std::string& value, CategoryAttribute& attribute) {
    if (value == "saving") {
        attribute = CategoryAttribute::Saving;
        return true;
    }
    if (value == "consumption") {
        attribute = CategoryAttribute::Consumption;
        return true;
    }
    return false;
}
