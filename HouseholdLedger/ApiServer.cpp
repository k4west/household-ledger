#include "ApiServer.h"

#include "RpgService.h"
#include "Utils.h"
#include "json.hpp"

using json = nlohmann::json;

void register_routes(httplib::Server& svr,
                     AccountManager& manager,
                     CategoryManager& categories,
                     BudgetManager& budgets,
                     ScheduleManager& schedules,
                     const std::string& mountPath) {
    svr.set_default_headers({ {"Access-Control-Allow-Origin", "*"} });

    svr.Get("/api/transactions", [&manager, &categories](const httplib::Request& req, httplib::Response& res) {
        auto yearParam = parseIntParam(req, "year");
        auto monthParam = parseIntParam(req, "month");
        if (req.has_param("year") && !yearParam) {
            res.status = 400;
            res.set_content(R"({"error":"invalid year"})", "application/json");
            return;
        }
        if (req.has_param("month") && !monthParam) {
            res.status = 400;
            res.set_content(R"({"error":"invalid month"})", "application/json");
            return;
        }

        if (yearParam && monthParam) {
            auto monthly = manager.getTransactionsForMonth(*yearParam, *monthParam);
            json payload = normalizeTransactions(monthly, categories);
            res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
			return;
		}

		if ( yearParam ) {
			auto yearly = manager.getTransactionsForYear(*yearParam);
			json payload = normalizeTransactions(yearly, categories);
			res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
			return;
		}

		auto current = getCurrentYearMonth( );
		auto monthly = manager.getTransactionsForMonth(current.first, current.second);
		json payload = normalizeTransactions(monthly, categories);
		res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
		});

	svr.Post("/api/transactions", [ &manager, &categories ] (const httplib::Request& req, httplib::Response& res) {
		try {
			auto payload = json::parse(req.body);
			Transaction tx = payload.get<Transaction>( );
			tx.category = categories.normalizeCategory(tx.category);
			manager.addTransaction(tx);
			res.status = 201;
			res.set_content(R"({"status":"ok"})", "application/json");
		}
		catch ( const std::exception& ex ) {
			json err{ {"error", ex.what( )} };
			res.status = 400;
			res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
		}
		});

	svr.Put("/api/transactions", [ &manager, &categories ] (const httplib::Request& req, httplib::Response& res) {
		try {
			auto payload = json::parse(req.body);
			Transaction tx = payload.get<Transaction>( );
			tx.category = categories.normalizeCategory(tx.category);
			if ( !manager.updateTransaction(tx) ) {
				res.status = 404;
				res.set_content(R"({"error":"not found"})", "application/json");
				return;
			}
			res.set_content(R"({"status":"ok"})", "application/json");
		}
		catch ( const std::exception& ex ) {
			json err{ {"error", ex.what( )} };
			res.status = 400;
			res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
		}
		});

	svr.Delete("/api/transactions", [ &manager ] (const httplib::Request& req, httplib::Response& res) {
		if ( !req.has_param("id") ) {
			res.status = 400;
			res.set_content(R"({"error":"missing id"})", "application/json");
			return;
		}
		long long id = 0;
		try {
			id = std::stoll(req.get_param_value("id"));
		}
		catch ( const std::exception& ) {
			res.status = 400;
			res.set_content(R"({"error":"invalid id"})", "application/json");
			return;
		}
		bool removed = manager.deleteTransaction(id);
		if ( !removed ) {
			res.status = 404;
			res.set_content(R"({"error":"not found"})", "application/json");
			return;
		}
		res.set_content(R"({"status":"ok"})", "application/json");
		});

	svr.Get("/api/categories", [ &categories ] (const httplib::Request&, httplib::Response& res) {
		json payload = categories.getCategories( );
		res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
		});

	svr.Post("/api/categories", [ &categories ] (const httplib::Request& req, httplib::Response& res) {
		try {
			auto payload = json::parse(req.body);
			std::string name = payload.value("name", "");
			std::string attributeValue = payload.value("attribute", "consumption");
			CategoryManager::CategoryAttribute attribute{};
			if ( !CategoryManager::tryParseAttribute(attributeValue, attribute) ) {
				res.status = 400;
				res.set_content(R"({"error":"invalid attribute"})", "application/json");
				return;
			}
			if ( !categories.addCategory(name, attribute) ) {
				res.status = 409;
				res.set_content(R"({"error":"duplicate or invalid category"})", "application/json");
				return;
			}
			res.status = 201;
			res.set_content(R"({"status":"ok"})", "application/json");
		}
		catch ( const std::exception& ex ) {
			json err{ {"error", ex.what( )} };
			res.status = 400;
			res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
		}
		});

	svr.Delete("/api/categories", [ &categories ] (const httplib::Request& req, httplib::Response& res) {
		if ( !req.has_param("name") ) {
			res.status = 400;
			res.set_content(R"({"error":"missing name"})", "application/json");
			return;
		}
		std::string name = req.get_param_value("name");
		if ( !categories.removeCategory(name) ) {
			res.status = 409;
			res.set_content(R"({"error":"cannot remove category"})", "application/json");
			return;
		}
		res.set_content(R"({"status":"ok"})", "application/json");
		});

	svr.Get("/api/category-attributes", [ &categories ] (const httplib::Request&, httplib::Response& res) {
		json payload = json::object( );
		for ( const auto& [name, attribute] : categories.getCategoryAttributes( ) ) {
			payload[ name ] = CategoryManager::attributeToString(attribute);
		}
		res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
		});

	svr.Post("/api/category-attributes", [ &categories ] (const httplib::Request& req, httplib::Response& res) {
		try {
			auto payload = json::parse(req.body);
			std::string name = payload.value("name", "");
			std::string attributeValue = payload.value("attribute", "");
			if ( name.empty( ) ) {
				res.status = 400;
				res.set_content(R"({"error":"missing name"})", "application/json");
				return;
			}
			CategoryManager::CategoryAttribute attribute{};
			if ( !CategoryManager::tryParseAttribute(attributeValue, attribute) ) {
				res.status = 400;
				res.set_content(R"({"error":"invalid attribute"})", "application/json");
				return;
			}
			if ( !categories.setCategoryAttribute(name, attribute) ) {
				res.status = 404;
				res.set_content(R"({"error":"not found"})", "application/json");
				return;
			}
			res.set_content(R"({"status":"ok"})", "application/json");
		}
		catch ( const std::exception& ex ) {
			json err{ {"error", ex.what( )} };
			res.status = 400;
			res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
		}
		});

	svr.Get("/api/budget", [ &budgets, &categories ] (const httplib::Request& req, httplib::Response& res) {
		auto yearParam = parseIntParam(req, "year");
		if ( !yearParam ) {
			res.status = 400;
			res.set_content(R"({"error":"missing year"})", "application/json");
			return;
		}
		json payload = budgets.getBudgetForYear(*yearParam);
		normalizeBudgetCategories(payload, categories);
		res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
		});

	svr.Get("/api/rpg", [ &manager, &budgets, &categories ] (const httplib::Request& req, httplib::Response& res) {
		auto yearParam = parseIntParam(req, "year");
		auto monthParam = parseIntParam(req, "month");
		if ( req.has_param("year") && !yearParam ) {
			res.status = 400;
			res.set_content(R"({"error":"invalid year"})", "application/json");
			return;
		}
		if ( req.has_param("month") && !monthParam ) {
			res.status = 400;
			res.set_content(R"({"error":"invalid month"})", "application/json");
			return;
		}

		auto current = getCurrentYearMonth( );
		int year = yearParam.value_or(current.first);
		bool isYearOnly = req.has_param("year") && !req.has_param("month");

		auto budgetYear = budgets.getBudgetForYear(year);
		RpgSummary summary{};
		int payloadMonth = 0;
		if ( monthParam ) {
			int month = *monthParam;
			auto monthly = manager.getTransactionsForMonth(year, month);
			summary = computeRpgSummary(normalizeTransactions(monthly, categories), budgetYear, year, month, categories);
			payloadMonth = month;
		}
		else if ( isYearOnly ) {
			auto yearly = manager.getTransactionsForYear(year);
			summary = computeRpgSummaryYear(normalizeTransactions(yearly, categories), budgetYear, year, categories);
			payloadMonth = 0;
		}
		else {
			int month = current.second;
			auto monthly = manager.getTransactionsForMonth(current.first, month);
			summary = computeRpgSummary(normalizeTransactions(monthly, categories), budgetYear, current.first, month, categories);
			year = current.first;
			payloadMonth = month;
		}

		json payload{
			{"year", year},
			{"month", payloadMonth},
			{"income", summary.income},
			{"expense", summary.expense},
			{"saving", summary.saving},
			{"budget", summary.budgetTotal},
			{"saving_goal", summary.savingGoal},
			{"goal_multiplier", summary.goalMultiplier},
			{"spend_bonus", summary.spendBonus},
			{"save_bonus", summary.saveBonus},
			{"total_multiplier", summary.totalMultiplier},
			{"total_exp", summary.totalExp},
			{"level", summary.level},
			{"party_hp", summary.partyHp},
			{"roles", {
				{"hunter", summary.hunterCount},
				{"guardian", summary.guardianCount},
				{"cleric", summary.clericCount},
				{"gremlin", summary.gremlinCount}
			}},
			{"gremlin_level", summary.gremlinLevel}
		};
		res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
		});

	svr.Post("/api/budget", [ &budgets, &categories ] (const httplib::Request& req, httplib::Response& res) {
		try {
			auto payload = json::parse(req.body);
			int year = payload.value("year", 0);
			if ( year <= 0 ) {
				res.status = 400;
				res.set_content(R"({"error":"invalid year"})", "application/json");
				return;
			}
			normalizeBudgetCategories(payload, categories);
			budgets.upsertBudget(year, payload);
			res.set_content(R"({"status":"ok"})", "application/json");
		}
		catch ( const std::exception& ex ) {
			json err{ {"error", ex.what( )} };
			res.status = 400;
			res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
		}
		});

	svr.Get("/api/schedules", [ &schedules ] (const httplib::Request&, httplib::Response& res) {
		json payload = schedules.getSchedules( );
		res.set_content(payload.dump(2, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
    });

    svr.Post("/api/schedules", [&schedules, &manager](const httplib::Request& req, httplib::Response& res) {
        try {
            auto payload = json::parse(req.body);
            ScheduleItem item = payload.get<ScheduleItem>();
            if (item.id == 0) {
                item.id = generateId();
            }
            schedules.addSchedule(item);
            schedules.generateDueTransactions(manager);
            res.status = 201;
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (const std::exception& ex) {
            json err{ {"error", ex.what()} };
            res.status = 400;
            res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
        }
    });

    svr.Put("/api/schedules", [&schedules](const httplib::Request& req, httplib::Response& res) {
        try {
            auto payload = json::parse(req.body);
            ScheduleItem item = payload.get<ScheduleItem>();
            if (!schedules.updateSchedule(item)) {
                res.status = 404;
                res.set_content(R"({"error":"not found"})", "application/json");
                return;
            }
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (const std::exception& ex) {
            json err{ {"error", ex.what()} };
            res.status = 400;
            res.set_content(err.dump(2, ' ', false, json::error_handler_t::replace), "application/json");
        }
    });

    svr.Delete("/api/schedules", [&schedules](const httplib::Request& req, httplib::Response& res) {
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
        if (!schedules.deleteSchedule(id)) {
            res.status = 404;
            res.set_content(R"({"error":"not found"})", "application/json");
            return;
        }
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    svr.set_mount_point("/", mountPath.c_str());
}
