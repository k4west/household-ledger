#include <chrono>
#include <iostream>
#include <limits>
#include <thread>

#include "AccountManager.h"
#include "Transaction.h"
#include "httplib.h"
#include "json.hpp"

namespace {

long long generateId() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

void addCorsHeaders(httplib::Response &res) {
  res.set_header("Access-Control-Allow-Origin", "*");
  res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
  res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

void configureServer(httplib::Server &svr, AccountManager &manager) {
  svr.set_pre_routing_handler([](const httplib::Request &, httplib::Response &res) {
    addCorsHeaders(res);
    return httplib::Server::HandlerResponse::Unhandled;
  });

  svr.Options(R"(/.*)", [](const httplib::Request &, httplib::Response &res) {
    addCorsHeaders(res);
    res.status = 204;
  });

  svr.Get("/api/transactions", [&manager](const httplib::Request &, httplib::Response &res) {
    auto transactions = manager.getAllTransactions();
    nlohmann::json data = transactions;
    addCorsHeaders(res);
    res.set_content(data.dump(), "application/json");
  });

  svr.Post("/api/transactions", [&manager](const httplib::Request &req, httplib::Response &res) {
    try {
      auto payload = nlohmann::json::parse(req.body);
      Transaction tx = payload.get<Transaction>();
      if (tx.id == 0) {
        tx.id = generateId();
      }
      manager.addTransaction(tx);
      addCorsHeaders(res);
      res.set_content(nlohmann::json(tx).dump(), "application/json");
    } catch (const std::exception &e) {
      res.status = 400;
      res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
    }
  });

  svr.Delete("/api/transactions", [&manager](const httplib::Request &req, httplib::Response &res) {
    if (!req.has_param("id")) {
      res.status = 400;
      res.set_content("Missing id parameter", "text/plain");
      return;
    }
    long long id = 0;
    try {
      id = std::stoll(req.get_param_value("id"));
    } catch (const std::exception &) {
      res.status = 400;
      res.set_content("Invalid id parameter", "text/plain");
      return;
    }
    bool deleted = manager.deleteTransaction(id);
    addCorsHeaders(res);
    res.set_content(nlohmann::json{{"deleted", deleted}}.dump(), "application/json");
  });

  svr.set_mount_point("/", "./www");
}

void printMenu() {
  std::cout << "\n[Household Ledger]" << std::endl;
  std::cout << "1. 거래 내역 조회" << std::endl;
  std::cout << "2. 거래 추가" << std::endl;
  std::cout << "3. 서버 종료" << std::endl;
  std::cout << "선택: ";
}

void printTransactions(const std::vector<Transaction> &transactions) {
  if (transactions.empty()) {
    std::cout << "저장된 거래가 없습니다." << std::endl;
    return;
  }
  for (const auto &tx : transactions) {
    std::cout << "ID: " << tx.id << " | 날짜: " << tx.date << " | 유형: " << tx.type
              << " | 분류: " << tx.category << " | 금액: " << tx.amount
              << " | 메모: " << tx.memo << std::endl;
  }
}

Transaction promptTransaction() {
  Transaction tx{};
  tx.id = generateId();
  std::cout << "날짜(YYYY-MM-DD): ";
  std::cin >> tx.date;
  std::cout << "유형(income/expense): ";
  std::cin >> tx.type;
  std::cout << "카테고리: ";
  std::cin >> tx.category;
  std::cout << "메모: ";
  std::cin.ignore();
  std::getline(std::cin, tx.memo);
  std::cout << "금액: ";
  std::cin >> tx.amount;
  return tx;
}

}  // namespace

int main() {
  AccountManager manager;
  manager.loadFromFile();

  httplib::Server svr;
  configureServer(svr, manager);

  std::thread server_thread([&svr]() { svr.listen("0.0.0.0", 8080); });

  bool running = true;
  while (running) {
    printMenu();
    int choice = 0;
    if (!(std::cin >> choice)) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    switch (choice) {
      case 1: {
        auto transactions = manager.getAllTransactions();
        printTransactions(transactions);
        std::cout << "현재 잔액: " << manager.getBalance() << std::endl;
        break;
      }
      case 2: {
        Transaction tx = promptTransaction();
        manager.addTransaction(tx);
        std::cout << "거래가 추가되었습니다." << std::endl;
        break;
      }
      case 3:
        running = false;
        break;
      default:
        std::cout << "올바른 번호를 선택하세요." << std::endl;
        break;
    }
  }

  svr.stop();
  if (server_thread.joinable()) {
    server_thread.join();
  }

  return 0;
}
