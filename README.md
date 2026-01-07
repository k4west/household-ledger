# household-ledger

## 사용 방법

### 빌드
```bash
g++ -std=c++17 -pthread main.cpp -o household-ledger
```

### 실행
```bash
./household-ledger
```

서버는 `http://localhost:8080`에서 실행됩니다. `data/ledger.json` 파일에 거래 내역이 저장됩니다.

### API 테스트 예시
```bash
curl http://localhost:8080/api/transactions
```

## 백엔드(C++) 파일 위치
웹 프론트엔드를 추가할 때를 대비해, 현재 C++ 백엔드 관련 파일은 루트에 정리되어 있습니다.

```
/workspace/household-ledger
  ├── main.cpp            # 서버 + 콘솔 UI 진입점
  ├── AccountManager.h    # 데이터 관리 및 파일 저장
  ├── Transaction.h       # 거래 데이터 모델
  ├── httplib.h           # cpp-httplib (단일 헤더)
  ├── json.hpp            # nlohmann/json (단일 헤더)
  └── data/               # JSON 데이터 저장 폴더
```
