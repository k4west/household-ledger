const API_BASE = "http://localhost:8888/api";
const TRANSACTIONS_ENDPOINT = `${API_BASE}/transactions`;
const CATEGORIES_ENDPOINT = `${API_BASE}/categories`;

let expenseChart = null;
let summaryChart = null;
let transactions = [];
let categories = [];
let selectedYear = null;
let selectedMonth = null;
let activeTypeFilters = new Set(["income", "expense", "transfer"]);
let activeCategoryFilters = new Set();
let isYearView = false;

const elements = {
  list: document.getElementById("transaction-list"),
  totalIncome: document.getElementById("total-income"),
  totalExpense: document.getElementById("total-expense"),
  totalTransfer: document.getElementById("total-transfer"),
  remainingBudget: document.getElementById("remaining-budget"),
  date: document.getElementById("tx-date"),
  type: document.getElementById("tx-type"),
  category: document.getElementById("tx-category"),
  memo: document.getElementById("tx-memo"),
  amount: document.getElementById("tx-amount"),
  monthLabel: document.getElementById("current-month"),
  prevMonth: document.getElementById("prev-month"),
  nextMonth: document.getElementById("next-month"),
  viewMonth: document.getElementById("view-month"),
  viewYear: document.getElementById("view-year"),
  filterCategory: document.getElementById("filter-category"),
  filterTypeInputs: document.querySelectorAll("input[name='filter-type']"),
  addCategoryBtn: document.getElementById("add-category-btn"),
  categoryInput: document.getElementById("new-category-name"),
  saveCategoryBtn: document.getElementById("save-category-btn"),
  editModal: document.getElementById("editTransactionModal"),
  editDate: document.getElementById("edit-date"),
  editType: document.getElementById("edit-type"),
  editCategory: document.getElementById("edit-category"),
  editMemo: document.getElementById("edit-memo"),
  editAmount: document.getElementById("edit-amount"),
  editSaveBtn: document.getElementById("save-edit-btn"),
};

function formatAmount(value) {
  return Number(value).toLocaleString();
}

function formatType(type) {
  if (type === "income") return "수입";
  if (type === "transfer") return "저축";
  return "지출";
}

function updateMonthLabel() {
  elements.monthLabel.textContent = isYearView
    ? `${selectedYear}년 전체`
    : `${selectedYear}년 ${selectedMonth}월`;
}

function calculateTotals(list) {
  return list.reduce(
    (acc, tx) => {
      if (tx.type === "income") {
        acc.income += Number(tx.amount || 0);
      } else if (tx.type === "expense") {
        acc.expense += Number(tx.amount || 0);
      } else if (tx.type === "transfer") {
        acc.transfer += Number(tx.amount || 0);
      }
      return acc;
    },
    { income: 0, expense: 0, transfer: 0 }
  );
}

function renderSummary(totals) {
  elements.totalIncome.textContent = formatAmount(totals.income);
  elements.totalExpense.textContent = formatAmount(totals.expense);
  elements.totalTransfer.textContent = formatAmount(totals.transfer);
  elements.remainingBudget.textContent = formatAmount(
    totals.income - totals.expense - totals.transfer
  );
}

function renderTable(list) {
  elements.list.innerHTML = "";

  list
    .sort((a, b) => b.id - a.id)
    .forEach((tx) => {
      const row = document.createElement("tr");
      let amountClass = "text-expense";
      if (tx.type === "income") amountClass = "text-income";
      if (tx.type === "transfer") amountClass = "text-transfer";

      row.innerHTML = `
        <td>${tx.date || "-"}</td>
        <td>${tx.category || "-"}</td>
        <td>
          <span class="badge bg-light text-dark badge-type me-2">${formatType(tx.type)}</span>
          ${tx.memo || "-"}
        </td>
        <td class="text-end ${amountClass}">${formatAmount(tx.amount)}</td>
        <td class="text-end">
          <button class="btn btn-sm btn-outline-secondary me-2" data-action="edit" data-id="${tx.id}">
            수정
          </button>
          <button class="btn btn-sm btn-outline-danger" data-action="delete" data-id="${tx.id}">
            <i class="fa-solid fa-trash"></i>
          </button>
        </td>
      `;

      elements.list.appendChild(row);
    });
}

function buildExpenseChart(list) {
  const expenseTotals = list
    .filter((tx) => tx.type === "expense")
    .reduce((acc, tx) => {
      const key = tx.category || "기타";
      acc[key] = (acc[key] || 0) + Number(tx.amount || 0);
      return acc;
    }, {});

  const labels = Object.keys(expenseTotals);
  const data = Object.values(expenseTotals);

  const chartContainer = document.getElementById("expenseChart").parentElement;
  if (labels.length === 0) {
    if (!chartContainer.querySelector(".chart-empty")) {
      const placeholder = document.createElement("div");
      placeholder.className = "chart-empty";
      placeholder.textContent = "지출 데이터가 없습니다.";
      chartContainer.appendChild(placeholder);
    }
  } else {
    const placeholder = chartContainer.querySelector(".chart-empty");
    if (placeholder) {
      placeholder.remove();
    }
  }

  if (!expenseChart) {
    const ctx = document.getElementById("expenseChart");
    expenseChart = new Chart(ctx, {
      type: "pie",
      data: {
        labels,
        datasets: [
          {
            data,
            backgroundColor: [
              "#ef5350",
              "#ab47bc",
              "#5c6bc0",
              "#29b6f6",
              "#66bb6a",
              "#ffa726",
              "#8d6e63",
            ],
          },
        ],
      },
      options: {
        plugins: {
          legend: {
            position: "bottom",
          },
        },
      },
    });
    return;
  }

  expenseChart.data.labels = labels;
  expenseChart.data.datasets[0].data = data;
  expenseChart.update();
}

function buildSummaryChart(totals) {
  const labels = ["수입", "지출", "저축"];
  const data = [totals.income, totals.expense, totals.transfer];

  if (!summaryChart) {
    const ctx = document.getElementById("summaryChart");
    summaryChart = new Chart(ctx, {
      type: "bar",
      data: {
        labels,
        datasets: [
          {
            data,
            backgroundColor: ["#1e88e5", "#e53935", "#43a047"],
          },
        ],
      },
      options: {
        plugins: {
          legend: { display: false },
        },
        scales: {
          y: {
            beginAtZero: true,
          },
        },
      },
    });
    return;
  }

  summaryChart.data.datasets[0].data = data;
  summaryChart.update();
}

function applyFilters() {
  return transactions.filter((tx) => {
    if (!activeTypeFilters.has(tx.type)) return false;
    if (activeCategoryFilters.size > 0 && !activeCategoryFilters.has(tx.category)) return false;
    return true;
  });
}

function renderDashboard() {
  const filtered = applyFilters();
  renderTable(filtered);
  const totals = calculateTotals(filtered);
  renderSummary(totals);
  buildExpenseChart(filtered);
  buildSummaryChart(totals);
}

function buildCategoryOptions(select, list, includeAll = false) {
  select.innerHTML = "";
  if (includeAll) {
    const option = document.createElement("option");
    option.value = "all";
    option.textContent = "전체";
    select.appendChild(option);
  }
  list.forEach((category) => {
    const option = document.createElement("option");
    option.value = category;
    option.textContent = category;
    select.appendChild(option);
  });
}

async function fetchCategories() {
  try {
    const response = await fetch(CATEGORIES_ENDPOINT);
    categories = await response.json();
    buildCategoryOptions(elements.category, categories);
    buildCategoryOptions(elements.editCategory, categories);
    buildCategoryOptions(elements.filterCategory, categories, true);
  } catch (error) {
    console.error("Failed to fetch categories", error);
  }
}

async function fetchTransactions() {
  try {
    let url = `${TRANSACTIONS_ENDPOINT}?year=${selectedYear}`;
    if (!isYearView) {
      const monthParam = String(selectedMonth).padStart(2, "0");
      url += `&month=${monthParam}`;
    }
    const response = await fetch(url);
    transactions = await response.json();
    renderDashboard();
  } catch (error) {
    console.error("Failed to fetch transactions", error);
  }
}

async function addTransaction() {
  const payload = {
    id: Date.now(),
    date: elements.date.value,
    type: elements.type.value,
    category: elements.category.value,
    memo: elements.memo.value.trim(),
    amount: Number(elements.amount.value),
  };

  if (!payload.date || !payload.amount || !payload.type || !payload.category) {
    return;
  }

  try {
    await fetch(TRANSACTIONS_ENDPOINT, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(payload),
    });

    elements.memo.value = "";
    elements.amount.value = "";
    fetchTransactions();
  } catch (error) {
    console.error("Failed to add transaction", error);
  }
}

async function updateTransaction(payload) {
  try {
    await fetch(TRANSACTIONS_ENDPOINT, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(payload),
    });
    fetchTransactions();
  } catch (error) {
    console.error("Failed to update transaction", error);
  }
}

async function deleteTransaction(id) {
  try {
    await fetch(`${TRANSACTIONS_ENDPOINT}?id=${id}`, {
      method: "DELETE",
    });
    fetchTransactions();
  } catch (error) {
    console.error("Failed to delete transaction", error);
  }
}

function openEditModal(tx) {
  elements.editDate.value = tx.date;
  elements.editType.value = tx.type;
  elements.editCategory.value = tx.category;
  elements.editMemo.value = tx.memo || "";
  elements.editAmount.value = tx.amount;
  elements.editSaveBtn.dataset.id = tx.id;
  const modal = new bootstrap.Modal(elements.editModal);
  modal.show();
}

async function addCategory() {
  const name = elements.categoryInput.value.trim();
  if (!name) return;
  try {
    await fetch(CATEGORIES_ENDPOINT, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ name }),
    });
    elements.categoryInput.value = "";
    await fetchCategories();
  } catch (error) {
    console.error("Failed to add category", error);
  }
}

function updateFiltersFromInputs() {
  activeTypeFilters = new Set();
  elements.filterTypeInputs.forEach((input) => {
    if (input.checked) {
      activeTypeFilters.add(input.value);
    }
  });
  activeCategoryFilters = new Set(
    Array.from(elements.filterCategory.selectedOptions)
      .map((option) => option.value)
      .filter((value) => value !== "all")
  );
  renderDashboard();
}

document.addEventListener("DOMContentLoaded", () => {
  const today = new Date();
  elements.date.value = today.toISOString().split("T")[0];
  selectedYear = today.getFullYear();
  selectedMonth = today.getMonth() + 1;
  updateMonthLabel();
  elements.viewMonth.classList.add("active");

  elements.prevMonth.addEventListener("click", () => {
    if (isYearView) {
      selectedYear -= 1;
    } else {
      selectedMonth -= 1;
      if (selectedMonth === 0) {
        selectedMonth = 12;
        selectedYear -= 1;
      }
    }
    updateMonthLabel();
    fetchTransactions();
  });

  elements.nextMonth.addEventListener("click", () => {
    if (isYearView) {
      selectedYear += 1;
    } else {
      selectedMonth += 1;
      if (selectedMonth === 13) {
        selectedMonth = 1;
        selectedYear += 1;
      }
    }
    updateMonthLabel();
    fetchTransactions();
  });

  elements.viewMonth.addEventListener("click", () => {
    isYearView = false;
    elements.viewMonth.classList.add("active");
    elements.viewYear.classList.remove("active");
    updateMonthLabel();
    fetchTransactions();
  });

  elements.viewYear.addEventListener("click", () => {
    isYearView = true;
    elements.viewYear.classList.add("active");
    elements.viewMonth.classList.remove("active");
    updateMonthLabel();
    fetchTransactions();
  });

  elements.list.addEventListener("click", (event) => {
    const target = event.target.closest("button");
    if (!target) return;
    const id = Number(target.dataset.id);
    const action = target.dataset.action;
    const tx = transactions.find((item) => item.id === id);
    if (action === "delete") {
      deleteTransaction(id);
    } else if (action === "edit" && tx) {
      openEditModal(tx);
    }
  });

  elements.filterTypeInputs.forEach((input) => {
    input.addEventListener("change", updateFiltersFromInputs);
  });
  elements.filterCategory.addEventListener("change", updateFiltersFromInputs);

  elements.saveCategoryBtn.addEventListener("click", addCategory);
  elements.addCategoryBtn.addEventListener("click", () => {
    elements.categoryInput.value = "";
    const modal = new bootstrap.Modal(document.getElementById("addCategoryModal"));
    modal.show();
  });

  elements.editSaveBtn.addEventListener("click", () => {
    const payload = {
      id: Number(elements.editSaveBtn.dataset.id),
      date: elements.editDate.value,
      type: elements.editType.value,
      category: elements.editCategory.value,
      memo: elements.editMemo.value.trim(),
      amount: Number(elements.editAmount.value),
    };
    updateTransaction(payload);
  });

  fetchCategories().then(fetchTransactions);
});
