# Prototype_MuseumHeist_InventoryGrid

> Status: **Closed / Migration Ready**
> Migration Readiness: **Ready with cleanup**

`Prototype_MuseumHeist_InventoryGrid`는 향후 개발 예정인 **Museum Heist** 프로젝트의 핵심 시스템 중 하나인
**4×5 서버 권한 그리드 인벤토리 시스템**을 독립적으로 검증하기 위한 Unreal Engine 5 C++ 프로토타입입니다.

본 저장소는 완성 게임 프로젝트가 아니라, 본 게임 개발 전에 기술 리스크를 줄이기 위한 **Inventory Spike Prototype**입니다.

---

## 프로젝트 목적

Museum Heist는 4인 경쟁 탑다운 잠입 액션 게임을 목표로 하며, 플레이어는 제한된 배낭 공간 안에서 유물을 훔치고, 무게 증가에 따른 이동 속도 패널티를 감수하며, 다른 플레이어를 방해하고 탈출해야 합니다.

이 과정에서 인벤토리 시스템은 단순한 아이템 보관 기능이 아니라 다음 시스템들과 직접 연결됩니다.

* 유물 획득
* 점수 계산
* 무게 계산
* 이동 속도 감소
* 아이템 드롭
* QuickSlot 사용
* 피격 시 유물 드롭
* 서버 권한 검증
* 멀티플레이어 동기화

따라서 본 프로젝트에서는 Museum Heist 본 개발에 들어가기 전, 가장 위험도가 높은 인벤토리 핵심 로직을 별도 프로토타입으로 분리하여 검증했습니다.

---

## 핵심 목표

이 프로토타입의 목표는 다음과 같습니다.

* 4×5 그리드 인벤토리 구조 검증
* 다중 셀 아이템 배치 검증
* 아이템 이동 / 회전 / 제거 / 드롭 검증
* 그리드 경계 및 아이템 중첩 검사
* 무게 / 점수 / 사용 슬롯 계산
* `InstanceId` 기반 Q/E/R QuickSlot 참조 검증
* UMG 기반 Drag & Drop 조작 검증
* 서버 권한 기반 인벤토리 변경 흐름 검증
* 잘못된 요청에 대한 서버 거부 및 UI 복구 검증
* 드롭 아이템의 월드 재생성 및 재획득 검증
* 본 프로젝트 이식을 위한 구조 적합성 검토

---

## 개발 및 검증 단계

| 단계                                               | 상태 | 목적                         |
| ------------------------------------------------ | -- | -------------------------- |
| INV-SPIKE-001: Core Grid Logic                   | 완료 | 4×5 인벤토리의 핵심 데이터 로직 검증     |
| INV-SPIKE-002: UMG Drag & Drop Prototype         | 완료 | 실제 UI 조작 계층 검증             |
| INV-SPIKE-003: Replication / FastArray Readiness | 완료 | 서버 권한, 복제 준비성, 실패 요청 복구 검증 |
| INV-SPIKE-004: Migration Plan to Main Project    | 완료 | 본 프로젝트 이식 계획 수립            |

---

## INV-SPIKE-001: Core Grid Logic

4×5 인벤토리의 핵심 데이터 로직을 구현하고 검증한 단계입니다.

### 구현 범위

* 4×5 다중 셀 인벤토리
* 아이템 자동 배치
* 아이템 이동
* 아이템 회전
* 아이템 제거 및 드롭
* 그리드 경계 검사
* 아이템 중첩 검사
* 무게 계산
* 점수 계산
* 사용 슬롯 계산
* `InstanceId` 기반 QuickSlot 참조
* 서버 권한 기반 픽업 및 인벤토리 변경
* 드롭 아이템의 월드 픽업 재생성
* 무작위 필드 픽업 생성
* 테스트용 인벤토리 UI 및 조작 키

### 검증 결과

| 검증 항목                       | 결과 |
| --------------------------- | -- |
| 플레이어별 인벤토리 상태 독립            | 통과 |
| 동일 픽업 동시 획득 시 한 명만 획득       | 통과 |
| 공간 부족 시 픽업이 월드에 유지          | 통과 |
| 실패한 이동 / 회전 시 기존 상태 유지      | 통과 |
| QuickSlot 아이템 드롭 시 참조 자동 해제 | 통과 |
| 드롭 아이템을 다른 플레이어가 재획득        | 통과 |

### 결론

Core Grid Logic 단계에서는 전체 핵심 검증 항목이 통과했습니다.

인벤토리 경계 이탈, 아이템 중첩, 실패한 변경으로 인한 상태 손상은 발생하지 않았으며, 서버가 최종 상태 변경을 담당하고 클라이언트는 요청만 전달하는 구조를 확인했습니다.

---

## INV-SPIKE-002: UMG Drag & Drop Prototype

기존에 검증된 Core Grid Logic 위에 UMG 기반 Drag & Drop 조작을 연결하고 검증한 단계입니다.

### 구현 범위

* 4×5 그리드 UI 표시
* 다중 셀 아이템 UI 표시
* 아이템 Drag & Drop 이동
* 아이템의 여러 지점을 잡은 상태에서 이동
* 마우스 앵커 보정
* 배치 가능 / 불가능 프리뷰
* 1×3 ↔ 3×1 회전
* 회전 상태에서 마우스 위치 보정
* 그리드 경계 초과 방지
* 아이템 전체 / 부분 겹침 방지
* 잘못된 배치 시 원래 상태 유지
* 인벤토리 외부 드롭
* 월드 아이템 재생성
* 드롭 아이템 재습득
* Q/E/R QuickSlot UI 할당
* QuickSlot 참조 자동 해제
* 서버 권한 기반 이동 / 회전 / 드롭 / QuickSlot 할당

### 검증 결과

| 검증 항목                           | 결과 |
| ------------------------------- | -- |
| 4×5 그리드와 다중 셀 아이템 표시            | 통과 |
| 아이템 위치와 실제 점유 슬롯 일치             | 통과 |
| 아이템의 여러 지점을 잡은 일반 이동            | 통과 |
| 이동 후 무게 / 점수 / 사용 슬롯 유지         | 통과 |
| 1×3 ↔ 3×1 회전 및 마우스 위치 보정        | 통과 |
| 아이템 표시 영역과 배치 프리뷰 일치            | 통과 |
| 그리드 경계 초과 방지                    | 통과 |
| 아이템 간 전체 / 부분 겹침 방지             | 통과 |
| 잘못된 배치 시 원래 상태 유지               | 통과 |
| 외부 드롭 및 월드 아이템 재생성              | 통과 |
| 재습득 시 인벤토리 정상 배치                | 통과 |
| Q/E/R QuickSlot `InstanceId` 할당 | 통과 |
| QuickSlot 할당 아이템 제거 시 참조 해제     | 통과 |
| 멀티플레이 서버 권한 처리                  | 통과 |
| 플레이어별 인벤토리 상태 분리                | 통과 |

### 권한 경계

UI는 배치 가능 여부만 로컬에서 미리 표시합니다.

실제 이동, 회전, 드롭 및 QuickSlot 할당은 기존 서버 RPC와 서버 검증을 거쳐 확정됩니다.

잘못된 요청은 서버에서 거부되며, UI는 서버에서 확정된 인벤토리 상태를 기준으로 다시 표시됩니다.

### 결론

UMG Drag & Drop Prototype 단계에서는 전체 요구사항과 수동 검증 항목이 통과했습니다.

이를 통해 인벤토리의 핵심 조작 계층은 본 프로젝트에 이식 가능한 수준의 기반을 확보했습니다.

---

## INV-SPIKE-003: Replication / FastArray Readiness

인벤토리 시스템이 멀티플레이 환경에서 서버 권한 기반으로 안정적으로 동작할 수 있는지 검증하고, 향후 FastArray 전환 가능성을 점검한 단계입니다.

### 구현 및 검증 범위

* 서버 권한 기반 인벤토리 mutation
* Owner Only 인벤토리 복제
* 서버 확정 상태 기반 UI refresh
* 서버 rejection handling
* 실패 요청 후 UI 복구
* `F8` invalid request test
* `F9` inventory validation report
* 동일 Pickup 동시 획득 방지
* QuickSlot stale reference 정리
* InstanceId 안전성 검토
* FastArray 전환 notes 작성
* 재사용 가능 코드 / prototype-only 코드 분류

### 검증 결과

| 검증 항목                | 결과 |
| -------------------- | -- |
| 기본 인벤토리 복제           | 통과 |
| 클라이언트별 인벤토리 분리       | 통과 |
| 잘못된 요청 서버 거부         | 통과 |
| 거부 후 UI 상태 복구        | 통과 |
| F8 rejection test    | 통과 |
| F9 validation report | 통과 |
| QuickSlot 참조 유지 / 정리 | 통과 |
| Drop 후 재획득           | 통과 |
| 동일 Pickup 경쟁 방지      | 통과 |
| 그리드 경계 / 겹침 방지       | 통과 |

### 검증된 서버 권한 흐름

```text
Client UI Preview
→ Server RPC Request
→ Server Validation
→ Server Mutation or Rejection
→ Client UI Refresh from Confirmed State
```

클라이언트는 `Items`나 `QuickSlots`를 직접 수정하지 않고, 이동 / 회전 / 드롭 / QuickSlot 할당을 서버에 요청합니다.

서버는 현재 인벤토리 상태 기준으로 요청을 검증하고, 유효하지 않은 요청은 상태 변경 없이 거부합니다.

### InstanceId 판단

현재 `InstanceId`는 인벤토리 컴포넌트별 단조 증가 `int32` 값입니다.

스파이크 범위에서는 충분히 안전합니다.

* 한 인벤토리 내부에서 active item ID가 중복되지 않음
* QuickSlot은 같은 인벤토리 내부의 InstanceId만 참조
* 다른 플레이어가 같은 숫자의 InstanceId를 가져도 소유 인벤토리가 다르므로 충돌하지 않음
* 드롭 후 다른 플레이어가 다시 주우면 해당 플레이어 인벤토리에서 새 InstanceId 부여

단, 본 프로젝트에서 저장/로드, 거래, 영구 소유권, 전역 추적이 필요해진다면 `FGuid` 또는 서버 전역 ID를 검토할 수 있습니다.

### FastArray 판단

이번 스파이크에서는 FastArray로 직접 전환하지 않았습니다.

현재 배열 전체 복제 방식은 작은 4×5 인벤토리 프로토타입에는 충분합니다.
다만 본 프로젝트로 옮겨갈 때는 다음 이유로 FastArray 전환을 검토할 가치가 있습니다.

* 아이템 단위 변경 복제
* Add / Remove / Change 콜백 분리
* 변경량 기반 네트워크 최적화
* 향후 인벤토리 확장 가능성

주의할 점은 다음과 같습니다.

* `MarkItemDirty`, `MarkArrayDirty` 누락 시 client desync 가능
* QuickSlot RepNotify와 FastArray callback의 중복 알림 정리 필요
* Pawn 소유 유지 여부와 PlayerState 이전 여부 결정 필요

---

## INV-SPIKE-004: Migration Plan to Main Project

검증된 인벤토리 프로토타입을 본 프로젝트인 Museum Heist에 이식하기 위한 구조 판단 단계입니다.

### 최종 판단

```text
Migration Readiness: Ready with cleanup
```

스파이크는 본 프로젝트 이식 계획 단계로 넘어갈 수 있습니다.

다만 다음 요소는 production 코드로 직접 가져가면 안 됩니다.

* `F8` rejection test
* `F9` debug dump
* Debug UI
* 하드코딩된 테스트 아이템
* 임시 GameMode
* 임시 Character
* 랜덤 Pickup field
* cube/text 기반 pickup presentation
* verbose debug logs

---

## 재사용 가능한 코드

| 파일 / 시스템                          | 결정             | 메모                                                               |
| --------------------------------- | -------------- | ---------------------------------------------------------------- |
| `FHeistInventoryItem`             | 정리 후 재사용       | 런타임 item instance 구조는 유효함                                        |
| `UHeistInventoryComponent`        | 핵심 로직 정리 후 재사용 | Add, Move, Rotate, Remove, Drop, QuickSlot assignment 검증 완료      |
| Grid placement / overlap / bounds | 재사용            | Coord/index 변환, effective size, occupancy grid, CanPlaceItem 재사용 |
| QuickSlot InstanceId 모델           | 재사용            | QuickSlot은 item data 복사가 아니라 InstanceId 참조로 유지                   |
| Server request/result flow        | 재사용            | 클라이언트 요청, 서버 검증, 성공/거부 결과 반환 구조 유지                               |
| FastArray-ready 구조 개념             | 전환 목표로 재사용     | item entry 구조는 FastArray container로 감싸기 적합                       |
| Pickup claim 패턴                   | 재작성 후 재사용      | 동일 pickup 경쟁 방지는 유효하지만 world presentation은 교체 필요                 |

---

## Prototype-only 코드

| 파일 / 시스템                      | 처리              | 이유                                               |
| ----------------------------- | --------------- | ------------------------------------------------ |
| `HeistPrototypeCharacter`     | 제거              | 스파이크용 입력 / 이동 / 디버그 조합                           |
| `HeistPrototypeGameModeBase`  | 제거              | 프로토타입 map / game mode wiring                     |
| `HeistInventoryDebugWidget`   | Debug로 분리 또는 제거 | production MVVM / UMG UI가 아님                     |
| cube/text pickup presentation | 제거              | production mesh / icon / outline / prompt로 교체 필요 |
| random pickup field           | Debug로 이동       | stress test에는 유용하지만 production spawn path가 아님    |
| hardcoded test presets        | 제거              | DataTable item definition으로 교체                   |
| F8 / F9                       | 개발 전용으로 이동      | 검증 도구이며 gameplay input이 아님                       |
| verbose logs                  | 개발 조건으로 제한      | shipping/runtime log noise 방지                    |

---

## 본 프로젝트 이식 권장안

### 인벤토리 소유권

v1.0에서는 **Hybrid 구조**를 권장합니다.

```text
Character / Pawn:
- active InventoryComponent
- QuickSlot use
- drag/drop request source
- current carried item state

PlayerState:
- TotalLootScore
- FinalScore
- Escape state
- Player color
- Result screen data
```

이 구조는 스파이크 구조를 크게 해치지 않으면서도 결과 정산과 PlayerState 데이터를 안정적으로 관리할 수 있습니다.

리스폰, Pawn 교체, reconnect, persistent match inventory가 요구되면 PlayerState-owned 구조로 전환합니다.

---

### 복제 전략

초기 연결은 현재 구조로도 가능하지만, 본 프로젝트에서 인벤토리가 다른 시스템과 깊게 결합되기 전 FastArray 전환을 권장합니다.

권장 흐름:

```text
1. 본 프로젝트에 InventoryComponent 이식
2. DataTable 기반 아이템 정의로 교체
3. 2-client / 4-client 검증
4. FastArray 전환 여부 결정
5. UI / QuickSlot / Result 시스템과 깊게 연결
```

---

### 아이템 정의

핵심 numeric / balance 정보는 **DataTable** 사용을 권장합니다.

DataAsset은 복잡한 asset bundle이나 item-specific behavior data가 필요해질 때 추가합니다.

권장 필드:

| 필드                                    | 용도                                     |
| ------------------------------------- | -------------------------------------- |
| ItemId                                | 안정적인 row/key identifier                |
| ItemTag                               | gameplay/category tag                  |
| ItemType                              | Loot, Utility, Trap, Coin, Rare Loot 등 |
| Width / Height                        | inventory grid size                    |
| Weight                                | movement penalty와 carry calculation    |
| ScoreValue                            | loot/result scoring                    |
| Icon                                  | UI item display                        |
| WorldActorClass                       | pickup/drop actor spawn                |
| bCanRotate / bCanDrop / bCanQuickSlot | item별 허용 flag                          |
| UseType / CooldownTag                 | QuickSlot use routing과 cooldown        |

---

### MVVM / UI 통합 계획

production UI는 C++ ViewModel이 UI 데이터를 준비하고, Blueprint UMG는 layout, animation, drag visual, color, font, screen composition을 담당하는 구조가 적합합니다.

권장 흐름:

```text
InventoryComponent
→ InventoryViewModel
→ WBP_Inventory
```

역할 분리:

| 계층                 | 역할                                                         |
| ------------------ | ---------------------------------------------------------- |
| InventoryComponent | 실제 인벤토리 데이터, 서버 RPC, mutation 검증                           |
| InventoryViewModel | confirmed inventory state를 cell/item view data로 변환         |
| QuickSlotViewModel | Q/E/R InstanceId를 display data로 resolve                    |
| HUDViewModel       | weight, score, used slot, carry status, pickup feedback 표시 |
| WBP_Inventory      | 레이아웃, Drag Visual, 색상, 애니메이션, 입력 전달                        |

UMG Widget은 inventory array를 직접 수정하지 않습니다.
모든 mutation은 ViewModel command를 통해 InventoryComponent의 server request로 전달해야 합니다.

---

## 본 프로젝트 Gameplay 통합 지점

| 시스템                | 현재 스파이크 지원         | 본 프로젝트 작업                                           |
| ------------------ | ------------------ | --------------------------------------------------- |
| Loot pickup        | 부분 지원              | DataTable lookup과 production interaction prompt로 교체 |
| Score calculation  | item score sum 지원  | PlayerState / result scoring과 연결                    |
| Weight calculation | item weight sum 지원 | movement speed penalty와 HUD feedback 연결             |
| Movement penalty   | production 미구현     | server-confirmed weight 기준으로 CharacterMovement에 적용  |
| QuickSlot use      | assignment만 지원     | Coin / Smoke / Glue 사용 로직 추가                        |
| Coin               | test item 수준       | row, icon, behavior, pickup presentation 정의         |
| Smoke Grenade      | 미구현                | effect spawn, cooldown, use validation 필요           |
| Glue Trap          | identity만 있음       | trap placement / use behavior 필요                    |
| Piñata Drop        | 미구현                | 피격 시 랜덤 Loot drop 로직 필요                             |
| Loot Dumping       | external drop 지원   | server-authoritative dump mutation 필요               |
| Rare Loot          | generic loot로 가능   | item type, score rule, UI treatment 필요              |
| Result scoring     | score sum 지원       | final aggregation을 PlayerState / result system으로 이전 |
| Gap Tracker        | 미구현                | confirmed score 기반 recalculation 필요                 |

---

## 마이그레이션 전 정리 체크리스트

* [ ] F8 / F9 debug controls 제거 또는 개발 전용으로 격리
* [ ] Debug logs를 development-only 조건 뒤로 이동
* [ ] Hardcoded item presets를 DataTable lookup으로 교체
* [ ] Pawn vs PlayerState ownership 최종 결정
* [ ] FastArray 전환 시점 결정
* [ ] Prototype class naming 정리
* [ ] Reusable inventory structs와 component logic 추출
* [ ] Prototype GameMode dependency 제거
* [ ] Random pickup spawner를 production path에서 제거
* [ ] QuickSlot은 InstanceId reference로 유지
* [ ] UI는 mutation 요청만 보내고 state를 직접 수정하지 않도록 보장
* [ ] 2-client / 4-client validation 절차 유지

---

## 권장 마이그레이션 순서

1. 본 프로젝트에 Inventory module/folder 생성
2. 재사용 가능한 inventory structs 이식
3. InventoryComponent core logic 이식
4. hardcoded item preset을 DataTable lookup으로 교체
5. replication strategy 결정 및 적용
6. PlayerState score / weight / result summary 연결
7. Character movement penalty 연결
8. QuickSlot use actions 연결
9. MVVM InventoryViewModel 연결
10. prototype behavior를 기준으로 WBP_Inventory 재구성
11. development-only debug tools를 별도 경로에 추가
12. 2-client 및 4-client validation 실행
13. Progress Board 갱신

---

## 남은 리스크

| 리스크                                    | 영향                | 대응                                                  |
| -------------------------------------- | ----------------- | --------------------------------------------------- |
| FastArray dirty marking 누락             | client desync     | add/move/rotate/remove/drop test 추가                 |
| Items와 QuickSlot notification ordering | 일시적 stale UI      | defensive UI와 revision number 검토                    |
| Pawn-owned 상태에서 pawn 교체                | inventory loss    | respawn 요구 시 PlayerState-owned로 전환                  |
| DataTable row mismatch                 | 잘못된 item 생성       | startup row validation과 server-side reject          |
| UI preview와 server confirmed state 충돌  | 잘못된 표시            | preview는 provisional로 취급하고 confirmed state로 rebuild |
| item consume logic 미검증                 | QuickSlot use bug | Coin / Smoke / Glue use-item 검증 필요                  |
| main project integration regression    | 검증된 로직 훼손         | 단계별 2-client / 4-client validation 유지               |

---

## 최종 상태

본 스파이크는 완료되었습니다.

이 저장소는 더 이상 신규 기능을 추가하지 않으며, 향후 Museum Heist 본 프로젝트에 인벤토리 핵심 로직을 이식하기 위한 참고 / 검증 저장소로 유지합니다.

최종 판단:

```text
Migration Readiness: Ready with cleanup
```

다음 실질 단계는 `Project_MuseumHeist` 본 프로젝트를 생성하고, 검증된 reusable core만 필요한 시점에 이식하는 것입니다.

---

## 기술 키워드

* Unreal Engine 5
* C++
* UMG
* Drag & Drop
* Server Authority
* Owner Only Replication
* Grid Inventory
* Multi-cell Item Placement
* Item Rotation
* QuickSlot
* InstanceId
* Multiplayer Validation
* FastArray Readiness
* MVVM
* Prototype
* Technical Spike

---

## 프로젝트 성격

이 저장소는 완성 게임 프로젝트가 아니라, 본 프로젝트 개발 전 핵심 시스템의 위험도를 줄이기 위한 기술 검증용 저장소입니다.

본 게임 프로젝트에서는 이 프로토타입에서 검증된 구조를 기반으로 정식 UI, 데이터 테이블, 복제 최적화, 게임플레이 시스템 연동을 추가할 예정입니다.
