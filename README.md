## 0. 커밋 메세지 자동화
깃 pull 받은 후 **레포루트**에서 딱 한 번만 실행 하면 됨
```
git config core.hooksPath .githooks
git config --get core.hooksPath
```
두번째 줄 실행 시 .githooks가 나오면 설정 완료  
(예시)
```
Feat: 커밋메세지 자동화
```
와 같이 입력하면 
```
✨ Feat: 커밋메세지 자동화 [S14P21A302-47]
```
같이 써짐

## 1. 커밋 메시지 구조

커밋 메시지는 **헤더(Header)**, **본문(Body)**, **꼬리말(Footer)**로 구성합니다. (본문과 꼬리말은 선택 사항)

```markdown
이모지 태그: 제목 [지라Key]

- 본문: 무엇을, 왜 변경했는지 상세 설명 (선택 사항)
- 꼬리말: 이슈 닫기 등 (선택 사항)
```

## 2. 커밋 헤더 작성 규칙

가장 중요한 첫 줄입니다. 지라 연동을 위해 **대괄호 `[]` 안에 이슈 키**를 반드시 넣습니다.

> 포맷: [이슈키] 이모지 태그: 제목
> 
> 
> 예시: ✨ Feat: 플레이어 점프 기능 구현 [S14P11A402-56] 
> 

### 🎨 이모지 및 태그 리스트 (Gitmoji)

실무에서 가장 많이 사용하는 **핵심 8가지**입니다. (디자인/UI 포함)

| **이모지** | **단축키 (Code)** | **태그 (Type)** | **설명 (Description)** |
| --- | --- | --- | --- |
| ✨ | `:sparkles:` | **Feat** | 새로운 기능 추가 |
| 🐛 | `:bug:` | **Fix** | 버그 수정 |
| 💄 | `:lipstick:` | **Design** | CSS, UI/UX 디자인 변경 (로직 변경 없음) |
| 📝 | `:memo:` | **Docs** | 문서 수정 (README, 주석, 위키 등) |
| ♻️ | `:recycle:` | **Refactor** | 코드 리팩토링 (기능 변경 없음, 구조 개선) |
| 🎨 | `:art:` | **Style** | 코드 포맷팅, 세미콜론, 공백 등 (코드 동작 영향 없음) |
| ✅ | `:white_check_mark:` | **Test** | 테스트 코드 추가 및 리팩토링 |
| 🔨 | `:hammer:` | **Chore** | 빌드 설정, 패키지 매니저, 라이브러리 설치 |
| ⚠️ | `:?:` | **HotFix** | 빌드오류 등 급한 수정 |
| 🔁 | `:?:` | **Merge** | 머지 |
---

### 💡 꿀팁: 단축키 사용법

1. **VS Code:** `Gitmoji` 확장 프로그램을 설치했다면, 커밋 창에서 단축키(`:bug:`)를 입력할 때 자동완성됩니다.
2. **Terminal:** 대부분의 터미널은 단축키 텍스트(`:bug:`) 그대로 커밋해도, 깃허브나 깃랩 웹사이트에서는 자동으로 아이콘(🐛)으로 변환되어 보입니다.
3. **Windows:** 단축키 외우기 귀찮다면 `Win` + `.` (마침표) 키를 눌러 이모지 패널을 여세요.

## 3. 상황별 작성 예시

**① 새로운 기능을 개발했을 때 (Feat)**

`✨ Feat: 인벤토리 UI 구현 및 아이템 사용 검증 추가 [S14P11A402-56]`

**② 버그를 잡았을 때 (Fix)**

`🐛 Fix: 보스 패턴 2페이즈에서 히트박스 판정 오류 수정 [S14P11A402-56]`

**③ 문서나 설정 파일을 건드렸을 때 (Docs, Chore)**

`📝 Docs: README.md에 빌드/실행(에디터 포함) 가이드 추가 [S14P11A402-56]`

`🔨 Chore: Unity 패키지(URP/Addressables) 버전 업데이트 [S14P11A402-56]`

**④ 코드를 깔끔하게 정리했을 때 (Refactor, Style)**

`♻️ Refactor: 전투 데미지 계산 로직을 CombatService로 분리 [S14P11A402-56]`

`🎨 Style: C\# 코드 스타일 규칙에 맞춰 네이밍/들여쓰기 정리 [S14P11A402-56]`

**⑤ 디자인만 변경했을 때 (Design)**

`💄 Design: 메인 메뉴 버튼 색상/패딩 조정 및 UI 정렬 개선 [S14P11A402-56]`

## 4. 브랜치 이름 규칙

> 구조: 타입/설명-지라키[-컴포넌트]
> 
> 
> 예시: docs/build-guide-S14P11A402-56-pm
> 
> ### 1. 상세 구조 설명
> 
> 1. **타입 (Type):** 작업의 종류 (폴더로 구분됨)
> 2. **설명 (Description):** 무엇을 하는지 (영어 소문자, 띄어쓰기는 하이픈 )
> 3. **지라키 (Jira Key):** 지라 이슈 번호
> 4. **컴포넌트 (Component):** (선택) `pm`, `fe`, `be` 등 파트나 모듈 구분
> 
> ### 2. 작성 예시 (Examples)
> 
> **[Front-end]**
> 
> - **기능:** `feat/inventory-ui-S14P11A402-56-fe`
> - **디자인:** `design/main-menu-layout-S14P11A402-60-fe`
> 
> **[Back-end]**
> 
> - **기능:** `feat/matchmaking-api-S14P11A402-59-be`
> - **버그:** `fix/session-token-expire-S14P11A402-58-be`
> 
> **[Embedded]**
> 
> - **기능:** `feat/camera-follow-smoothing-S14P11A402-61-em`
> - **잡일:** `chore/build-pipeline-setup-S14P11A402-62-em`
> 
> **[Common/PM]**
> 
> - **문서:** `docs/game-design-doc-S14P11A402-56-pm`

## 5. PR(MR)규칙

### 1. Target Branch 변경 (자동화 완료)

- 상단 `Change branches` 클릭.
- Target을 `master` ➡️ **`dev`*로 변경.

### 2. Title (제목)

- 포맷: `[지라키] 이모지 태그: 핵심 내용`
- 예시: `[S14P11A402-56] ✨ Feat: 아이템 드랍 시스템 구현`

### 3. Mark as draft (진행 상태)

- ✅ **체크:** 작업 중 (WIP). 머지 버튼 잠김.
- ⬜ **해제:** 개발 완료. 리뷰 및 머지 요청.

### 4. 담당자 및 리뷰어

- **Assignee:** `Assign to me` 클릭 (본인).
- **Reviewer:** 검토할 팀원 1명 지정 (안전장치).

### 5. Merge options (옵션)

- ✅ **Delete source branch:** **필수 체크**. (머지 후 브랜치 자동 삭제)
- ⬜ **Squash commits:**  체크 X (= 커밋들을 통합 하나로 통합 X)
- **Labels, Milestone** 은 체크 X

### 6. Merge Templates

```jsx
## 📝 작업 요약
	- (예시) Addressables로 리소스 로딩 적용

## 😈 주의 해야 할 점
	- (예시) 빌드 타겟(Android/iOS/PC)별 설정 확인 및 버전 동기화

## ✅ 체크리스트
- [ ] Merge 되는 브랜치가 `develop`이 맞나요?
- [ ] 리뷰어를 지정했나요?
```

## 💡 주의 할 점

1. **커밋 첫 줄 맨 앞**에 `[S14P...-번호]`를 꼭 붙여주세요. (지라 연동 필수!)
2. 제목은 **"~함", "~구현", "~수정"** 처럼 명사형이나 과거형으로 간결하게 적어주세요.