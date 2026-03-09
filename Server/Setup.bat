@echo off
setlocal

cd /d "%~dp0"

echo =========================================
echo GameServer 설정 및 자동 빌드 스크립트
echo =========================================

:: 1. VCPKG_ROOT 환경변수 확인
if "%VCPKG_ROOT%"=="" (
    echo [에러] VCPKG_ROOT 환경 변수가 설정되어 있지 않습니다.
    echo 시스템 전역으로 vcpkg를 설치하고 VCPKG_ROOT 환경 변수로 지정해 주세요.
    echo 예시: setx VCPKG_ROOT "C:\path\to\vcpkg"
    pause
    exit /b 1
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [에러] %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake 파일을 찾을 수 없습니다.
    echo VCPKG_ROOT가 유효한 vcpkg 경로를 가리키고 있는지 확인해 주세요.
    pause
    exit /b 1
)

:: 2. CMake와 vcpkg 툴체인을 사용하여 GameServer 구성
echo [안내] CMake 프로젝트를 구성하는 중...
cd GameServer
if exist "build" (
    echo [안내] 이전 빌드 캐시 초기화 중...
    rd /s /q build
)
mkdir build
cd build

:: 참고: GameServer 폴더 안에 vcpkg.json이 있으므로 자동으로 매니페스트 모드가 작동합니다.
cmake .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" -A x64

echo [안내] CMake 구성이 완료되었습니다.
echo 프로젝트를 빌드하려면 다음 명령어를 실행하세요: cmake --build . --config Release
echo =========================================
pause
