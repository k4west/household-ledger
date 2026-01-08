# RPG Status Formula Reference

This document summarizes the formulas used to compute the RPG status metrics returned by the `/api/rpg` endpoint.

## Inputs

- **Transactions**: Normalized list of income/expense/saving/transfer entries.
- **Budget**: Monthly budget (month view) or annual budget totals (year view).
- **Categories**: Used to normalize categories and determine saving categories.

## Common Helpers

- **Clamp**: `clamp(value, min, max)` limits a number to the `[min, max]` range.
- **Budget totals**:
  - `budgetTotal`: sum of non-saving expense budgets.
  - `savingGoal`: sum of saving-category budgets.
- **Multipliers**:
  - `goalMultiplier = 1 + 0.10 * spendTight + 0.10 * saveTight`
  - `spendBonus = 1.1` if `expense <= budgetTotal` (and `budgetTotal > 0`), otherwise `1.0`
  - `saveBonus = 1 + 0.15 * clamp(saving / savingGoal, 0, 1)` (only if `savingGoal > 0`)
  - `totalMultiplier = min(goalMultiplier * spendBonus * saveBonus, 1.5)`

## Monthly RPG Summary

### Budget and Totals

- **Income/Expense/Saving** are summed from the current month’s transactions.
- **Budget totals** come from the selected month’s budget.

### Tightness Metrics

If `income > 0`:

- `spendTight = clamp((0.90 - (budgetTotal / income)) / 0.30, 0, 1)`
- `saveTight = clamp((savingGoal / income) / 0.30, 0, 1)`

Otherwise both are `0`.

### EXP/Party HP Model

Transactions are sorted by date (then id), skipping transfers.

- **Scale**: `scale = max(income / 100, 10000)`
- **Base log EXP**: `baseLogExp = log(1 + (amount / scale))`
- **Effective multiplier**: `totalMultiplier` when `partyHp > 0`, otherwise `1.0`

#### Transaction EXP

- **Income**: `+ 120 * baseLogExp * multiplier`
- **Saving**: `+ 150 * baseLogExp * multiplier`
- **Expense (under budget for category)**:
  - `+ 80 * baseLogExp * multiplier`
- **Expense (over budget for category)**:
  - `+ 260 * baseLogExp * multiplier`

#### Party HP Damage

- **Under budget**: if `budgetTotal > 0`,
  - `damage = (amount / budgetTotal) * 100 * 0.5`
- **Over budget**: if `budgetTotal > 0`,
  - `damage = (amount / budgetTotal) * 100 * 1.5`
  - plus `gremlinLevel * 0.5` after incrementing `gremlinLevel`

`partyHp` is floored at `0`.

### Levels and Counts

- `level = floor(totalExp / 100) + 1`
- `hunterCount`: count of income transactions
- `guardianCount`: count of saving transactions
- `clericCount`: count of under-budget expense transactions
- `gremlinCount`: count of over-budget expense transactions
- `gremlinLevel`: incremented for each over-budget expense

## Yearly RPG Summary

The yearly summary uses the same formulas with these differences:

- **Budget totals** are derived by summing every month’s budget values.
- **Category limits** are summed across all months for each category.
- **Scale**: `scale = max(income * 0.01, 1)`
- Over-budget checks compare category spend against the **annual** category limit.

All multipliers, EXP gains, party HP damage, and level calculation match the monthly formulas above.
