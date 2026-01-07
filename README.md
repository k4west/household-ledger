# Household Ledger (RPG Gamified)

**Household Ledger**는 C++로 작성된 웹 기반 가계부 애플리케이션입니다.

일반적인 수입/지출 관리 기능에 **RPG 게임 요소**를 접목하여, 건전한 소비 습관을 유지할수록 레벨이 오르고 파티가 성장하는 동기 부여 시스템을 갖추고 있습니다.

## 🌟 주요 기능 (Key Features)

### 1. ⚔️ RPG 게이미피케이션 (RPG Gamification)

소비 패턴을 분석하여 사용자의 재정 상태를 RPG 파티의 모험으로 시각화합니다.

* **직업 시스템**:
* **Hunter (사냥꾼)**: 수입(Income)이 발생하면 증가합니다.
* **Guardian (수호자)**: 저축(Saving)을 하면 증가합니다.
* **Cleric (성직자)**: 예산 내에서 합리적인 지출(Expense)을 하면 증가하며 파티를 유지합니다.
* **Gremlin (그렘린)**: 예산을 초과하거나 불필요한 지출을 하면 등장하여 파티의 HP에 피해를 입힙니다.


* **레벨 및 경험치**: 건전한 재정 활동을 통해 경험치(EXP)를 획득하고 레벨을 올릴 수 있습니다.
* **파티 HP**: 과소비(Gremlin 발생) 시 HP가 감소하며, 예산 관리에 실패하면 파티가 위험해집니다.

### 2. 💰 가계부 관리

* **거래 내역 관리**: 날짜, 유형(수입/지출/저축/이체), 카테고리, 메모, 금액을 기록합니다.
* **예산 설정**: 연간/월별 예산을 카테고리별로 설정할 수 있습니다.
* **카테고리 커스터마이징**: 사용자 정의 카테고리를 추가/삭제하고 속성(소비성/저축성 등)을 지정할 수 있습니다.

### 3. 📅 일정 및 반복 거래 (Schedules)

* 정기적인 수입이나 고정 지출을 스케줄로 등록하여 관리할 수 있습니다.
* 설정된 일정에 따라 자동으로 거래 내역 생성을 유도하거나 처리합니다.

### 4. 🌐 인터페이스

* **웹 대시보드**: 내장된 웹 서버(`http://localhost:8888`)를 통해 브라우저에서 편리하게 통계를 확인하고 입력할 수 있습니다.
* **콘솔 모드**: 서버 실행 중에도 터미널을 통해 기본적인 거래 내역 조회 및 추가가 가능합니다.

---

## 🛠️ 기술 스택 (Tech Stack)

* **Language**: C++ (C++17 이상 권장)
* **Web Server**: [cpp-httplib](https://github.com/yhirose/cpp-httplib) (내장 HTTP 서버)
* **JSON Parsing**: [nlohmann/json](https://github.com/nlohmann/json)
* **Frontend**: HTML, CSS, JavaScript (Vanilla JS)
* **IDE**: Visual Studio (솔루션 파일 `.sln` 포함)

---

## 🚀 빌드 및 실행 방법 (Build & Run)

이 프로젝트는 Visual Studio 환경에 최적화되어 있습니다.

1. **프로젝트 열기**: `HouseholdLedger/HouseholdLedger.sln` 파일을 Visual Studio로 엽니다.
2. **빌드**: `Release` 또는 `Debug` 모드로 솔루션을 빌드합니다.
* *의존성 라이브러리(`httplib.h`, `json.hpp`)는 프로젝트 내에 포함되어 있거나 NuGet 등을 통해 확인해야 할 수 있습니다.*


3. **실행**: 빌드된 애플리케이션을 실행합니다.
4. **접속**:
* 콘솔 창에 `Server listening on http://localhost:8888` 메시지가 뜨면 성공입니다.
* 웹 브라우저를 열고 `http://localhost:8888`에 접속하여 가계부를 사용합니다.



---

## 📡 API 명세 (API Reference)

애플리케이션은 RESTful API를 제공하여 프론트엔드와 통신합니다.

| Method | Endpoint | 설명 |
| --- | --- | --- |
| **GET** | `/api/transactions` | 거래 내역 조회 (파라미터: `year`, `month`) |
| **POST** | `/api/transactions` | 새로운 거래 내역 추가 |
| **PUT** | `/api/transactions` | 기존 거래 내역 수정 |
| **DELETE** | `/api/transactions` | 거래 내역 삭제 (파라미터: `id`) |
| **GET** | `/api/rpg` | **RPG 상태 요약 조회** (레벨, 경험치, 직업 카운트 등) |
| **GET** | `/api/budget` | 예산 내역 조회 |
| **POST** | `/api/budget` | 예산 설정 및 수정 |
| **GET** | `/api/categories` | 카테고리 목록 조회 |
| **POST** | `/api/categories` | 카테고리 추가 |
| **DELETE** | `/api/categories` | 카테고리 삭제 |
| **GET** | `/api/schedules` | 예약/반복 거래 조회 |

---

## 📂 디렉토리 구조 (Directory Structure)

```
k4west/household-ledger/household-ledger-dev2/
├── HouseholdLedger/
│   ├── main.cpp            # 엔트리 포인트, 서버 설정, API 라우팅
│   ├── AccountManager.* # 거래 내역 데이터 관리
│   ├── BudgetManager.* # 예산 데이터 관리
│   ├── CategoryManager.* # 카테고리 관리
│   ├── ScheduleManager.* # 일정 관리
│   ├── httplib.h           # HTTP 서버 라이브러리
│   ├── json.hpp            # JSON 파서 라이브러리
│   ├── data/               # 데이터 저장소 (JSON 파일)
│   └── www/                # 웹 프론트엔드 리소스 (HTML/JS/CSS)
└── README.md

```

---

## 📝 라이선스 (License)

이 프로젝트는 개인적인 용도로 개발되었으며, 해당 코드를 기반으로 자유롭게 수정하여 사용할 수 있습니다.

---


---

## cpp 학습 내용

### 1. C++ 기초 문법 및 타입 (Chap 01, 02, 16)

기초적인 문법 요소들은 프로젝트 전반에 걸쳐 효율성과 안전성을 위해 사용되었습니다.

* **참조자 (Reference, Chap 02)**:
* 함수 매개변수로 큰 객체(`std::string`, `std::vector` 등)를 전달할 때 복사 비용을 줄이기 위해 `const &`를 적극적으로 사용했습니다.
* **사용 예시:** `AccountManager.h`의 `bool parseYearMonth(const std::string& date, int& year, int& month) const;`


* **디폴트 매개변수 (Default Value, Chap 01)**:
* 함수 호출 시 인자를 생략할 수 있도록 기본값을 설정하여 편의성을 높였습니다.
* **사용 예시:** `CategoryManager.h`에서 카테고리 추가 시 속성을 생략하면 기본값인 `Consumption`(소비)으로 설정됩니다.
* 코드: `bool addCategory(const std::string& category, CategoryAttribute attribute = CategoryAttribute::Consumption);`


* **형변환 (Casting, Chap 16)**:
* RPG 요소를 계산하는 로직에서 정밀한 계산을 위해 `double`로 변환하거나, 결과를 다시 정수형 레벨로 변환할 때 `static_cast`를 사용했습니다.
* **사용 예시:** `main.cpp`의 `computeRpgSummary` 함수.
* 코드: `summary.level = static_cast<int>(summary.totalExp / 100.0) + 1;`



### 2. 객체 지향 프로그래밍 (OOP) (Chap 03, 04, 06)

각 기능을 담당하는 `Manager` 클래스들을 정의하여 책임을 분리하고 캡슐화했습니다.

* **클래스와 캡슐화 (Class & Encapsulation, Chap 03)**:
* 데이터를 처리하는 내부 함수(`ensureDataDir`, `loadAll` 등)는 `private`으로 숨기고, 외부에서 필요한 기능만 `public`으로 노출했습니다.
* **사용 예시:** `AccountManager`, `BudgetManager`, `CategoryManager` 클래스 정의.


* **Static 멤버 (Static Member, Chap 06)**:
* 객체 생성 없이도 호출할 수 있는 유틸리티성 함수를 `static`으로 정의했습니다.
* **사용 예시:** `CategoryManager`에서 속성(`enum`)을 문자열로 변환하는 함수.
* 코드: `static std::string attributeToString(CategoryAttribute attribute);`



### 3. STL과 알고리즘 (Chap 12, 17)

데이터를 효율적으로 관리하고 처리하기 위해 STL 컨테이너와 알고리즘이 핵심적으로 사용되었습니다.

* **STL 컨테이너 (Vector, Map, String, Chap 12, 17)**:
* `std::vector<Transaction>`: 거래 내역 목록을 관리.
* `std::unordered_map`: 카테고리별 속성이나 예산 집계에 사용.
* `std::string`: 날짜, 메모, 카테고리명 등 문자열 처리.


* **STL 알고리즘 (Algorithm, Chap 17)**:
* `std::sort`: 거래 내역을 날짜순으로 정렬하기 위해 사용했습니다.
* `std::find`, `std::find_if`: 특정 ID를 가진 거래나 스케줄을 찾거나, 중복된 카테고리를 검사할 때 사용했습니다.
* `std::remove_if`: 삭제할 항목을 벡터의 끝으로 보낸 뒤 제거하는 관용구(Erase-Remove Idiom)에 사용되었습니다.
* **사용 예시:** `AccountManager.cpp`의 `deleteTransaction` 함수.


* **람다 표현식 (Lambda, Chap 17)**:
* `httplib`의 요청 핸들러나 STL 알고리즘의 조건자(Predicate)로 람다 함수를 광범위하게 사용했습니다.
* **사용 예시:** `main.cpp`에서 날짜 비교를 위한 정렬 기준 정의.
* 코드: `std::sort(transactions.begin(), transactions.end(), [](const Transaction& a, const Transaction& b) { ... });`



### 4. 파일 입출력 및 파일 시스템 (Chap 01 응용)

데이터 영속성을 위해 파일 입출력이 구현되었습니다.

* **파일 입출력 (`fstream`)**:
* JSON 형식으로 데이터를 저장하고 불러오기 위해 `std::ifstream`, `std::ofstream`을 사용했습니다.


* **파일 시스템 (`filesystem`)**:
* 데이터 디렉터리 생성(`create_directories`), 파일 목록 순회(`directory_iterator`), 파일명 파싱 등에 C++17의 `std::filesystem`을 활용했습니다.



### 5. 예외 처리 (Exception Handling, Chap 15)

런타임 오류를 안전하게 처리하기 위해 예외 처리가 적용되었습니다.

* **Try-Catch 블록**:
* 클라이언트로부터 받은 JSON 데이터 파싱 실패, 파일 읽기 오류, 문자열 변환(`stoi`) 실패 등을 처리하기 위해 사용되었습니다.
* **사용 예시:** `main.cpp`의 API 라우팅 내부.
* 코드:
```cpp
try {
    auto payload = json::parse(req.body);
    // ...
} catch (const std::exception& ex) {
    // 에러 응답 반환
}

```

---

### 요약


| cpp-basic 학습 내용 | 가계부 프로젝트 적용 사례 | 주요 파일 |
| --- | --- | --- |
| **참조자/Const** | `const std::string&` 등으로 불필요한 복사 방지 | 모든 Manager 파일 |
| **Class/OOP** | `Manager` 클래스들을 통한 기능 모듈화 및 캡슐화 | `*.h`, `*.cpp` |
| **Static** | 유틸리티 함수(Enum 변환 등)를 정적 멤버로 구현 | `CategoryManager.h` |
| **STL (Vector/Map)** | 거래 내역(`vector`), 카테고리(`unordered_map`) 관리 | `AccountManager.h`, `CategoryManager.h` |
| **STL Algorithm** | `sort`(정렬), `find_if`(검색), `remove_if`(삭제) | `AccountManager.cpp`, `main.cpp` |
| **Lambda** | HTTP 요청 처리 핸들러 및 정렬 기준 정의 | `main.cpp` |
| **Exception** | JSON 파싱 및 데이터 유효성 검사 시 예외 처리 | `main.cpp`, `AccountManager.cpp` |
| **Casting** | RPG 로직 계산 시 `static_cast` 활용 | `main.cpp` |

---


---

## 안 배운 내용

### 1. **멀티스레딩 및 동시성 제어 (Concurrency)**
* **내용:** 여러 작업이 동시에 데이터에 접근할 때 발생할 수 있는 문제를 방지하기 위해 `std::mutex`와 `std::lock_guard`를 사용하여 스레드 안전성(Thread Safety)을 확보했습니다. 또한 `std::thread`를 사용해 서버를 별도 스레드에서 실행시켰습니다.
* **관련 파일:** `AccountManager.h`, `main.cpp`



### 2. **Modern C++ (C++17) 최신 기능**
* **`std::filesystem`:** 파일 및 디렉터리 경로를 객체지향적으로 다루고, 폴더 생성이나 파일 목록 순회를 간편하게 처리했습니다.
* **`std::optional`:** 값이 있을 수도 있고 없을 수도 있는 상황(예: 검색 결과 없음)을 안전하게 처리하기 위해 사용했습니다.
* **구조적 바인딩 (Structured Binding):** 맵(Map) 등을 순회할 때 `auto [key, value]` 문법을 사용하여 코드를 간결하게 작성했습니다.
* **관련 파일:** `AccountManager.cpp`, `BudgetManager.cpp`



### 3. **외부 라이브러리 활용**
* **내용:** 표준 라이브러리(STL)에 없는 기능을 구현하기 위해 오픈소스 라이브러리를 프로젝트에 통합했습니다.
* **`nlohmann::json`:** 복잡한 데이터 구조를 JSON 형식으로 직렬화/역직렬화.
* **`httplib`:** 내장 웹 서버 구축 및 REST API 통신 처리.



### 4. **정밀한 시간 처리 (`std::chrono`)**
* **내용:** 단순한 `time_t`를 넘어, `std::chrono` 라이브러리를 사용하여 밀리초 단위의 타임스탬프(ID 생성용)를 다루거나 시간 간격을 계산했습니다.
* **관련 파일:** `main.cpp`, `ScheduleManager.cpp`
