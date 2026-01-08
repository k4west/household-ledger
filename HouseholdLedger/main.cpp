#include <iostream>
#include <thread>

#include "AccountManager.h"
#include "ApiServer.h"
#include "BudgetManager.h"
#include "CategoryManager.h"
#include "ConsoleUi.h"
#include "ScheduleManager.h"

int main() {
    AccountManager manager;
    CategoryManager categories;
    BudgetManager budgets;
    ScheduleManager schedules;
    categories.loadFromFile();
    schedules.generateDueTransactions(manager);

    httplib::Server svr;
    register_routes(svr, manager, categories, budgets, schedules);

    std::thread server([&svr]() {
        std::cout << "\nServer listening on http://localhost:8888\n";
        svr.listen("0.0.0.0", 8888);
    });

    runConsoleLoop(manager, categories);

    std::cout << "Stopping server...\n";
    svr.stop();
    server.join();
    return 0;
}
