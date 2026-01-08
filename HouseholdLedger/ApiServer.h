#pragma once

#include "AccountManager.h"
#include "BudgetManager.h"
#include "CategoryManager.h"
#include "ScheduleManager.h"
#include "httplib.h"

void register_routes(httplib::Server& svr,
                     AccountManager& manager,
                     CategoryManager& categories,
                     BudgetManager& budgets,
                     ScheduleManager& schedules);
