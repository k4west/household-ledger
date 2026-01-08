import json
import random
import os
import calendar

# 1. 설정: 저장할 폴더 및 카테고리
output_dir = "financial_data_2026"
os.makedirs(output_dir, exist_ok=True)  # 폴더가 없으면 생성

categories = ["식비", "교통", "쇼핑", "기타"]
start_id = 2026000001  # ID 시작 번호

# 2. 랜덤 데이터 생성 함수
def generate_random_amount(min_val=5000, max_val=200000, step=1000):
    """최소~최대 금액 사이에서 step 단위로 랜덤 금액 생성"""
    return random.randint(min_val // step, max_val // step) * step

# 3. 1월부터 12월까지 파일 생성 루프
for month in range(1, 13):
    monthly_data = []
    
    # 해당 월의 마지막 날짜 구하기 (예: 1월은 31일, 2월은 28일)
    _, last_day = calendar.monthrange(2026, month)
    
    # ---------------------------
    # (1) 수입 (월급) - 매월 5일 고정
    # ---------------------------
    income_entry = {
        "id": start_id,
        "date": f"2025-{month:02d}-05",
        "type": "income",
        "category": "월급",
        "memo": f"2025-{month:02d} 월급",
        "amount": 3000000
    }
    monthly_data.append(income_entry)
    start_id += 1

    # ---------------------------
    # (2) 지출 (Expense) - 매월 10~20건 랜덤 생성
    # ---------------------------
    num_expenses = random.randint(2, 10)
    
    for _ in range(num_expenses):
        day = random.randint(1, last_day)
        category = random.choice(categories)
        amount = generate_random_amount()
        
        expense_entry = {
            "id": start_id,
            "date": f"2025-{month:02d}-{day:02d}",
            "type": "expense",
            "category": category,
            "memo": f"2025-{month:02d} {category} 지출",
            "amount": amount
        }
        monthly_data.append(expense_entry)
        start_id += 1

    # ---------------------------
    # (3) 저축 (Saving) - 매월 25일 고정
    # ---------------------------
    if 25 <= last_day: # 25일이 존재하는 경우만 (보통 다 존재함)
        saving_entry = {
            "id": start_id,
            "date": f"2026-{month:02d}-25",
            "type": "saving",
            "category": "기타",
            "memo": f"2026-{month:02d} 청년 적금",
            "amount": 700000
        }
        monthly_data.append(saving_entry)
        start_id += 1

    # ---------------------------
    # (4) 날짜순 정렬 및 파일 저장
    # ---------------------------
    # 날짜 기준으로 데이터 정렬
    monthly_data.sort(key=lambda x: x['date'])

    # 파일명 예: ledger_2026-01.json
    filename = f"ledger_2026-{month:02d}.json"
    filepath = os.path.join(output_dir, filename)
    
    with open(filepath, 'w', encoding='utf-8') as f:
        # ensure_ascii=False를 해야 한글이 깨지지 않고 저장됩니다.
        json.dump(monthly_data, f, indent=2, ensure_ascii=False)

    print(f"파일 생성 완료: {filepath} (항목 수: {len(monthly_data)}개)")

print("\n모든 작업이 완료되었습니다.")