# Mausoleum

> 멀티플레이 협동 탈출 게임 — Unreal Engine 5 + C++ 전용 서버 (WebSocket / UDP)

---

## 목차

1. [프로젝트 개요](#프로젝트-개요)
2. [게임 설명](#게임-설명)
3. [시스템 아키텍처](#시스템-아키텍처)
4. [서버 구성 (WebSocket / Voice 서버)](#서버-구성-websocket--voice-서버)
5. [전용 서버 (Dedicated Server, DS)](#전용-서버-dedicated-server-ds)
6. [클라이언트 (Client)](#클라이언트-client)
7. [공유 코드 (Shared)](#공유-코드-shared)
8. [네트워크 포트 정보](#네트워크-포트-정보)
9. [게임 흐름 및 페이즈](#게임-흐름-및-페이즈)
10. [아이템 목록](#아이템-목록)
11. [조작법](#조작법)
12. [프로젝트 구조 요약](#프로젝트-구조-요약)

---

## 프로젝트 개요

**Mausoleum**은 여러 플레이어가 같은 공간에서 협동하며 탈출을 목표로 하는 멀티플레이 게임입니다. 게임은 크게 세 개의 페이즈로 진행되며, 각 페이즈마다 개인 이벤트와 그룹 이벤트가 발생합니다. 플레이어는 아이템을 획득·사용하여 다른 플레이어와 상호작용하거나 석상을 조작해 탈출구를 개방해야 합니다.

---

## 게임 설명

### 기본 정보

| 항목 | 내용 |
|------|------|
| **장르** | 멀티플레이 협동 탈출 (Co-op Escape) |
| **시점** | 1인칭 (기본: FirstPersonChest 카메라, 3인칭 전환 가능) |
| **플레이어** | 다중 플레이어 동시 진행 (방 단위 분리) |
| **목표** | 3단계 페이즈를 완료하고 탈출구 포탈을 통해 살아서 탈출 |
| **제한 시간** | 매치당 최대 666초 (약 11분) |
| **엔진** | Unreal Engine 5 (C++) |

---

### 게임 스타일

Mausoleum은 **1인칭 시점의 협동 탈출 게임**으로, 단순한 탈출만이 아니라 플레이어 간 사회적 긴장감을 만드는 **악의(Malice) 시스템**이 핵심입니다. 맵 내에서 아이템을 수집하고, 오브젝트를 클리어하며, 석상을 협동으로 조작해 탈출구를 개방해야 합니다. 그 과정에서 개인 이벤트가 발동되어 특정 플레이어에게 불리하거나 유리한 조건이 부여되고, 그룹 이벤트를 통해 전체 플레이어의 아이템 또는 상태가 영향을 받기도 합니다.

실시간 거리 기반 음성 채팅을 지원하여 가까운 플레이어의 목소리만 들을 수 있으며, 로비에서는 전체 방 브로드캐스트로 전환됩니다.

---

### 핵심 시스템

| 시스템 | 설명 |
|--------|------|
| **악의(Malice) 시스템** | 특정 행동을 하면 악의 수치가 쌓이고, 수치에 따라 이벤트 발동 조건이 달라짐. 악의가 과부하되거나 공개 선언(PublicMalice)되면 주변 플레이어에게 알려짐 |
| **개인 이벤트** | 특정 플레이어에게 개별적으로 발동. 저주받은 검 획득, 악의 검사, 정화의 불꽃 등 결과가 해당 플레이어의 상황을 크게 바꿈 |
| **그룹 이벤트** | 방 전체 플레이어에게 영향. 몰수(Confiscate) 이벤트로 아이템이 빼앗기거나, 심판(Judgment) 이벤트로 상태가 변함 |
| **페이즈 진행** | Phase0(아이템 수집) → Phase1(오브젝트 클리어) → Phase2(석상 협동 조작) → 탈출 순서로 진행 |
| **석상 상호작용** | Phase2에서 맵에 배치된 석상을 여러 플레이어가 협동하여 모두 완료해야 탈출구 차단이 해제됨 |
| **거리 기반 음성 채팅** | 인게임에서 가까운 플레이어의 목소리만 들림. 실시간 Opus 코덱으로 인코딩/전송 |
| **아이템 퀵슬롯** | 최대 6칸의 퀵슬롯. 1~6키로 슬롯 선택, 마우스 클릭으로 사용 |
| **탈출 포탈** | Phase2 석상 전체 완료 시 EscapeRouteBlocker가 해제되어 포탈 활성화 |

---

### 게임 진행 흐름

| 단계 | 행동 | 주요 시스템 |
|------|------|-------------|
| **로비** | 방 생성/입장, 로비 채팅, 대기 | WebSocket 로비 서버, 로비 음성 채팅 |
| **게임 시작** | 게임 맵으로 이동, 캐릭터 스폰 | Dedicated Server, Level Streaming |
| **Phase 0** | 맵 탐색, 아이템 수집 | 아이템 스폰 오케스트레이터, 퀵슬롯 |
| **Phase 1** | 특정 오브젝트 상호작용 완료 | 상호작용 컴포넌트, 개인 이벤트 발동 |
| **Phase 2** | 석상 협동 조작, 그룹 이벤트 처리 | 석상 인터랙터블, 그룹 이벤트 |
| **탈출** | 포탈을 통해 탈출, 결과 화면 | 탈출 차단 오브젝트 해제, 보상 처리 |

---

### 게임 시연 사진
<img width="2616" height="1632" alt="Image" src="https://github.com/user-attachments/assets/53d84e3d-c47d-4ee8-9011-1ed45583893f" />
<img width="2616" height="1632" alt="Image" src="https://github.com/user-attachments/assets/8fa78b8a-ca21-4a83-9008-abe96484c7a1" />
<img width="2616" height="1632" alt="Image" src="https://github.com/user-attachments/assets/abf81f3c-5013-4ea4-9393-77f1e1fa220b" />
<img width="2608" height="1632" alt="Image" src="https://github.com/user-attachments/assets/c6bc9c86-db94-4cf8-89d7-a837055050a9" />
<img width="2616" height="1632" alt="Image" src="https://github.com/user-attachments/assets/16bbda72-e30b-49cf-b8c9-6defa0b0e26a" />

---

## 시스템 아키텍처

```
┌──────────────────────────────────────────────────────────┐
│                      클라이언트 (UE5)                     │
│  A302Client + A302Shared                                 │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐ │
│  │  UI / HUD   │  │ 음성 채팅     │  │  게임플레이 로직  │ │
│  └──────┬──────┘  └───────┬──────┘  └────────┬─────────┘ │
│         │ WebSocket       │ UDP              │ UE Repl.  │
└─────────┼─────────────────┼──────────────────┼───────────┘
          │                 │                  │
          ▼                 ▼                  ▼
┌─────────────────┐  ┌──────────────┐  ┌─────────────────┐
│  C++ 로비 서버   │  │ Voice 서버   │  │  UE DS (전용     │
│  (WebSocket)    │  │  (UDP)       │  │  서버, 포트      │
│  포트 8001      │  │  포트 48100   │  │  47777)         │
│  - 방 목록/생성  │  │  - Opus 코덱 │  │  - 게임 진행     │
│  - 채팅         │  │  - 거리 기반  │  │  - 이벤트 판정   │
│  - 게임 시작     │  │    음성 믹싱 │  │  - 보상 처리     │
└────────┬────────┘  └──────────────┘  └────────┬────────┘
         │ (BackendService)                      │
         └──────────────────────────────────────┘
               WebSocket 백엔드 채널 (DS ↔ 로비 서버)
```

프로젝트는 세 개의 독립적인 서버 컴포넌트로 구성됩니다.

- **C++ 독립 서버 (`Server/GameServer`)**: 로비, 방 관리, 채팅, 음성 처리를 담당하는 순수 C++ 서버로 uWebSockets 기반의 WebSocket 서버와 UDP 기반 Voice 서버가 하나의 프로세스에서 멀티스레드로 동작합니다.
- **UE Dedicated Server (DS)**: Unreal Engine으로 구동되는 전용 서버로 실제 게임 월드를 관리하며, 페이즈 진행·아이템 스폰·이벤트 판정·보상 처리 등 게임 로직의 핵심을 담당합니다.
- **UE 클라이언트**: 플레이어가 실행하는 게임 클라이언트로 UI, 입력, 음성 캡처/재생, 서버 통신을 처리합니다.

---

## 서버 구성 (WebSocket / Voice 서버)

경로: `Server/GameServer/`

순수 C++로 작성된 독립 실행형 서버입니다. 빌드 시스템은 CMake이며 의존성 관리에 vcpkg를 사용합니다.

### 로비 서버 (WebSocket, 포트 8001)

| 경로 | 역할 |
|------|------|
| `src/main.cpp` | 진입점. 로비·음성 서버를 각각 별도 스레드에서 구동 |
| `include/websocket/WebSocketServer.h` | uWebSockets 기반 WebSocket 서버 추상화. 도메인별 메시지 라우팅 지원 |
| `include/websocket/WebSocketRouter.h` | 수신된 메시지를 도메인 핸들러로 분배 |
| `include/lobby/LobbyServer.h` | WebSocket 서버 + LobbyService + BackendService 통합 |
| `include/lobby/LobbyService.h` | 로비 도메인 메시지 처리 진입점 |
| `include/lobby/LobbyPacketRouter.h` | 패킷 타입별 핸들러(Room/Chat/Game) 라우팅 |
| `include/lobby/handlers/RoomHandler.h` | 방 목록 조회, 방 생성, 입장/퇴장 |
| `include/lobby/handlers/ChatHandler.h` | 로비 채팅 브로드캐스트 |
| `include/lobby/handlers/GameHandler.h` | 게임 시작 요청, DS로 전달 |
| `include/lobby/domain/Room.h` | 방(Room) 도메인 객체 |
| `include/lobby/domain/RoomManager.h` | 방 목록 관리, 방 코드 생성 |
| `include/lobby/domain/Player.h` | 플레이어 도메인 객체 |
| `include/lobby/LobbyClientManager.h` | 연결된 클라이언트 WebSocket 세션 관리 |
| `include/backend/BackendService.h` | DS(UE 전용 서버)와의 백엔드 WebSocket 통신 채널 |
| `include/network/NetworkService.h` | 플랫폼 추상화 네트워크 I/O |
| `include/network/ConnectionRegistry.h` | 소켓 연결 레지스트리 |
| `include/network/platform/NetworkEventLoopLinux.h` | Linux epoll 이벤트 루프 구현 |
| `include/network/platform/NetworkEventLoopWin.h` | Windows IOCP 이벤트 루프 구현 |
| `include/common/ConcurrentQueue.h` | 락-프리 스레드 안전 큐 (VoiceWorker 용) |
| `include/common/Logger.h` | 파일/콘솔 로깅 |

### 음성 서버 (UDP, 포트 48100)

| 경로 | 역할 |
|------|------|
| `include/voice/VoiceServer.h` | UDP 기반 음성 서버. 워커 스레드 2개로 패킷 처리 |
| `include/voice/VoicePacketRouter.h` | Join / VoiceData / Leave 패킷 분배 |
| `include/voice/VoiceClientManager.h` | 방 코드별 음성 클라이언트 세션 관리 |
| `include/voice/VoicePacketType.h` | 패킷 타입 열거형 정의 |
| `include/voice/VoiceConstants.h` | 음성 서버 상수 (버퍼 크기 등) |

---

## 전용 서버 (Dedicated Server, DS)

경로: `A302/Source/A302Server/`

Unreal Engine으로 구동되는 게임 서버입니다. 실제 게임 월드를 호스팅하며, 클라이언트는 UE 표준 복제(Replication) 채널로 이 서버에 접속합니다.

### GameMode

| 파일 | 역할 |
|------|------|
| `GameMode/A302GameMode.h/.cpp` | 인게임 전용 서버 메인 GameMode. 플레이어 스폰, 방 게임 시작, 백엔드 메시지 수신/처리 |
| `GameMode/LobbyGameMode.h/.cpp` | 대기실(로비) 맵용 GameMode |

### 네트워크 (DS ↔ 로비 서버 백엔드 통신)

| 파일 | 역할 |
|------|------|
| `Network/GameServerBackendSubsystem.h/.cpp` | DS가 C++ 로비 서버와 WebSocket으로 통신하는 서브시스템. 자신을 `A302-Dedicated`로 등록 |
| `Network/A302ServerBackendRouter.h/.cpp` | 백엔드로부터 수신한 메시지를 파싱하여 GameMode 등으로 라우팅 |

### 페이즈 관리

| 파일 | 역할 |
|------|------|
| `Phase/A302ServerPhaseSubsystem.h/.cpp` | Phase0→Phase1→Phase2→Ended 전환 타이머 관리. 방(Room)별 독립 페이즈 상태 유지. 조건 달성 시 다음 페이즈로 자동 전환 및 GameState 복제 |

### 방(Room) 런타임

| 파일 | 역할 |
|------|------|
| `Room/A302RoomRuntimeSubsystem.h/.cpp` | 방별 Level Streaming 인스턴스 생성·관리. 방 오프셋(World Offset)으로 동일 서버에서 다중 방 동시 호스팅 지원 |
| `Room/RoomMembershipRegistry.h/.cpp` | 어떤 플레이어가 어떤 방에 속하는지 매핑 관리 |

### 아이템 스폰

| 파일 | 역할 |
|------|------|
| `Subsystem/A302ItemSpawnSubsystem.h/.cpp` | 방별 아이템 스폰 오케스트레이션 서브시스템 |
| `ItemSpawn/A302ItemSpawnOrchestrator.h/.cpp` | 스폰 정책에 따라 아이템 스폰 위치·타이밍 결정 |
| `Manager/SpawnManager.h/.cpp` | SpawnArea 기반 실제 아이템 액터 스폰 처리 |

### 이벤트 시스템

| 파일 | 역할 |
|------|------|
| `Subsystem/A302RoomEventSubsystem.h/.cpp` | 방별 이벤트 진행 상태 관리 |
| `Reward/A302ServerEventResolver.h/.cpp` | 이벤트 결과에 따른 보상 계산 및 배분 |

#### 개인 이벤트 (PersonalEvents)

| 파일 | 설명 |
|------|------|
| `PersonalEventMalice` | 악의(Malice) 수치 관련 이벤트 |
| `PersonalEventMaliceOverload` | 악의 과부하 이벤트 |
| `PersonalEventPublicMalice` | 악의 공개 선언 이벤트 |
| `PersonalEventAnothersPortrait` | 타인의 초상 이벤트 |
| `PersonalEventInspectMalice` | 악의 검사 이벤트 |
| `PersonalEventCursedSword` | 저주받은 검 장착 이벤트 |
| `PersonalEventBizarreForge` | 기묘한 단조 이벤트 |
| `PersonalEventPhase1Collect` | 페이즈1 수집 이벤트 |
| `PersonalEventDevilsEye` | 악마의 눈 상태 이벤트 |
| `PersonalEventPurifyingFlame` | 정화의 불꽃 이벤트 |

#### 그룹 이벤트 (GroupEvents)

| 파일 | 설명 |
|------|------|
| `GroupEventConfiscate` | 몰수 이벤트 |
| `GroupEventJudgment` | 심판 이벤트 |

### 플레이어 서브시스템

| 파일 | 역할 |
|------|------|
| `Subsystem/A302ServerPlayerSubsystem.h/.cpp` | 서버 측 플레이어 상태 및 세션 관리 |

---

## 클라이언트 (Client)

경로: `A302/Source/A302Client/`

플레이어가 실행하는 클라이언트 전용 모듈입니다. `A302Shared` 모듈에 의존하며 UI, 음성, 네트워크 연결 처리를 담당합니다.

### 네트워크

| 파일 | 역할 |
|------|------|
| `Network/GameNetworkSubsystem.h/.cpp` | 클라이언트 네트워크 통합 서브시스템. WebSocket(로비 연결)과 UDP(음성) 두 채널을 통합 관리 |
| `Network/ConnectionHandler.h/.cpp` | WebSocket 연결 수명 주기 관리 |
| `Network/UDPHandler.h/.cpp` | UDP 소켓 송수신 처리 |

### UI

| 파일 | 역할 |
|------|------|
| `UI/A302GameHUD.h/.cpp` | 인게임 HUD 루트 |
| `UI/PlayerHUDComponent.h/.cpp` | 체력·아이템·타이머 등 플레이어 HUD 컴포넌트 |
| `UI/GroupEventHUDComponent.h/.cpp` | 그룹 이벤트 상태 HUD |
| `UI/LobbyWidget.h/.cpp` | 로비 메인 위젯 |
| `UI/WaitingRoomWidget.h/.cpp` | 방 대기실 위젯 |
| `UI/RoomListPopup.h/.cpp` | 방 목록 팝업 |
| `UI/RoomListItem.h/.cpp` | 방 목록 항목 위젯 |
| `UI/EnterRoomPopup.h/.cpp` | 방 입장 팝업 |
| `UI/PlayerListItem.h/.cpp` | 대기실 플레이어 목록 항목 |
| `UI/ChatWidget.h/.cpp` | 채팅 위젯 |
| `UI/ChatMessageItem.h/.cpp` | 채팅 메시지 단일 항목 |
| `UI/IntroSplashWidget.h/.cpp` | 인트로/스플래시 화면 |
| `UI/IntroSplashLocalPlayerSubsystem.h/.cpp` | 인트로 스플래시 로컬 플레이어 서브시스템 |
| `UI/EscapeWaitingWidget.h/.cpp` | 탈출 대기 화면 위젯 |

### 음성 채팅

클라이언트의 음성 시스템은 Strategy 패턴으로 설계되어 로비(전체 방송)와 인게임(거리 기반)을 동적으로 전환합니다.

| 파일 | 역할 |
|------|------|
| `Voice/PrivateVoiceChatComponent.h/.cpp` | 음성 채팅 최상위 컴포넌트. 전략 교체 담당 |
| `Voice/Strategy/VoiceChatStrategyBase.h/.cpp` | 전략 기반 클래스 |
| `Voice/Strategy/LobbyVoiceChatStrategy.h/.cpp` | 로비용 전략 — 방 전체 브로드캐스트 |
| `Voice/Strategy/DistanceVoiceChatStrategy.h/.cpp` | 인게임용 전략 — 거리 기반 음성 믹싱 |
| `Voice/Capture/VoiceCaptureProcessor.h/.cpp` | 마이크 캡처 및 전처리 |
| `Voice/Codec/VoiceCodec.h/.cpp` | Opus 코덱 인코딩/디코딩 |
| `Voice/Network/VoiceNetworkClient.h/.cpp` | 음성 데이터 UDP 송신 |
| `Voice/Playback/VoiceAudioReceiver.h/.cpp` | 수신 음성 데이터 재생 |
| `Voice/Profiling/VoiceProfiler.h/.cpp` | 음성 성능 프로파일링 |

### GameMode

| 파일 | 역할 |
|------|------|
| `GameMode/LobbyMessageRouter.h/.cpp` | 로비에서 WebSocket으로 수신된 메시지를 UI 이벤트로 변환 |

---

## 공유 코드 (Shared)

경로: `A302/Source/A302Shared/`

DS와 클라이언트 **양쪽 모두에서 컴파일·실행**되는 공통 코드입니다. Unreal Engine 복제(Replication) 시스템과 연동되는 게임플레이 핵심 클래스들이 여기 위치합니다.

### 네트워크 공통

| 파일 | 역할 |
|------|------|
| `Network/A302WebSocketClient.h/.cpp` | WebSocket 클라이언트 래퍼. DS(백엔드 연결)와 클라이언트(로비 연결) 양쪽에서 사용 |
| `Network/A302NetworkEndpointConfig.h/.cpp` | 로비(8001), 음성(48100), DS(47777) 포트 및 호스트 설정. Local/PublicEc2 프리셋 전환 |
| `Network/LobbyConstants.h` | 로비 패킷 도메인/타입 상수 |

### 캐릭터

| 파일 | 역할 |
|------|------|
| `Character/MyCharacter.h/.cpp` | 메인 플레이어 캐릭터. 입력 처리(이동·점프·상호작용·아이템), UE 복제 RPC 정의 |
| `Character/MyPlayerController.h/.cpp` | 플레이어 컨트롤러 |

#### 캐릭터 컴포넌트

| 파일 | 역할 | 실행 환경 |
|------|------|-----------|
| `Components/Combat/CharacterHealthComponent` | 체력 관리, 사망 판정 | DS + Client |
| `Components/Combat/CombatStatusComponent` | 전투 상태(공격 중·피격 등) | DS + Client |
| `Components/Combat/EquipmentComponent` | 무기 장착 상태 | DS + Client |
| `Components/Inventory/ItemManagerComponent` | 아이템 인벤토리 관리 | DS + Client |
| `Components/Inventory/QuickSlotComponent` | 퀵슬롯(1~6번) 관리 | DS + Client |
| `Components/Inventory/ItemTargetingComponent` | 아이템 사용 대상 지정 | DS + Client |
| `Components/Interaction/InteractComponent` | 상호작용(G키) 처리, QTE | DS + Client |
| `Components/Interaction/CharacterActionInputComponent` | 입력 액션 처리 | 주로 Client |
| `Components/Interaction/CharacterRewardComponent` | 보상 수령 처리 | DS + Client |
| `Components/Effects/ItemEffectComponent` | 아이템 효과 적용 | DS + Client |
| `Components/MaliceComponent` | 악의(Malice) 수치 관리 | DS + Client |
| `Components/PlayerEventComponent` | 플레이어별 이벤트 상태 | DS + Client |
| `Components/Dance/DanceComponent` | 댄스 애니메이션 트리거 | DS + Client |
| `Components/Audio/GameBGMComponent` | 게임 BGM 재생 | Client |
| `Components/Audio/CursedSwordBGMComponent` | 저주받은 검 BGM | Client |
| `Components/Audio/MaliceBGMComponent` | 악의 상태 BGM | Client |

### 게임 상태·인스턴스

| 파일 | 역할 |
|------|------|
| `GameMode/A302GameState.h/.cpp` | 복제되는 게임 상태. `EGamePhase`(Phase0/1/2/Ended), 매치 타이머, 생존 플레이어 수 등을 클라이언트로 복제 |
| `GameMode/A302PlayerState.h/.cpp` | 플레이어별 복제 상태 (닉네임 등) |
| `GameMode/A302GameInstance.h/.cpp` | 게임 인스턴스 |

### 게임플레이 오브젝트

| 파일 | 역할 |
|------|------|
| `Object/BaseInteractable.h/.cpp` | 상호작용 가능한 오브젝트 기반 클래스 |
| `Object/StatueInteractable.h/.cpp` | 석상 오브젝트. 페이즈2 클리어 조건 |
| `Object/ItemSpawnArea.h/.cpp` | 아이템 스폰 구역 마커 |
| `Object/SpawnArea.h/.cpp` | 캐릭터 스폰 구역 |
| `Object/PhaseSpawnPoint.h/.cpp` | 페이즈별 스폰 포인트 |
| `Object/EscapeRouteBlocker.h/.cpp` | 탈출구 차단 오브젝트. 페이즈 조건 완료 시 해제 |
| `Object/VFX/InteractableVFXData.h/.cpp` | 상호작용 오브젝트 VFX 데이터 |

### 아이템 시스템

| 파일 | 역할 |
|------|------|
| `GamePlay/Items/BaseItem.h/.cpp` | 아이템 기반 클래스. `OnItemAcquired`, `OnItemUsed`, `ResolveServerTargetedUse` 정의 |
| `GamePlay/Items/ItemKnife` | 단검 — 대상에게 피해 |
| `GamePlay/Items/ItemShield` | 방패 — 피해 방어 |
| `GamePlay/Items/ItemCursedSword` | 저주받은 검 — 특수 공격 |
| `GamePlay/Items/ItemMaliciousShield` | 악의의 방패 |
| `GamePlay/Items/ItemMaliciousSword` | 악의의 검 |
| `GamePlay/Items/ItemOminousMirror` | 불길한 거울 |
| `GamePlay/Items/ItemSereneLantern` | 고요한 등불 |
| `GamePlay/Items/ItemCrimsonQuartz` | 진홍 수정 |
| `GamePlay/Actor/WeaponActor` | 무기 액터 기반 클래스 |
| `GamePlay/Actor/KnifeActor` | 단검 액터 |
| `GamePlay/Actor/ShieldActor` | 방패 액터 |
| `GamePlay/Actor/TimeKnifeActor` | 타임 단검 액터 |
| `GamePlay/Factories/ItemActionFactory` | 아이템 액션 팩토리 |

### 이벤트 기반 클래스

| 파일 | 역할 |
|------|------|
| `GamePlay/Events/BaseEvent.h/.cpp` | 모든 이벤트의 기반 클래스 |
| `GamePlay/Events/GroupEvents/BaseGroupEvent.h/.cpp` | 그룹 이벤트 기반 클래스 |
| `GamePlay/Events/PersonalEvents/BasePersonalEvent.h/.cpp` | 개인 이벤트 기반 클래스 |

### 데이터 정의 (GameData)

`GameData/` 폴더에는 이벤트·아이템·보상에 대한 데이터 구조체 정의(`*Definition.h`)가 위치합니다. 이 파일들은 헤더 전용으로, DS와 클라이언트 양쪽에서 공유됩니다.

| 파일 | 역할 |
|------|------|
| `Items/ItemDefinition.h` | 아이템 정의 데이터 에셋 |
| `Items/ItemInstance.h` | 아이템 인스턴스 (런타임 상태) |
| `Items/ItemTypes.h` | 아이템 타입 열거형 |
| `RewardDefinition.h` | 보상 정의 |
| `RewardTypes.h` | 보상 타입 열거형 |
| `StageRewardPoolDefinition.h` | 스테이지 보상 풀 정의 |
| `Spawn/ItemSpawnPolicy.h/.cpp` | 아이템 스폰 정책 |

### 애니메이션

| 파일 | 역할 |
|------|------|
| `Animation/MyAnimInstance.h/.cpp` | 캐릭터 애니메이션 인스턴스 |
| `Animation/AnimNotify/ShowWeapon` | 무기 표시 애니메이션 노티파이 |
| `Animation/AnimNotify/HideWeapon` | 무기 숨김 애니메이션 노티파이 |
| `Animation/AnimNotify/PlaySound` | 사운드 재생 노티파이 |
| `Animation/AnimNotify/SpawnVFX` | VFX 스폰 노티파이 |

### 인터페이스

| 파일 | 역할 |
|------|------|
| `Interface/InteractableInterface.h/.cpp` | 상호작용 가능 오브젝트 인터페이스 |
| `Interface/UsableItem.h` | 사용 가능한 아이템 인터페이스 |
| `Public/Interface/A302AnimationBridge.h` | 애니메이션 ↔ 게임로직 브릿지 |
| `Public/Interface/A302ServerRewardBridge.h` | 서버 보상 처리 브릿지 |
| `Public/Interface/A302TimedKillEventBridge.h` | 타이머 킬 이벤트 브릿지 |

### 공통 유틸

| 파일 | 역할 |
|------|------|
| `Public/A302GameplayGuards.h` | 게임플레이 실행 가드 (DS 전용 로직 보호) |
| `Public/A302RuntimeGuards.h` | 런타임 조건 검사 매크로 |
| `Public/Room/RoomScopeRules.h` | 방 스코프 규칙 |
| `Public/Room/RoomWorldOffset.h` | 방 월드 오프셋 계산 |

---

## 네트워크 포트 정보

| 포트 | 프로토콜 | 용도 |
|------|----------|------|
| **8001** | WebSocket (TCP) | C++ 로비 서버 (방 목록·채팅·게임 시작) |
| **48100** | UDP | C++ 음성 서버 (Opus 코덱 실시간 음성) |
| **47777** | UE Networking (UDP) | Unreal Engine 전용 서버 (게임 월드·복제) |

---

## 게임 흐름 및 페이즈

```
[로비 접속] → WebSocket(8001)으로 C++ 로비 서버 연결
     │
     ▼
[방 생성 / 방 입장] → 대기실에서 다른 플레이어 대기
     │
     ▼
[게임 시작] → 로비 서버가 DS로 게임 시작 신호 전송
              DS가 플레이어를 게임 맵으로 스폰
     │
     ▼
┌─── Phase 0 ───────────────────────────────────────┐
│  아이템 수집 단계                                   │
│  - 맵에 아이템 스폰                                 │
│  - Phase0RequiredItemCount 달성 시 Phase1 전환      │
└────────────────────────────────────────────────────┘
     │
     ▼
┌─── Phase 1 ───────────────────────────────────────┐
│  오브젝트 클리어 단계                               │
│  - 특정 오브젝트 상호작용 필요                       │
│  - Phase1RequiredClearObjectCount 달성 시 Phase2   │
└────────────────────────────────────────────────────┘
     │
     ▼
┌─── Phase 2 ───────────────────────────────────────┐
│  석상 그룹 이벤트 단계                              │
│  - 석상(StatueInteractable) 협동 조작               │
│  - 모든 석상 완료 → EscapeRouteBlocker 해제          │
│  - 탈출구 개방 후 포탈 통해 탈출                     │
└────────────────────────────────────────────────────┘
     │
     ▼
[Ended] → 결과 화면, 보상 지급
```

페이즈는 `A302ServerPhaseSubsystem`이 0.25초 간격으로 폴링하며 조건 달성 여부를 판단합니다. 페이즈 변경 시 `A302GameState`를 통해 모든 클라이언트로 복제되며, 클라이언트는 `OnRep_GamePhase`를 통해 UI를 갱신합니다. 전체 매치 제한 시간은 기본 666초이며, 시간 초과 시 탈출 없이 종료됩니다.

---

## 아이템 목록

| 아이템 | 설명 |
|--------|------|
| 단검 (Knife) | 대상 플레이어에게 피해를 입힘 |
| 방패 (Shield) | 피해를 방어 |
| 저주받은 검 (Cursed Sword) | 특수 공격 아이템, 전용 BGM 재생 |
| 악의의 검 (Malicious Sword) | 악의 수치와 연동된 검 |
| 악의의 방패 (Malicious Shield) | 악의 수치와 연동된 방패 |
| 불길한 거울 (Ominous Mirror) | 특수 효과 아이템 |
| 고요한 등불 (Serene Lantern) | 특수 효과 아이템 |
| 진홍 수정 (Crimson Quartz) | 특수 효과 아이템 |

---

## 조작법

| 키 | 행동 |
|----|------|
| `W` `A` `S` `D` | 이동 |
| `Space` | 점프 |
| `F` | 상호작용 (오브젝트·석상 등) |
| `1` ~ `6` | 퀵슬롯 아이템 선택 (활성화) |
| `마우스 좌클릭` | 선택한 아이템 사용 |

---

## 프로젝트 구조 요약

```
S14P21A302/
├── README.md                    # 커밋·브랜치 컨벤션 문서
├── README_MAUSOLEUM.md          # 본 문서
│
├── Server/                      # C++ 독립 서버
│   └── GameServer/
│       ├── include/
│       │   ├── websocket/       # WebSocket 서버 (uWebSockets)
│       │   ├── lobby/           # 로비 서비스 (방·채팅·게임시작)
│       │   ├── voice/           # UDP 음성 서버
│       │   ├── network/         # 플랫폼별 네트워크 I/O
│       │   ├── backend/         # DS 연동 백엔드 서비스
│       │   └── common/          # 공통 유틸 (큐·로거·타이머)
│       └── src/                 # 구현 파일
│
└── A302/                        # Unreal Engine 5 프로젝트
    └── Source/
        ├── A302Shared/          # ★ DS + 클라이언트 공유 코드
        │   ├── Character/       # 캐릭터·컴포넌트
        │   ├── GameMode/        # GameState·PlayerState·GameInstance
        │   ├── GamePlay/        # 아이템·이벤트·액터
        │   ├── GameData/        # 데이터 정의 구조체
        │   ├── Network/         # WebSocket 클라이언트·엔드포인트 설정
        │   ├── Object/          # 상호작용 오브젝트
        │   ├── Animation/       # 애니메이션 인스턴스·노티파이
        │   ├── Interface/       # C++ 인터페이스
        │   ├── UI/              # 공유 UI 위젯
        │   └── Public/          # 공개 헤더·가드
        │
        ├── A302Server/          # ★ DS 전용 코드
        │   ├── GameMode/        # A302GameMode·LobbyGameMode
        │   ├── Network/         # 백엔드 서브시스템·라우터
        │   ├── Phase/           # 페이즈 관리 서브시스템
        │   ├── Room/            # 방 런타임·멤버십 레지스트리
        │   ├── ItemSpawn/       # 아이템 스폰 오케스트레이터
        │   ├── Manager/         # SpawnManager
        │   ├── Subsystem/       # 아이템스폰·이벤트·플레이어 서브시스템
        │   ├── Reward/          # 이벤트 리졸버
        │   └── GamePlay/Events/ # 개인·그룹 이벤트 구현
        │
        └── A302Client/          # ★ 클라이언트 전용 코드
            ├── Network/         # GameNetworkSubsystem·UDP·연결
            ├── UI/              # HUD·로비·대기실·채팅 위젯
            ├── Voice/           # 음성 채팅 (캡처·코덱·전략·재생)
            └── GameMode/        # LobbyMessageRouter
```
