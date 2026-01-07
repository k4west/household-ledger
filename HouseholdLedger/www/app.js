const API_BASE = "http://localhost:8888/api/transactions";

let expenseChart = null;

const elements = {
  list: document.getElementById("transaction-list"),
  totalIncome: document.getElementById("total-income"),
  totalExpense: document.getElementById("total-expense"),
  currentBalance: document.getElementById("current-balance"),
  date: document.getElementById("tx-date"),
  type: document.getElementById("tx-type"),
  category: document.getElementById("tx-category"),
  memo: document.getElementById("tx-memo"),
  amount: document.getElementById("tx-amount"),
};

function formatAmount(value) {
  return Number(value).toLocaleString();
}

function formatType(type) {
  return type === "income" ? "Income" : "Expense";
}

function calculateTotals(transactions) {
  return transactions.reduce(
    (acc, tx) => {
      if (tx.type === "income") {
        acc.income += Number(tx.amount || 0);
      } else if (tx.type === "expense") {
        acc.expense += Number(tx.amount || 0);
      }
      return acc;
    },
    { income: 0, expense: 0 }
  );
}

function renderSummary(totals) {
  elements.totalIncome.textContent = formatAmount(totals.income);
  elements.totalExpense.textContent = formatAmount(totals.expense);
  elements.currentBalance.textContent = formatAmount(totals.income - totals.expense);
}

function renderTable(transactions) {
  elements.list.innerHTML = "";

  transactions
    .sort((a, b) => b.id - a.id)
    .forEach((tx) => {
      const row = document.createElement("tr");
      const amountClass = tx.type === "income" ? "text-income" : "text-expense";

      row.innerHTML = `
        <td>${tx.date || "-"}</td>
        <td>${tx.category || "-"}</td>
        <td>
          <span class="badge bg-light text-dark badge-type me-2">${formatType(tx.type)}</span>
          ${tx.memo || "-"}
        </td>
        <td class="text-end ${amountClass}">${formatAmount(tx.amount)}</td>
        <td class="text-end">
          <button class="btn btn-sm btn-outline-danger" onclick="deleteTransaction(${tx.id})">
            <i class="fa-solid fa-trash"></i>
          </button>
        </td>
      `;

      elements.list.appendChild(row);
    });
}

function buildChart(transactions) {
  const expenseTotals = transactions
    .filter((tx) => tx.type === "expense")
    .reduce((acc, tx) => {
      const key = tx.category || "Other";
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
      placeholder.textContent = "No expense data yet.";
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

async function fetchTransactions() {
  try {
    const response = await fetch(API_BASE);
    const transactions = await response.json();

    renderTable(transactions);
    const totals = calculateTotals(transactions);
    renderSummary(totals);
    buildChart(transactions);
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
    await fetch(API_BASE, {
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

async function deleteTransaction(id) {
  try {
    await fetch(`${API_BASE}?id=${id}`, {
      method: "DELETE",
    });
    fetchTransactions();
  } catch (error) {
    console.error("Failed to delete transaction", error);
  }
}

document.addEventListener("DOMContentLoaded", () => {
  const today = new Date().toISOString().split("T")[0];
  elements.date.value = today;
  fetchTransactions();
});
