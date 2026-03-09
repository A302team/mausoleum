# GameServer (C++ WebSocket & UDP Server)

이 디렉토리에는 로비 기능(WebSockets)과 음성 채팅(UDP & Protobuf)을 처리하는 C++ GameServer가 포함되어 있습니다.

## 초기 설정 및 빌드 가이드

팀원들의 로컬 개발 및 설정 편의를 위해 `vcpkg` 매니페스트 모드(`vcpkg.json`)를 사용하여 C++ 종속성(`uwebsockets`, `nlohmann-json`, `protobuf`)을 관리하고 있습니다.

### Windows 설정 및 빌드

1. 로컬 환경에 `vcpkg`가 설치되어 있지 않다면 전역으로 설치합니다:

   ```cmd
   git clone https://github.com/microsoft/vcpkg.git
   .\vcpkg\bootstrap-vcpkg.bat
   setx VCPKG_ROOT "C:\본인PC의\vcpkg\경로"
   ```

   > ⚠️ `VCPKG_ROOT` 환경 변수를 설정한 후 터미널(CMD/PowerShell)을 재시작해야 적용될 수 있습니다.

2. `Server` 디렉토리(이 폴더) 내에 있는 `Setup.bat` 파일을 더블클릭하거나 터미널에서 실행합니다.
   - 내부적으로 CMake를 실행하고 매니페스트 모드를 통해 `GameServer/vcpkg.json`에 정의된 종속 라이브러리를 자동으로 가져와 프로젝트를 세팅합니다.

3. `Setup.bat`이 성공적으로 끝난 후, 다음 명령어를 통해 프로젝트를 빌드합니다.

   ```cmd
   cd GameServer/build
   cmake --build . --config Release
   ```

4. 빌드 완료 후 실행:
   ```cmd
   Release\Server.exe
   ```

## 종속성 (Dependencies)

- `uwebsockets` (로비 통신 / WebSockets)
- `nlohmann-json` (JSON 파싱 로직)
- `protobuf` (음성 데이터용 구조화 포맷)

이 항목들은 CMake Configure 시(즉, `Setup.bat` 실행 시) 매니페스트 모드를 기반으로 `vcpkg`를 통해 자동으로 설치됩니다. 별도의 다운로드나 빌드가 필요없습니다.
