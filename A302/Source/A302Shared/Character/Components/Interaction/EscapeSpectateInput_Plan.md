# 탈출 관전 입력 연결 계획서

## 현재 구조 파악

`CharacterActionInputComponent::OnAttack()`은 이미 플레이어 상태에 따라 분기 처리를 하고 있습니다.

```
[공격 버튼(IA_Attack) 입력]
    ↓
OnAttack()
    ├─ IsDead() == true  → CycleAlivePlayerViewTarget()  (사망 관전 전환)
    └─ IsDead() == false → 일반 공격 처리
```

## 문제

탈출(bIsEscaped == true)한 플레이어는 `IsDead() == false`이기 때문에 공격 버튼을 눌러도 일반 공격 분기로 들어갑니다. 하지만 탈출 후에는 이미 입력이 비활성화(`DisableInput`)된 상태이므로 OnAttack 자체가 호출되지 않습니다.

→ **별도 입력 바인딩이 필요합니다.**

## 선택한 방법: OnAttack() 분기 추가

`HandleEscapedState()`에서 `DisableInput(PC)`를 호출하고 있어서 `IA_Attack` 이벤트 자체가 차단됩니다. 따라서 다음 두 가지 중 하나를 선택해야 합니다.

| 방법 | 설명 | 장단점 |
|------|------|--------|
| **A. OnAttack 분기 추가 + DisableInput 제거** | DisableInput 대신 공격/이동만 막고, 입력 컴포넌트는 살려둠 | 기존 버튼 재활용, 코드 최소 변경 |
| **B. PlayerController에 마우스 클릭 직접 바인딩** | PC 레벨에서 마우스 버튼을 별도로 처리 | 입력 비활성화 영향 없음, PC 레벨 처리 |

### 선택: **방법 A** (DisableInput 범위 조정 + OnAttack 분기)

이유:
- 사망 관전과 동일한 UX (같은 버튼)
- Blueprint 수정 불필요
- 입력 컴포넌트를 살리되 공격 로직만 막으면 됨

## 구현 내용

### 1. `MyCharacter::HandleEscapedState()` 수정

`DisableInput(PC)` 대신 이동/룩 입력만 막고, 입력 컴포넌트 자체는 비활성화하지 않습니다.

```cpp
// 변경 전
PC->SetIgnoreMoveInput(true);
PC->SetIgnoreLookInput(true);
DisableInput(PC);  // ← 입력 컴포넌트 전체 비활성화 → OnAttack도 차단됨

// 변경 후
PC->SetIgnoreMoveInput(true);
PC->SetIgnoreLookInput(true);
// DisableInput 제거: 이동/룩만 막고 클릭은 살려둠
```

### 2. `CharacterActionInputComponent::OnAttack()` 분기 추가

```
[공격 버튼(IA_Attack) 입력]
    ↓
OnAttack()
    ├─ bIsEscaped == true  → CycleEscapeSpectatorViewTarget()  ← 추가
    ├─ IsDead() == true    → CycleAlivePlayerViewTarget()
    └─ 둘 다 아님          → 일반 공격 처리
```

### 3. 추가 안전장치: 탈출 상태에서 공격 차단

`OnAttack()` 내에서 `bIsEscaped` 체크를 먼저 해서 공격 분기로 빠지지 않도록 합니다.

## 기대 효과

| 상황 | 버튼 동작 |
|------|----------|
| 살아있고 플레이 중 | 일반 공격 |
| 사망 후 관전 중 | 다음 플레이어로 관전 전환 (기존 동작 유지) |
| **탈출 후 관전 대기 중** | **다음 플레이어로 관전 전환 (신규)** |

- Blueprint 수정 없이 기존 입력 체계 재활용
- 플레이어 입장에서 "사망 때와 같은 버튼으로 관전 전환" → 직관적 UX
- `SetIgnoreMoveInput` / `SetIgnoreLookInput`으로 이동은 막히므로 카메라 회전 없음

## 수정 파일

1. `MyCharacter.cpp` — `HandleEscapedState()` : `DisableInput` 제거
2. `CharacterActionInputComponent.cpp` — `OnAttack()` : 탈출 분기 추가
