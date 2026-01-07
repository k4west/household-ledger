const API_BASE = "http://localhost:8888/api";
const BUDGET_ENDPOINT = `${API_BASE}/budget`;
const TRANSACTIONS_ENDPOINT = `${API_BASE}/transactions`;
const CATEGORIES_ENDPOINT = `${API_BASE}/categories`;

let budgetData = {};
let categories = [];
let transactions = [];
let comboChart = null;
let gaugeChart = null;

const elements = {
  yearSelect: document.getElementById("budget-year"),
  editBudgetBtn: document.getElementById("edit-budget-btn"),
  annualSavingsGoal: document.getElementById("annual-savings-goal"),
  annualSavingsRate: document.getElementById("annual-savings-rate"),
  annualExpenseLimit: document.getElementById("annual-expense-limit"),
  annualExpenseCurrent: document.getElementById("annual-expense-current"),
  currentMonthBudget: document.getElementById("current-month-budget"),
  currentMonthExpense: document.getElementById("current-month-expense"),
  savingsGaugeLabel: document.getElementById("savings-gauge-label"),
  editModal: document.getElementById("budgetEditModal"),
  editAnnualSavings: document.getElementById("edit-annual-savings"),
  editAnnualExpense: document.getElementById("edit-annual-expense"),
  editMonthSelect: document.getElementById("edit-month-select"),
  copyPrevMonthBtn: document.getElementById("copy-prev-month"),
  editMonthlySavings: document.getElementById("edit-monthly-savings"),
  editBudgetRows: document.getElementById("edit-budget-rows"),
  saveBudgetBtn: document.getElementById("save-budget-btn"),
};

function formatAmount(value) {
  return Number(value || 0).toLocaleString();
}

function getSelectedYear() {
  return Number(elements.yearSelect.value);
}

function getCurrentMonthKey() {
  const now = new Date();
  return String(now.getMonth() + 1).padStart(2, "0");
}

function buildYearOptions() {
  const now = new Date();
  const currentYear = now.getFullYear();
  elements.yearSelect.innerHTML = "";
  for (let offset = -1; offset <= 1; offset += 1) {
    const year = currentYear + offset;
    const option = document.createElement("option");
    option.value = year;
    option.textContent = `${year}년`;
    if (offset === 0) {
      option.selected = true;
    }
    elements.yearSelect.appendChild(option);
  }
}

function buildMonthOptions() {
  elements.editMonthSelect.innerHTML = "";
  for (let month = 1; month <= 12; month += 1) {
    const option = document.createElement("option");
    option.value = String(month).padStart(2, "0");
    option.textContent = `${month}월`;
    if (month === new Date().getMonth() + 1) {
      option.selected = true;
    }
    elements.editMonthSelect.appendChild(option);
  }
}

async function fetchCategories() {
  try {
    const response = await fetch(CATEGORIES_ENDPOINT);
    categories = await response.json();
  } catch (error) {
    console.error("Failed to fetch categories", error);
  }
}

async function fetchBudget() {
  const year = getSelectedYear();
  try {
    const response = await fetch(`${BUDGET_ENDPOINT}?year=${year}`);
    budgetData = await response.json();
  } catch (error) {
    console.error("Failed to fetch budget", error);
    budgetData = {};
  }
}

async function fetchTransactions() {
  const year = getSelectedYear();
  try {
    const response = await fetch(`${TRANSACTIONS_ENDPOINT}?year=${year}`);
    transactions = await response.json();
  } catch (error) {
    console.error("Failed to fetch transactions", error);
    transactions = [];
  }
}

function monthlyExpenseTotals() {
  const totals = Array.from({ length: 12 }, () => 0);
  transactions
    .filter((tx) => tx.type === "expense")
    .forEach((tx) => {
      const month = Number(tx.date?.split("-")[1]) || 0;
      if (month >= 1 && month <= 12) {
        totals[month - 1] += Number(tx.amount || 0);
      }
    });
  return totals;
}

function monthlyBudgetTotals() {
  const totals = Array.from({ length: 12 }, () => 0);
  if (!budgetData.monthly) {
    return totals;
  }
  Object.entries(budgetData.monthly).forEach(([month, data]) => {
    const idx = Number(month) - 1;
    if (idx < 0 || idx >= 12) return;
    const expenses = data?.expenses || {};
    totals[idx] = Object.values(expenses).reduce((sum, value) => sum + Number(value || 0), 0);
  });
  return totals;
}

function renderSummary() {
  const annualGoals = budgetData.annual_goals || {};
  const annualSavingsGoal = annualGoals.total_savings || 0;
  const annualExpenseLimit = annualGoals.total_expense_limit || 0;
  const expenseTotal = transactions
    .filter((tx) => tx.type === "expense")
    .reduce((sum, tx) => sum + Number(tx.amount || 0), 0);
  const savingsTotal = transactions
    .filter((tx) => tx.type === "saving")
    .reduce((sum, tx) => sum + Number(tx.amount || 0), 0);
  const monthKey = getCurrentMonthKey();
  const currentMonthBudget = budgetData.monthly?.[monthKey]?.expenses
    ? Object.values(budgetData.monthly[monthKey].expenses).reduce(
        (sum, value) => sum + Number(value || 0),
        0
      )
    : 0;
  const currentMonthExpense = transactions
    .filter((tx) => tx.type === "expense" && tx.date?.split("-")[1] === monthKey)
    .reduce((sum, tx) => sum + Number(tx.amount || 0), 0);

  elements.annualSavingsGoal.textContent = formatAmount(annualSavingsGoal);
  elements.annualExpenseLimit.textContent = formatAmount(annualExpenseLimit);
  elements.annualExpenseCurrent.textContent = formatAmount(expenseTotal);
  elements.currentMonthBudget.textContent = formatAmount(currentMonthBudget);
  elements.currentMonthExpense.textContent = formatAmount(currentMonthExpense);

  const rate = annualSavingsGoal > 0 ? Math.min(savingsTotal / annualSavingsGoal, 1) : 0;
  elements.annualSavingsRate.textContent = `${Math.round(rate * 100)}%`;
}

function buildComboChart() {
  const budgetTotals = monthlyBudgetTotals();
  const expenseTotals = monthlyExpenseTotals();
  const labels = Array.from({ length: 12 }, (_, idx) => `${idx + 1}월`);

  if (!comboChart) {
    const ctx = document.getElementById("budgetComboChart");
    comboChart = new Chart(ctx, {
      type: "bar",
      data: {
        labels,
        datasets: [
          {
            label: "예산",
            data: budgetTotals,
            backgroundColor: "rgba(30, 136, 229, 0.6)",
            borderRadius: 6,
          },
          {
            label: "실제 지출",
            data: expenseTotals,
            type: "line",
            borderColor: "#e53935",
            backgroundColor: "#e53935",
            tension: 0.3,
          },
        ],
      },
      options: {
        responsive: true,
        plugins: {
          legend: { position: "bottom" },
        },
        scales: {
          y: { beginAtZero: true },
        },
      },
    });
    return;
  }

  comboChart.data.datasets[0].data = budgetTotals;
  comboChart.data.datasets[1].data = expenseTotals;
  comboChart.update();
}

function buildGaugeChart() {
  const annualGoals = budgetData.annual_goals || {};
  const annualSavingsGoal = Number(annualGoals.total_savings || 0);
  const savingsTotal = transactions
    .filter((tx) => tx.type === "saving")
    .reduce((sum, tx) => sum + Number(tx.amount || 0), 0);
  const remaining = Math.max(annualSavingsGoal - savingsTotal, 0);

  if (!gaugeChart) {
    const ctx = document.getElementById("savingsGaugeChart");
    gaugeChart = new Chart(ctx, {
      type: "doughnut",
      data: {
        labels: ["달성", "남은 목표"],
        datasets: [
          {
            data: [savingsTotal, remaining],
            backgroundColor: ["#43a047", "#e0e0e0"],
            borderWidth: 0,
          },
        ],
      },
      options: {
        cutout: "70%",
        plugins: {
          legend: { display: false },
        },
      },
    });
  } else {
    gaugeChart.data.datasets[0].data = [savingsTotal, remaining];
    gaugeChart.update();
  }

  const rate = annualSavingsGoal > 0 ? Math.min((savingsTotal / annualSavingsGoal) * 100, 100) : 0;
  elements.savingsGaugeLabel.textContent = `달성률 ${rate.toFixed(1)}% · ${formatAmount(
    savingsTotal
  )} / ${formatAmount(annualSavingsGoal)}`;
}

function buildBudgetRows() {
  const monthKey = elements.editMonthSelect.value;
  const monthData = budgetData.monthly?.[monthKey] || {};
  const expenses = monthData.expenses || {};
  elements.editBudgetRows.innerHTML = "";
  categories.forEach((category) => {
    const row = document.createElement("tr");
    row.innerHTML = `
      <td>${category}</td>
      <td class="text-end">
        <input class="form-control form-control-sm text-end" type="number" min="0"
          data-category="${category}" value="${expenses[category] || 0}" />
      </td>
    `;
    elements.editBudgetRows.appendChild(row);
  });
  elements.editMonthlySavings.value = monthData.goals?.monthly_savings || 0;
}

function copyPreviousMonth() {
  const month = Number(elements.editMonthSelect.value);
  const prevMonth = month === 1 ? "12" : String(month - 1).padStart(2, "0");
  const prevData = budgetData.monthly?.[prevMonth];
  if (!prevData) return;
  const expenses = prevData.expenses || {};
  elements.editBudgetRows.querySelectorAll("input[data-category]").forEach((input) => {
    const category = input.dataset.category;
    input.value = expenses[category] || 0;
  });
  elements.editMonthlySavings.value = prevData.goals?.monthly_savings || 0;
}

async function saveBudget() {
  const year = getSelectedYear();
  const monthKey = elements.editMonthSelect.value;
  const expenses = {};
  elements.editBudgetRows.querySelectorAll("input[data-category]").forEach((input) => {
    expenses[input.dataset.category] = Number(input.value || 0);
  });
  const payload = {
    year,
    annual_goals: {
      total_savings: Number(elements.editAnnualSavings.value || 0),
      total_expense_limit: Number(elements.editAnnualExpense.value || 0),
    },
    monthly: {
      [monthKey]: {
        expenses,
        goals: {
          monthly_savings: Number(elements.editMonthlySavings.value || 0),
        },
      },
    },
  };

  try {
    await fetch(BUDGET_ENDPOINT, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    const modal = bootstrap.Modal.getInstance(elements.editModal);
    if (modal) {
      modal.hide();
    }
    await refreshData();
  } catch (error) {
    console.error("Failed to save budget", error);
  }
}

function openEditModal() {
  elements.editAnnualSavings.value = budgetData.annual_goals?.total_savings || 0;
  elements.editAnnualExpense.value = budgetData.annual_goals?.total_expense_limit || 0;
  buildBudgetRows();
  const modal = new bootstrap.Modal(elements.editModal);
  modal.show();
}

async function refreshData() {
  await Promise.all([fetchBudget(), fetchTransactions()]);
  renderSummary();
  buildComboChart();
  buildGaugeChart();
}

document.addEventListener("DOMContentLoaded", async () => {
  buildYearOptions();
  buildMonthOptions();
  await fetchCategories();
  await refreshData();

  elements.yearSelect.addEventListener("change", refreshData);
  elements.editBudgetBtn.addEventListener("click", openEditModal);
  elements.editMonthSelect.addEventListener("change", buildBudgetRows);
  elements.copyPrevMonthBtn.addEventListener("click", copyPreviousMonth);
  elements.saveBudgetBtn.addEventListener("click", saveBudget);
});
