#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

#include "AccountManager.h"
#include "httplib.h"
#include "json.hpp"

namespace {

long long generateId() {
  auto now = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void register_routes(httplib::Server& svr, AccountManager& manager) {
  svr.set_default_headers({{"Access-Control-Allow-Origin", "*"}});

  svr.Get("/api/transactions", [&manager](const httplib::Request&, httplib::Response& res) {
    auto all = manager.getAllTransactions();
    nlohmann::json payload = all;
    res.set_content(payload.dump(2), "application/json");
  });

  svr.Post("/api/transactions", [&manager](const httplib::Request& req, httplib::Response& res) {
    try {
      auto payload = nlohmann::json::parse(req.body);
      Transaction tx = payload.get<Transaction>();
      manager.addTransaction(tx);
      res.status = 201;
      res.set_content(R"({"status":"ok"})", "application/json");
    } catch (const std::exception& ex) {
      nlohmann::json err{{"error", ex.what()}};
      res.status = 400;
      res.set_content(err.dump(2), "application/json");
    }
  });

  svr.Delete("/api/transactions", [&manager](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("id")) {
      res.status = 400;
      res.set_content(R"({"error":"missing id"})", "application/json");
      return;
    }
    long long id = 0;
    try {
      id = std::stoll(req.get_param_value("id"));
    } catch (const std::exception&) {
      res.status = 400;
      res.set_content(R"({"error":"invalid id"})", "application/json");
      return;
    }
    bool removed = manager.deleteTransaction(id);
    if (!removed) {
      res.status = 404;
      res.set_content(R"({"error":"not found"})", "application/json");
      return;
    }
    res.set_content(R"({"status":"ok"})", "application/json");
  });

  svr.set_mount_point("/", "./www");
}

void printTransactions(const std::vector<Transaction>& transactions) {
  if (transactions.empty()) {
    std::cout << "No transactions available.\n";
    return;
  }
  for (const auto& tx : transactions) {
    std::cout << "[" << tx.id << "] " << tx.date << " " << tx.type << " "
              << tx.category << " " << tx.memo << " " << tx.amount << "\n";
  }
}

}  // namespace

int main() {
  AccountManager manager;
  manager.loadFromFile();

  httplib::Server svr;
  register_routes(svr, manager);

  std::thread server([&svr]() {
    std::cout << "Server listening on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
  });

  bool running = true;
  while (running) {
    std::cout << "\n=== Household Ledger ===\n";
    std::cout << "1. View transactions\n";
    std::cout << "2. Add transaction\n";
    std::cout << "3. Exit\n";
    std::cout << "Select option: ";

    int choice = 0;
    if (!(std::cin >> choice)) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    if (choice == 1) {
      auto all = manager.getAllTransactions();
      printTransactions(all);
    } else if (choice == 2) {
      Transaction tx{};
      tx.id = generateId();
      std::cout << "Date (YYYY-MM-DD): ";
      std::cin >> tx.date;
      std::cout << "Type (income/expense): ";
      std::cin >> tx.type;
      std::cout << "Category: ";
      std::cin >> tx.category;
      std::cout << "Memo: ";
      std::cin >> tx.memo;
      std::cout << "Amount: ";
      std::cin >> tx.amount;
      manager.addTransaction(tx);
      std::cout << "Transaction added.\n";
    } else if (choice == 3) {
      running = false;
    }
  }

  std::cout << "Stopping server...\n";
  svr.stop();
  server.join();
  return 0;
}
