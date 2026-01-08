const API_BASE = "http://localhost:8888/api";
const TRANSACTIONS_ENDPOINT = `${API_BASE}/transactions`;
const CATEGORIES_ENDPOINT = `${API_BASE}/categories`;
const CATEGORY_ATTRIBUTES_ENDPOINT = `${API_BASE}/category-attributes`;
const RPG_ENDPOINT = `${API_BASE}/rpg`;

let expenseChart = null;
let summaryChart = null;
let transactions = [];
let categories = [];
let categoryAttributes = {};
let selectedYear = null;
let selectedMonth = null;
let activeTypeFilters = new Set(["income", "expense", "saving", "transfer"]);
let activeCategoryFilters = new Set();
let isYearView = false;
let memoTransaction = null;
let rpgStatusAnimation = {
  intervalId: null,
  key: null,
  frameIndex: 0,
};

const elements = {
  list: document.getElementById("transaction-list"),
  totalIncome: document.getElementById("total-income"),
  totalExpense: document.getElementById("total-expense"),
  totalSaving: document.getElementById("total-saving"),
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
  filterCategoryChips: document.getElementById("filter-category-chips"),
  filterCategoryAll: document.getElementById("filter-category-all"),
  filterTypeInputs: document.querySelectorAll("input[name='filter-type']"),
  addCategoryBtn: document.getElementById("add-category-btn"),
  categoryInput: document.getElementById("new-category-name"),
  categoryAttribute: document.getElementById("new-category-attribute"),
  saveCategoryBtn: document.getElementById("save-category-btn"),
  categoryList: document.getElementById("category-list"),
  rpgLevel: document.getElementById("rpg-level"),
  rpgExp: document.getElementById("rpg-exp"),
  rpgHp: document.getElementById("rpg-hp"),
  rpgHpBar: document.getElementById("rpg-hp-bar"),
  rpgMultiplier: document.getElementById("rpg-multiplier"),
  rpgRoles: document.getElementById("rpg-roles"),
  rpgStatusVisual: document.getElementById("rpg-status-visual"),
  editModal: document.getElementById("editTransactionModal"),
  editDate: document.getElementById("edit-date"),
  editType: document.getElementById("edit-type"),
  editCategory: document.getElementById("edit-category"),
  editMemo: document.getElementById("edit-memo"),
  editAmount: document.getElementById("edit-amount"),
  editSaveBtn: document.getElementById("save-edit-btn"),
  memoModal: document.getElementById("memoModal"),
  memoMeta: document.getElementById("memo-meta"),
  memoTextarea: document.getElementById("memo-textarea"),
  memoSaveBtn: document.getElementById("save-memo-btn"),
};

const RPG_STATUS_FRAMES = {
  gameOver: [
    String.raw`      _____________
     /  R.I.P.    /|
    / ___________/ |
    |  EMPTY     | |
    |  WALLET    | |
    |____________|/
        ⚰️  GAME OVER
     ───────────────`,
    String.raw`      _____________
     /  R.I.P.    /|
    / ___________/ |
    |  EMPTY     | |
    |  WALLET    | |
    |____________|/
        ☠️  GAME OVER
     ───────────────`,
    String.raw`      _____________
     /  R.I.P.    /|
    / ___________/ |
    |  EMPTY     | |
    |  WALLET    | |
    |____________|/
       🕯️  GAME OVER
     ───────────────`,
  ],
  bossFight: [
    String.raw` (⚠ WARNING ⚠)   ⚡⚡
    🐉🔥🔥🔥🔥🔥
  /|__||__||__| \
 /    BOSS RAID  \
 \_____/___\_____/`,
    String.raw` (⚠ WARNING ⚠)   ⚡⚡
    🐲🔥🔥🔥🔥🔥
  /|__||__||__| \
 /   BOSS STRIKE \
 \_____/___\_____/`,
    String.raw` (⚠ WARNING ⚠)   ⚡⚡
    🐉🔥🔥🔥🔥💥
  /|__||__||__| \
 /   BOSS STRIKE \
 \_____/___\_____/`,
    String.raw` (⚠ WARNING ⚠)   ⚡⚡
    🐲🔥🔥🔥💥🔥
  /|__||__||__| \
 /   BOSS STRIKE \
 \_____/___\_____/`,
    String.raw` (⚠ WARNING ⚠)   ⚔️💥
    🐉🔥🔥💥🔥🔥
  /|__||__||__| \
 /   BOSS ATTACK \
 \_____/___\_____/`,
    String.raw` (⚠ WARNING ⚠)   ⚔️💥
    🐲🔥💥🔥🔥🔥
  /|__||__||__| \
 /   BOSS ATTACK \
 \_____/___\_____/`,
  ],
  battle: [
    String.raw`   YOU ⚔ GREMLIN BRIGADE
    _O_      _^_ _^_
   /| |\    (oo)(oo)
    / \     /|__||__|\
  ==[  FIGHT ON!  ]==`,
    String.raw`   YOU ⚔ GREMLIN BRIGADE
     _O_     _^_ _^_
    /| |\   (><)(oo)
     / \    /|__||__|\
  ==[   CLASH!!   ]==`,
    String.raw`   YOU ⚔ GREMLIN BRIGADE
     _O_    _^_ _^_
    /| |\  (oo)(><)
     / \   /|__||__|\
  ==[  STRIKE!!  ]==`,
    String.raw`   YOU ⚔ GREMLIN BRIGADE
     _O_ 💥_^_ _^_
    /| |\  (xx)(oo)
     / \   /|__||__|\
  ==[  COLLIDE!  ]==`,
    String.raw`   YOU ⚔ GREMLIN BRIGADE
     _O_💥 _^_ _^_
    /| |\  (><)(xx)
     / \   /|__||__|\
  ==[  IMPACT!!  ]==`,
    String.raw`   YOU ⚔ GREMLIN BRIGADE
     _O_⚔️💥_^_ _^_
    /| |\   (xx)(xx)
     / \   /|__||__|\
  ==[  SMASH!!  ]==`,
  ],
  kingdom: [
    String.raw`        /\  /\  ⚑
       /__\/__\ 🏰
       |  __  |  💰
       | |__| |  💎
       |  👑  |  ✨
       |______|  ✨
    ==[  WEALTH  ]==`,
    String.raw`        /\  /\  ⚑
       /__\/__\ 🏰
       |  __  |  💰
       | |__| |  💎
       |  👑  |  ✨
       |______|  ✨
    ==[   GLORY  ]==`,
    String.raw`        /\  /\  ⚑
       /__\/__\ 🏰
       |  __  |  💰
       | |__| |  💎
       |  👑  |  ✨
       |______|  ✨
    ==[   HONOR  ]==`,
    String.raw`        /\  /\  ⚑
       /__\/__\ 🏰
       |  __  |  💰
       | |__| |  💎
       |  👑  | 🎆🎆
       |______| 🎆🎆
    ==[ FIREWORK ]==`,
    String.raw`        /\  /\  ⚑
       /__\/__\ 🏰
       |  __  |  💰
       | |__| |  💎
       |  👑  | 🎆✨
       |______| ✨🎆
    ==[  SPARKS  ]==`,
    String.raw`        /\  /\  ⚑
       /__\/__\ 🏰
       |  __  |  💰
       | |__| |  💎
       |  👑  | 🎇🎇
       |______| 🎇🎇
    ==[  FESTA   ]==`,
  ],
  danger: [
    String.raw`   (Help... storm!)
    O    🌩️🌩️
   /|\  |]
   / \  |]
 _/   \_|]
==[   DANGER   ]==`,
    String.raw`   (Hold on...!)
    O   🌩️🌩️
   /|\  |]
   / \  |]
 _/   \_|]
==[    ALERT   ]==`,
    String.raw`   (Stay sharp!)
    O  🌩️🌩️
   /|\  |]
   / \  |]
 _/   \_|]
==[   BRACE   ]==`,
    String.raw`   (⚡ ZAP!)
    O ⚡🌩️
   /|\  |]
   / \  |]
 _/   \_|]
==[  STRUCK  ]==`,
    String.raw`   (⚡ HIT!!)
    O⚡⚡
   /|\  |]
   / \ _|]
 _/   \_|]
==[  SHOCK!  ]==`,
    String.raw`   (⚡ RUMBLE!)
    ⚡   ⚡
   /|\  |]
   / \ _|]
 _/   \_|]
==[  SURVIVE ]==`,
  ],
  adventure: [
    String.raw`     ☁  ☀      🗺️
      O        ⛺
     /|\/
     / \     🌲
    /  /    🌲
  ==[  WALK  ]==`,
    String.raw`     ☁  ☀      🗺️
      O        ⛺
     /|\_
     / \     🌲
    /  /    🌲
  ==[  STEP  ]==`,
    String.raw`     ☁  ☀      🗺️
      O        ⛺
     /|\/
     / \    🌲🌲
    /  /    🌲
  ==[  TREK  ]==`,
    String.raw`     ☁  ☀      🗺️
      O \      ⛺
     /|\/
     / \   🌲🌲
    /  /    🌲
  ==[  RUN!  ]==`,
    String.raw`     ☁  ☀      🗺️
       O       ⛺
      /|\/
      / \   🌲🌲
     /  /   🌲
   ==[  DASH  ]==`,
    String.raw`     ☁  ☀      🗺️
       O       ⛺
      /|\_
      / \   🌲
     /  /  🌲🌲
   ==[  SPRINT ]==`,
  ],
};

function getRpgScenario({ hp, gremlinCount, level }) {
  if (hp <= 0) {
    return { key: "game-over", frames: RPG_STATUS_FRAMES.gameOver };
  }
  if (hp > 0 && gremlinCount >= 5) {
    return { key: "boss-fight", frames: RPG_STATUS_FRAMES.bossFight };
  }
  if (hp > 0 && gremlinCount > 3) {
    return { key: "battle", frames: RPG_STATUS_FRAMES.battle };
  }
  if (level >= 10 && gremlinCount === 0) {
    return { key: "kingdom", frames: RPG_STATUS_FRAMES.kingdom };
  }
  if (hp < 30 && gremlinCount === 0) {
    return { key: "danger", frames: RPG_STATUS_FRAMES.danger };
  }
  return { key: "adventure", frames: RPG_STATUS_FRAMES.adventure };
}

function startRpgStatusAnimation(scenario) {
  if (!elements.rpgStatusVisual) return;
  const { key, frames } = scenario;
  const safeFrames = frames && frames.length ? frames : RPG_STATUS_FRAMES.adventure;
  if (rpgStatusAnimation.intervalId && rpgStatusAnimation.key !== key) {
    clearInterval(rpgStatusAnimation.intervalId);
    rpgStatusAnimation.intervalId = null;
    rpgStatusAnimation.frameIndex = 0;
  }
  if (!rpgStatusAnimation.intervalId) {
    rpgStatusAnimation.key = key;
    rpgStatusAnimation.frameIndex = 0;
    elements.rpgStatusVisual.textContent = safeFrames[0];
    rpgStatusAnimation.intervalId = setInterval(() => {
      rpgStatusAnimation.frameIndex =
        (rpgStatusAnimation.frameIndex + 1) % safeFrames.length;
      elements.rpgStatusVisual.textContent =
        safeFrames[rpgStatusAnimation.frameIndex];
    }, 700);
  }
}

function formatAmount(value) {
  return Number(value).toLocaleString();
}

function formatType(type) {
  if (type === "income") return "수입";
  if (type === "saving") return "저축";
  if (type === "transfer") return "이체";
  return "지출";
}

function normalizeCategory(category) {
  if (categories.includes(category)) {
    return category;
  }
  return "기타";
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
      } else if (tx.type === "saving") {
        acc.saving += Number(tx.amount || 0);
      } else if (tx.type === "transfer") {
        acc.transfer += Number(tx.amount || 0);
      }
      return acc;
    },
    { income: 0, expense: 0, saving: 0, transfer: 0 }
  );
}

function renderSummary(totals) {
  elements.totalIncome.textContent = formatAmount(totals.income);
  elements.totalExpense.textContent = formatAmount(totals.expense);
  elements.totalSaving.textContent = formatAmount(totals.saving);
  elements.remainingBudget.textContent = formatAmount(
    totals.income - totals.expense - totals.saving
  );
}

function renderTable(list) {
  elements.list.innerHTML = "";

  list
    .sort((a, b) => b.id - a.id)
    .forEach((tx) => {
      const category = normalizeCategory(tx.category);
      const row = document.createElement("tr");
      let amountClass = "text-expense";
      if (tx.type === "income") amountClass = "text-income";
      if (tx.type === "saving") amountClass = "text-saving";
      if (tx.type === "transfer") amountClass = "text-transfer";
      const memoValue = (tx.memo || "").trim();
      const memoText = memoValue || "메모 추가";
      const memoClass = memoValue ? "" : "text-muted";

      row.innerHTML = `
        <td>${tx.date || "-"}</td>
        <td>${category || "-"}</td>
        <td class="memo-cell">
          <span class="badge bg-light text-dark badge-type me-2">${formatType(tx.type)}</span>
          <button class="btn btn-link p-0 memo-preview ${memoClass}" type="button" data-action="memo" data-id="${tx.id}">
            ${memoText}
          </button>
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
      const key = normalizeCategory(tx.category);
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
  const data = [totals.income, totals.expense, totals.saving];

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
    if (activeCategoryFilters.size === 0) return false;
    const category = normalizeCategory(tx.category);
    if (!activeCategoryFilters.has(category)) return false;
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

function buildCategoryChips(list) {
  elements.filterCategoryChips.innerHTML = "";
  list.forEach((category) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "category-chip";
    button.textContent = category;
    if (activeCategoryFilters.has(category)) {
      button.classList.add("active");
    }
    button.addEventListener("click", () => {
      if (activeCategoryFilters.has(category)) {
        activeCategoryFilters.delete(category);
      } else {
        activeCategoryFilters.add(category);
      }
      button.classList.toggle("active");
      renderDashboard();
    });
    elements.filterCategoryChips.appendChild(button);
  });
}

function selectAllCategories() {
  activeCategoryFilters = new Set(categories);
  buildCategoryChips(categories);
  renderDashboard();
}

function buildCategoryList(list) {
  elements.categoryList.innerHTML = "";
  list.forEach((category) => {
    const attribute = categoryAttributes[category] || "consumption";
    const row = document.createElement("div");
    row.className = "d-flex justify-content-between align-items-center py-1 gap-2";
    row.innerHTML = `
      <span>${category}</span>
      <div class="d-flex align-items-center gap-2">
        <select class="form-select form-select-sm" data-action="update-attribute" data-category="${category}">
          <option value="consumption">소비형</option>
          <option value="saving">저축형</option>
        </select>
        <button type="button" class="btn btn-sm btn-outline-danger" data-action="remove-category" data-category="${category}">
          삭제
        </button>
      </div>
    `;
    const attributeSelect = row.querySelector("select");
    attributeSelect.value = attribute;
    if (category === "기타") {
      row.querySelector("button").disabled = true;
      attributeSelect.disabled = true;
    }
    elements.categoryList.appendChild(row);
  });
}

async function fetchCategories() {
  try {
    const response = await fetch(CATEGORIES_ENDPOINT);
    categories = await response.json();
    await fetchCategoryAttributes();
    buildCategoryOptions(elements.category, categories);
    buildCategoryOptions(elements.editCategory, categories);
    const previousSelections = new Set(activeCategoryFilters);
    activeCategoryFilters = new Set(
      categories.filter(
        (category) => previousSelections.size === 0 || previousSelections.has(category)
      )
    );
    if (activeCategoryFilters.size === 0) {
      activeCategoryFilters = new Set(categories);
    }
    buildCategoryChips(categories);
    buildCategoryList(categories);
    if (elements.filterCategoryAll) {
      elements.filterCategoryAll.disabled = categories.length === 0;
    }
  } catch (error) {
    console.error("Failed to fetch categories", error);
  }
}

async function fetchCategoryAttributes() {
  try {
    const response = await fetch(CATEGORY_ATTRIBUTES_ENDPOINT);
    categoryAttributes = await response.json();
  } catch (error) {
    console.error("Failed to fetch category attributes", error);
    categoryAttributes = {};
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
    fetchRpgStatus();
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
  elements.editCategory.value = normalizeCategory(tx.category);
  elements.editMemo.value = tx.memo || "";
  elements.editAmount.value = tx.amount;
  elements.editSaveBtn.dataset.id = tx.id;
  const modal = new bootstrap.Modal(elements.editModal);
  modal.show();
}

function openMemoModal(tx) {
  memoTransaction = tx;
  const category = normalizeCategory(tx.category);
  elements.memoTextarea.value = tx.memo || "";
  elements.memoMeta.innerHTML = "";
  const metaItems = [
    `날짜: ${tx.date || "-"}`,
    `카테고리: ${category || "-"}`,
    `금액: ${formatAmount(tx.amount)}`,
  ];
  metaItems.forEach((item) => {
    const span = document.createElement("span");
    span.textContent = item;
    elements.memoMeta.appendChild(span);
  });
  const modal = new bootstrap.Modal(elements.memoModal);
  modal.show();
}

async function addCategory() {
  const name = elements.categoryInput.value.trim();
  if (!name) return;
  try {
    await fetch(CATEGORIES_ENDPOINT, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        name,
        attribute: elements.categoryAttribute.value,
      }),
    });
    elements.categoryInput.value = "";
    elements.categoryAttribute.value = "consumption";
    await fetchCategories();
  } catch (error) {
    console.error("Failed to add category", error);
  }
}

async function deleteCategory(name) {
  try {
    await fetch(`${CATEGORIES_ENDPOINT}?name=${encodeURIComponent(name)}`, {
      method: "DELETE",
    });
    await fetchCategories();
    fetchTransactions();
  } catch (error) {
    console.error("Failed to delete category", error);
  }
}

function updateTypeFilters() {
  activeTypeFilters = new Set();
  elements.filterTypeInputs.forEach((input) => {
    if (input.checked) {
      activeTypeFilters.add(input.value);
    }
  });
  renderDashboard();
}

async function updateCategoryAttribute(name, attribute) {
  try {
    await fetch(CATEGORY_ATTRIBUTES_ENDPOINT, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ name, attribute }),
    });
    categoryAttributes[name] = attribute;
  } catch (error) {
    console.error("Failed to update category attribute", error);
  }
}

async function fetchRpgStatus() {
  try {
    let url = `${RPG_ENDPOINT}?year=${selectedYear}`;
    if (!isYearView) {
      const monthParam = String(selectedMonth).padStart(2, "0");
      url += `&month=${monthParam}`;
    }
    const response = await fetch(url);
    const payload = await response.json();
    renderRpgStatus(payload);
  } catch (error) {
    console.error("Failed to fetch RPG status", error);
  }
}

function renderRpgStatus(payload) {
  if (!payload || typeof payload !== "object") return;
  const level = payload.level ?? 1;
  const totalExp = payload.total_exp ?? 0;
  const hp = payload.party_hp ?? 100;
  const multiplier = payload.total_multiplier ?? 1;
  const roles = payload.roles || {};
  const gremlinCount = Number(roles.gremlin || 0);
  const scenario = getRpgScenario({ hp, gremlinCount, level });

  elements.rpgLevel.textContent = `Lv. ${level}`;
  elements.rpgExp.textContent = totalExp.toFixed(2);
  elements.rpgHp.textContent = `${hp.toFixed(1)} / 100`;
  elements.rpgHpBar.style.width = `${Math.min(hp, 100)}%`;
  elements.rpgMultiplier.textContent = `${multiplier.toFixed(2)}x`;
  if (elements.rpgStatusVisual) {
    elements.rpgStatusVisual.className = `rpg-status-visual status-${scenario.key}`;
    startRpgStatusAnimation(scenario);
  }

  elements.rpgRoles.innerHTML = "";
  const roleItems = [
    ["HUNTER", roles.hunter || 0],
    ["GUARDIAN", roles.guardian || 0],
    ["CLERIC", roles.cleric || 0],
    ["GREMLIN", roles.gremlin || 0],
  ];
  roleItems.forEach(([role, count]) => {
    const span = document.createElement("span");
    span.className = "badge text-bg-light";
    span.textContent = `${role}: ${count}`;
    elements.rpgRoles.appendChild(span);
  });
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
    } else if (action === "memo" && tx) {
      openMemoModal(tx);
    }
  });

  elements.filterTypeInputs.forEach((input) => {
    input.addEventListener("change", updateTypeFilters);
  });
  if (elements.filterCategoryAll) {
    elements.filterCategoryAll.addEventListener("click", selectAllCategories);
  }

  elements.saveCategoryBtn.addEventListener("click", addCategory);
  elements.addCategoryBtn.addEventListener("click", () => {
    elements.categoryInput.value = "";
    elements.categoryAttribute.value = "consumption";
    const modal = new bootstrap.Modal(document.getElementById("addCategoryModal"));
    modal.show();
  });
  elements.categoryList.addEventListener("click", (event) => {
    const target = event.target.closest("button");
    if (!target) return;
    if (target.dataset.action !== "remove-category") return;
    const name = target.dataset.category;
    if (!name) return;
    deleteCategory(name);
  });
  elements.categoryList.addEventListener("change", (event) => {
    const target = event.target;
    if (!target || target.dataset.action !== "update-attribute") return;
    const name = target.dataset.category;
    const attribute = target.value;
    if (!name) return;
    updateCategoryAttribute(name, attribute);
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

  elements.memoSaveBtn.addEventListener("click", () => {
    if (!memoTransaction) return;
    const payload = {
      id: Number(memoTransaction.id),
      date: memoTransaction.date,
      type: memoTransaction.type,
      category: normalizeCategory(memoTransaction.category),
      memo: elements.memoTextarea.value.trim(),
      amount: Number(memoTransaction.amount),
    };
    updateTransaction(payload);
    const modal = bootstrap.Modal.getInstance(elements.memoModal);
    if (modal) {
      modal.hide();
    }
  });

  fetchCategories().then(fetchTransactions);
});
