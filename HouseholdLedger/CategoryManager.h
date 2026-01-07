#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class CategoryManager {
public:
    enum class CategoryAttribute {
        Consumption,
        Saving
    };

private:
    void ensureDataDir() const;
    std::filesystem::path dataDir() const;
    std::filesystem::path dataFilePath() const;
    std::filesystem::path attributesFilePath() const;
    void saveToFile() const;
    std::vector<std::string> categories_;
    std::unordered_map<std::string, CategoryAttribute> attributes_;
    mutable std::mutex mtx_;

public:
    void loadFromFile();
    std::vector<std::string> getCategories() const;
    std::unordered_map<std::string, CategoryAttribute> getCategoryAttributes() const;
    CategoryAttribute getCategoryAttribute(const std::string& category) const;
    bool setCategoryAttribute(const std::string& category, CategoryAttribute attribute);
    bool addCategory(const std::string& category, CategoryAttribute attribute = CategoryAttribute::Consumption);
    bool removeCategory(const std::string& category);
    bool isCategoryValid(const std::string& category) const;
    std::string normalizeCategory(const std::string& category) const;
    bool isSavingCategory(const std::string& category) const;
    static std::string attributeToString(CategoryAttribute attribute);
    static bool tryParseAttribute(const std::string& value, CategoryAttribute& attribute);
};
