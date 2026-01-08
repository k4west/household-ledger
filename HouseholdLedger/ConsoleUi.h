#pragma once

#include "AccountManager.h"
#include "CategoryManager.h"

void printTransactions(const std::vector<Transaction>& transactions);
void runConsoleLoop(AccountManager& manager, CategoryManager& categories);
