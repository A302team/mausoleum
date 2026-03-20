param(
    [int]$GamePort = 47777
)

$ErrorActionPreference = "SilentlyContinue"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$LobbyExeCandidates = @(
    (Join-Path $ProjectRoot "..\Server\build\Server.exe"),
    (Join-Path $ProjectRoot "..\Server\GameServer\build\Release\Server.exe"),
    (Join-Path $ProjectRoot "..\Server\GameServer\build\Server.exe")
)

$LobbyExePaths = @()
foreach ($candidate in $LobbyExeCandidates) {
    $resolved = Resolve-Path $candidate -ErrorAction SilentlyContinue
    if ($resolved) {
        $LobbyExePaths += $resolved.Path.ToLower()
    }
    else {
        $LobbyExePaths += $candidate.ToLower()
    }
}
$LobbyExePaths = @($LobbyExePaths | Select-Object -Unique)

$A302ServerExePaths = @(
    (Join-Path $ProjectRoot "Binaries\Win64\A302Server-Win64-Development.exe").ToLower(),
    (Join-Path $ProjectRoot "Binaries\Win64\A302Server.exe").ToLower(),
    (Join-Path $ProjectRoot "Saved\StagedBuilds\WindowsServer\A302Server.exe").ToLower(),
    (Join-Path $ProjectRoot "Saved\StagedBuilds\WindowsServer\A302\Binaries\Win64\A302Server.exe").ToLower(),
    (Join-Path (Split-Path $ProjectRoot -Parent) "Artifacts\WindowsServer\A302Server.exe").ToLower(),
    (Join-Path (Split-Path $ProjectRoot -Parent) "Artifacts\WindowsServer\A302\Binaries\Win64\A302Server.exe").ToLower()
)
$UProjectPath = (Join-Path $ProjectRoot "A302.uproject").ToLower()

$Processes = Get-CimInstance Win32_Process

# 1) Stop lobby server process
$LobbyProcesses = $Processes | Where-Object {
    $_.Name -ieq "Server.exe" -and
    $_.ExecutablePath -and
    $LobbyExePaths -contains $_.ExecutablePath.ToLower()
}

foreach ($p in $LobbyProcesses) {
    Write-Host "[LOCAL-TEST] Stopping lobby server PID=$($p.ProcessId)"
    Stop-Process -Id $p.ProcessId -Force
}

# 2) Stop packaged/server target process if present
$DedicatedProcesses = $Processes | Where-Object {
    $_.Name -ieq "A302Server-Win64-Development.exe" -or
    $_.Name -ieq "A302Server.exe" -or
    (
        $_.ExecutablePath -and
        $A302ServerExePaths -contains $_.ExecutablePath.ToLower()
    )
}

foreach ($p in $DedicatedProcesses) {
    Write-Host "[LOCAL-TEST] Stopping A302Server process PID=$($p.ProcessId)"
    Stop-Process -Id $p.ProcessId -Force
}

# 3) Stop UnrealEditor-based dedicated server process only
$EditorServerProcesses = $Processes | Where-Object {
    $_.Name -ieq "UnrealEditor.exe" -and
    $_.CommandLine -and
    $_.CommandLine.ToLower().Contains($UProjectPath) -and
    $_.CommandLine.Contains("-server") -and
    $_.CommandLine.Contains("-port=$GamePort")
}

foreach ($p in $EditorServerProcesses) {
    Write-Host "[LOCAL-TEST] Stopping UnrealEditor dedicated server PID=$($p.ProcessId)"
    Stop-Process -Id $p.ProcessId -Force
}

Write-Host "[LOCAL-TEST] Stop complete."
