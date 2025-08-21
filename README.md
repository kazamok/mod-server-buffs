# Server Buffs Module (mod-server-buffs)

## 1. 모듈 소개

`mod-server-buffs`는 아제로스코어(AzerothCore) 월드 서버에서 특정 시간에 모든 온라인 플레이어에게 이로운 버프(주문)를 자동으로 적용하는 모듈입니다. 이 모듈은 서버 운영자가 플레이어들에게 주기적인 혜택을 제공하거나, 특정 이벤트에 맞춰 버프를 부여하는 등 다양한 방식으로 활용될 수 있습니다.

이 모듈은 아제로스코어의 핵심 파일을 수정하지 않는 **완전한 플러그인 형태**로 제작되어, 기존 서버 환경에 영향을 주지 않으면서 독립적으로 작동하도록 설계되었습니다.

## 2. 주요 기능

*   **시간 기반 버프 적용**: 설정 파일에 지정된 시간에 자동으로 모든 온라인 플레이어에게 버프를 적용합니다.
*   **다양한 버프 지원**: 여러 종류의 버프 주문 ID를 설정하여 동시에 적용할 수 있습니다.
*   **중복 적용 방지**: 동일한 버프가 하루에 여러 번 중복 적용되는 것을 방지하는 로직이 포함되어 있습니다.
*   **서버 전역 안내 메시지**: 버프 적용 시 서버 전역에 안내 메시지를 브로드캐스트하여 플레이어들에게 버프 적용 사실을 알립니다.
*   **독립적인 설정**: `worldserver.conf` 파일을 직접 수정하지 않고, 모듈 자체의 설정 파일(`mod-server-buffs.conf.dist`)을 통해 모듈의 활성화 여부, 안내 메시지, 버프 주문, 적용 시간 등을 유연하게 제어할 수 있습니다.
*   **콘솔 로깅**: 버프 적용 과정 및 발생 가능한 오류(예: 유효하지 않은 주문 ID)를 서버 콘솔에 상세하게 기록하여 운영자가 모듈의 작동 상태를 쉽게 파악할 수 있도록 돕습니다.

## 3. 설치 방법

1.  **모듈 파일 배치**: 이 모듈의 모든 파일을 아제로스코어 소스 코드의 `modules/mod-server-buffs/` 경로에 배치합니다.
    (예시: `C:/azerothcore/modules/mod-server-buffs/`)

2.  **아제로스코어 빌드**: 아제로스코어 프로젝트를 다시 빌드합니다. 이 과정에서 `mod-server-buffs` 모듈이 함께 컴파일되고, 필요한 설정 파일이 설치 경로로 복사됩니다.
    (빌드 과정은 사용자의 환경에 따라 다를 수 있습니다. 일반적으로 `cmake --build .` 명령을 사용합니다.)

3.  **설정 파일 준비**: 빌드 완료 후, 아제로스코어 설치 디렉토리의 `configs/modules/` 폴더에 `mod-server-buffs.conf.dist` 파일이 생성됩니다. 이 파일은 모듈의 설정 파일로 사용됩니다.

    *   **설정 파일 경로 예시**: `C:/BUILD/bin/RelWithDebInfo/configs/modules/mod-server-buffs.conf.dist`

## 4. 설정 (Configuration)

`mod-server-buffs.conf.dist` 파일에서 다음 옵션들을 설정할 수 있습니다.

```ini
#----------------------------------------------------------
# Server Buffs Module Settings
#----------------------------------------------------------
#
# Enable Server Buffs Module (0 = disabled, 1 = enabled)
ServerBuffs.Enable = 1

# Message to display when buffs are applied server-wide
ServerBuffs.AnnounceMessage = "서버 전역 버프가 적용되었습니다!"

# Show announce message (0 = disabled, 1 = enabled)
ServerBuffs.ShowAnnounceMessage = 1

# List of spell IDs to apply as buffs (comma-separated)
# Example: 25809 (Arcane Intellect), 25814 (Mark of the Wild), 25818 (Prayer of Fortitude)
ServerBuffs.BuffSpells = 25809,25814,25818

# List of times to cast buffs (HH:MM format, comma-separated)
# Example: 09:00,12:00,18:00,21:00
ServerBuffs.CastTimes = 09:00,12:00,18:00,21:00
```

*   `ServerBuffs.Enable`: 모듈의 기능을 전체적으로 켜거나 끕니다. 기본값은 `1` (활성화)입니다.
*   `ServerBuffs.AnnounceMessage`: 버프 적용 시 서버 전역에 표시될 메시지를 설정합니다.
*   `ServerBuffs.ShowAnnounceMessage`: `ServerBuffs.AnnounceMessage`의 표시 여부를 설정합니다. 기본값은 `1` (표시)입니다.
*   `ServerBuffs.BuffSpells`: 플레이어에게 적용할 버프 주문의 ID 목록을 쉼표(`,`)로 구분하여 입력합니다. 유효한 주문 ID를 사용해야 합니다.
*   `ServerBuffs.CastTimes`: 버프를 적용할 시간을 `HH:MM` 형식으로 쉼표(`,`)로 구분하여 입력합니다. (예: `09:00,12:00,18:00`) 서버 시간 기준으로 해당 분이 되면 버프가 적용됩니다.

## 5. 사용 방법

1.  **월드 서버 실행**: 설정이 완료된 후 월드 서버를 실행합니다.

2.  **버프 적용 확인**: `ServerBuffs.CastTimes`에 설정된 시간이 되면, 접속 중인 모든 플레이어에게 `ServerBuffs.BuffSpells`에 지정된 버프가 적용됩니다. `ServerBuffs.ShowAnnounceMessage`가 활성화되어 있다면, 서버 전역 메시지를 통해 버프 적용 사실이 안내됩니다.

3.  **콘솔 로그 확인**: 월드 서버 콘솔에서 모듈의 작동 상태, 버프 적용 성공/실패 여부, 유효하지 않은 주문 ID 등의 정보를 확인할 수 있습니다.

## 6. 향후 개선 사항

*   버프 적용 실패 시 상세한 원인 로깅 (예: 플레이어 상태, 주문 조건 불일치 등).
*   특정 지역 또는 특정 레벨 범위의 플레이어에게만 버프를 적용하는 기능.
*   버프 적용 후 재사용 대기시간 설정 (예: 한 번 버프를 받은 플레이어는 일정 시간 동안 다시 받지 않음).
*   인게임 명령어를 통한 버프 수동 적용 또는 설정 변경 기능.

## 👥 크레딧
- Kazamok
- Gemini
- 모든 기여자들

## 📄 라이선스

이 프로젝트는 GPL-3.0 라이선스 하에 배포됩니다.
