#pragma once

#include <string>
#include "json.hpp"

enum class TransactionType {
	INCOME,
	EXPENSE,
	TRANSFER,
	SAVING
};

NLOHMANN_JSON_SERIALIZE_ENUM(TransactionType, {
	{TransactionType::INCOME, "income"},
	{TransactionType::EXPENSE, "expense"},
	{TransactionType::TRANSFER, "transfer"},
	{TransactionType::SAVING, "saving"},
})

struct Transaction {
	long long id{};
	std::string date;
	TransactionType type{ TransactionType::EXPENSE };
	std::string category;
	std::string memo;
	long long amount{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transaction, id, date, type, category, memo, amount)
