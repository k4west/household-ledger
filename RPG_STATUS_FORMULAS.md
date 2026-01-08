# RPG 상태 계산 공식 레퍼런스

본 문서는 `/api/rpg` 엔드포인트에서 반환되는 RPG 상태 지표들을 계산하기 위해 사용되는 공식들을 정리한 문서이다.

---

## 입력값 (Inputs)

- **Transactions**  
  수입 / 지출 / 저축 / 이체로 정규화된 거래 목록

- **Budget**  
  월별 예산(월 단위 조회 시) 또는 연간 예산 합계(연 단위 조회 시)

- **Categories**  
  카테고리 정규화 및 저축 카테고리 판별에 사용

---

## 공통 헬퍼 함수 (Common Helpers)

- **Clamp**
```

clamp(value, min, max)

```
→ 값을 `[min, max]` 범위로 제한한다.

---

### 예산 관련 합계

- `budgetTotal`  
→ 저축을 제외한 모든 지출 예산의 합

- `savingGoal`  
→ 저축 카테고리에 해당하는 예산의 합

---

### 배율(Multiplier) 정의

- `goalMultiplier`
```

goalMultiplier = 1 + 0.10 * spendTight + 0.10 * saveTight

```

- `spendBonus`
```

spendBonus = 1.1   (expense <= budgetTotal 이고 budgetTotal > 0 인 경우)
1.0   (그 외)

```

- `saveBonus`
```

saveBonus = 1 + 0.15 * clamp(saving / savingGoal, 0, 1)
(savingGoal > 0 인 경우에만 적용)

```

- `totalMultiplier`
```

totalMultiplier = min(goalMultiplier * spendBonus * saveBonus, 1.5)

```

---

## 월간 RPG 요약 (Monthly RPG Summary)

### 수입 / 지출 / 저축 합계

- **Income / Expense / Saving**  
→ 해당 월의 거래(Transaction)로부터 합산

- **Budget totals**  
→ 선택된 월의 예산 설정값을 사용

---

### 타이트함 지표 (Tightness Metrics)

`income > 0` 인 경우:

- **지출 타이트함**
```

spendTight = clamp((0.90 - (budgetTotal / income)) / 0.30, 0, 1)

```

- **저축 타이트함**
```

saveTight = clamp((savingGoal / income) / 0.30, 0, 1)

```

`income <= 0` 인 경우:

- `spendTight = 0`
- `saveTight = 0`

---

## EXP / Party HP 모델

거래는 **날짜순 → id순**으로 정렬한 뒤 처리하며,  
**TRANSFER(이체)** 거래는 EXP/HP 계산에서 제외한다.

---

### 금액 스케일 (Scale)

```

scale = max(income / 100, 10000)

```

---

### 기본 로그 EXP 값

```

baseLogExp = log(1 + (amount / scale))

```

---

### 실제 적용 배율

- `partyHp > 0` 인 경우:
```

multiplier = totalMultiplier

```

- `partyHp <= 0` 인 경우:
```

multiplier = 1.0

```

(Party HP가 0 이하일 경우, 모든 Benefit 배율이 무효화된다)

---

## 거래 유형별 EXP 획득

- **수입 (Income)**
```

* 120 * baseLogExp * multiplier

```

- **저축 (Saving)**
```

* 150 * baseLogExp * multiplier

```

- **지출 (카테고리 예산 이내)**
```

* 80 * baseLogExp * multiplier

```

- **지출 (카테고리 예산 초과)**
```

* 260 * baseLogExp * multiplier

```

---

## Party HP 감소 규칙

### 예산 이내 지출

`budgetTotal > 0` 인 경우:

```

damage = (amount / budgetTotal) * 100 * 0.5

```

---

### 예산 초과 지출

`budgetTotal > 0` 인 경우:

```

damage = (amount / budgetTotal) * 100 * 1.5

```

추가로,

```

gremlinLevel 증가 이후
추가 피해 = gremlinLevel * 0.5

```

---

### Party HP 제한

- `partyHp`는 최소 `0`으로 제한된다.
- 0 이하로 내려가지 않는다.

---

## 레벨 및 카운트 계산

- **레벨**
```

level = floor(totalExp / 100) + 1

```

- **거래 카운트**
- `hunterCount`  : 수입 거래 수
- `guardianCount`: 저축 거래 수
- `clericCount`  : 예산 이내 지출 거래 수
- `gremlinCount` : 예산 초과 지출 거래 수

- **Gremlin 레벨**
- 예산 초과 지출 거래 1건당 `+1`

---

## 연간 RPG 요약 (Yearly RPG Summary)

연간 요약은 **월간 공식과 동일한 계산 로직**을 사용하되, 다음 차이점이 있다.

---

### 연간 예산 계산

- **Budget totals**
- 모든 월의 예산 값을 합산하여 사용

- **카테고리 제한**
- 각 카테고리별 월 예산을 연간 합계로 변환

---

### 연간 Scale 계산

```

scale = max(income * 0.01, 1)

```

---

### 예산 초과 판정 기준

- 월 단위가 아닌 **연간 카테고리 한도** 기준으로 판정

---

### 기타 규칙

- EXP 획득
- Party HP 감소
- 배율(Multiplier)
- 레벨 계산

모두 **월간 공식과 동일**하게 적용된다.
