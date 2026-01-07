#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

class CategoryManager {
private:
    void ensureDataDir() const;
    std::filesystem::path dataDir() const;
    std::filesystem::path dataFilePath() const;
    void saveToFile() const;
    std::vector<std::string> categories_;
    mutable std::mutex mtx_;

public:
    void loadFromFile();
    std::vector<std::string> getCategories() const;
    bool addCategory(const std::string& category);
    bool removeCategory(const std::string& category);
    bool isCategoryValid(const std::string& category) const;
    std::string normalizeCategory(const std::string& category) const;
};
