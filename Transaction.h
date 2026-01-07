#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

#include "json.hpp"

struct Transaction {
  long long id;
  std::string date;
  std::string type;
  std::string category;
  std::string memo;
  long long amount;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transaction, id, date, type, category, memo, amount)

#endif
