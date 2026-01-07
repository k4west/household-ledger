const API_BASE = "http://localhost:8888/api";
const SCHEDULES_ENDPOINT = `${API_BASE}/schedules`;
const CATEGORIES_ENDPOINT = `${API_BASE}/categories`;

let schedules = [];
let categories = [];

const elements = {
  form: document.getElementById("schedule-form"),
  name: document.getElementById("schedule-name"),
  type: document.getElementById("schedule-type"),
  amount: document.getElementById("schedule-amount"),
  category: document.getElementById("schedule-category"),
  day: document.getElementById("schedule-day"),
  start: document.getElementById("schedule-start"),
  end: document.getElementById("schedule-end"),
  list: document.getElementById("schedule-list"),
};

function formatAmount(value) {
  return Number(value || 0).toLocaleString();
}

function formatType(type) {
  if (type === "INCOME") return "수입";
  if (type === "TRANSFER") return "저축";
  return "지출";
}

function formatFrequency(item) {
  if (item.frequency === "MONTHLY_DATE") {
    return `매월 ${item.day}일`;
  }
  return item.frequency || "-";
}

function buildCategoryOptions() {
  elements.category.innerHTML = "";
  categories.forEach((category) => {
    const option = document.createElement("option");
    option.value = category;
    option.textContent = category;
    elements.category.appendChild(option);
  });
}

async function fetchCategories() {
  try {
    const response = await fetch(CATEGORIES_ENDPOINT);
    categories = await response.json();
    buildCategoryOptions();
  } catch (error) {
    console.error("Failed to fetch categories", error);
  }
}

async function fetchSchedules() {
  try {
    const response = await fetch(SCHEDULES_ENDPOINT);
    schedules = await response.json();
    renderSchedules();
  } catch (error) {
    console.error("Failed to fetch schedules", error);
  }
}

function renderSchedules() {
  elements.list.innerHTML = "";
  schedules
    .slice()
    .sort((a, b) => b.id - a.id)
    .forEach((item) => {
      const row = document.createElement("tr");
      const period = `${item.startDate || "-"} ~ ${item.endDate || "진행 중"}`;
      row.innerHTML = `
        <td>${item.name}</td>
        <td>${formatType(item.type)}</td>
        <td>${formatAmount(item.amount)}</td>
        <td>${item.category}</td>
        <td>${formatFrequency(item)}</td>
        <td>${period}</td>
        <td class="text-end">
          <button class="btn btn-sm btn-outline-secondary me-2" data-action="duplicate" data-id="${item.id}">
            복제
          </button>
          <button class="btn btn-sm btn-outline-danger" data-action="end" data-id="${item.id}">
            종료
          </button>
        </td>
      `;
      elements.list.appendChild(row);
    });
}

function todayString() {
  const now = new Date();
  return now.toISOString().split("T")[0];
}

function yesterdayString() {
  const now = new Date();
  now.setDate(now.getDate() - 1);
  return now.toISOString().split("T")[0];
}

async function createSchedule(payload) {
  await fetch(SCHEDULES_ENDPOINT, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  await fetchSchedules();
}

async function updateSchedule(payload) {
  await fetch(SCHEDULES_ENDPOINT, {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  await fetchSchedules();
}

async function handleSubmit() {
  const payload = {
    name: elements.name.value.trim(),
    type: elements.type.value,
    amount: Number(elements.amount.value || 0),
    category: elements.category.value,
    frequency: "MONTHLY_DATE",
    day: Number(elements.day.value || 1),
    startDate: elements.start.value,
    endDate: elements.end.value,
    lastGenerated: "",
  };

  if (!payload.name || !payload.amount || !payload.category || !payload.startDate) {
    return;
  }

  try {
    await createSchedule(payload);
    elements.form.reset();
    elements.start.value = todayString();
  } catch (error) {
    console.error("Failed to add schedule", error);
  }
}

async function handleEndSchedule(id) {
  const schedule = schedules.find((item) => item.id === id);
  if (!schedule) return;
  const payload = { ...schedule, endDate: yesterdayString() };
  await updateSchedule(payload);
}

async function handleDuplicateSchedule(id) {
  const schedule = schedules.find((item) => item.id === id);
  if (!schedule) return;
  const payload = {
    ...schedule,
    id: 0,
    startDate: todayString(),
    endDate: "",
    lastGenerated: "",
  };
  await createSchedule(payload);
}

document.addEventListener("DOMContentLoaded", async () => {
  elements.start.value = todayString();
  await fetchCategories();
  await fetchSchedules();

  elements.form.addEventListener("submit", (event) => {
    event.preventDefault();
    handleSubmit();
  });

  elements.list.addEventListener("click", (event) => {
    const target = event.target.closest("button");
    if (!target) return;
    const id = Number(target.dataset.id);
    const action = target.dataset.action;
    if (action === "end") {
      handleEndSchedule(id);
    } else if (action === "duplicate") {
      handleDuplicateSchedule(id);
    }
  });
});
