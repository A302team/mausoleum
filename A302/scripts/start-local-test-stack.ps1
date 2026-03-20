param(
    [string]$EngineRoot = "C:\UnrealEngine",
    [string]$Map = "/Game/PersonalWorkSpace/wjtmd28/MyMap",
    [int]$GamePort = 47777,
    [switch]$SkipLobby,
    [switch]$SkipGameServer,
    [switch]$SkipStopExisting,
    [switch]$SkipCloseEditor,
    [switch]$ForceRebuildStaged,
    [switch]$CookAllMaps,
    [string]$ArchiveDirectory = ""
)

$ErrorActionPreference = "Stop"

function Resolve-StagedServerExe {
    param(
        [Parameter(Mandatory = $true)][string]$ArchiveRoot,
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$ProjectName
    )

    $candidates = @(
        (Join-Path $ArchiveRoot "WindowsServer\A302Server.exe"),
        (Join-Path $ArchiveRoot "WindowsServer\A302Server-Win64-Development.exe"),
        (Join-Path $ArchiveRoot "WindowsServer\$ProjectName\Binaries\Win64\A302Server.exe"),
        (Join-Path $ArchiveRoot "WindowsServer\$ProjectName\Binaries\Win64\A302Server-Win64-Development.exe"),
        (Join-Path $ProjectRoot "Saved\StagedBuilds\WindowsServer\A302Server.exe"),
        (Join-Path $ProjectRoot "Saved\StagedBuilds\WindowsServer\$ProjectName\Binaries\Win64\A302Server.exe"),
        (Join-Path $ProjectRoot "Saved\StagedBuilds\WindowsServer\$ProjectName\Binaries\Win64\A302Server-Win64-Development.exe")
    )

    return $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

function Stop-StaleUnrealBuildProcesses {
    param(
        [Parameter(Mandatory = $true)][string]$EngineDir
    )

    $targets = Get-Process -Name "UnrealBuildTool", "dotnet" -ErrorAction SilentlyContinue
    if (-not $targets) {
        return
    }

    $stopped = @()
    foreach ($proc in $targets) {
        if ($proc.Id -eq $PID) {
            continue
        }

        $shouldStop = $false
        if ($proc.ProcessName -eq "UnrealBuildTool") {
            $shouldStop = $true
        }
        elseif ($proc.ProcessName -eq "dotnet") {
            $procPath = $null
            try {
                $procPath = $proc.Path
            }
            catch {
                $procPath = $null
            }

            if ($procPath -and $procPath -like "$EngineDir*") {
                $shouldStop = $true
            }
        }

        if ($shouldStop) {
            try {
                Stop-Process -Id $proc.Id -Force -ErrorAction Stop
                $stopped += "$($proc.ProcessName)#$($proc.Id)"
            }
            catch {
                # ignore and continue
            }
        }
    }

    if ($stopped.Count -gt 0) {
        Write-Host "[LOCAL-TEST] Stopped stale build processes: $($stopped -join ', ')"
    }
}

function Build-StagedServer {
    param(
        [Parameter(Mandatory = $true)][string]$RunUATBat,
        [Parameter(Mandatory = $true)][string]$UProject,
        [Parameter(Mandatory = $true)][string]$ArchiveRoot,
        [Parameter(Mandatory = $true)][string[]]$MapPaths,
        [Parameter(Mandatory = $true)][bool]$BuildAllMaps
    )

    if (-not (Test-Path $RunUATBat)) {
        throw "RunUAT.bat not found: $RunUATBat"
    }

    $uatArgs = @(
        "BuildCookRun",
        "-project=$UProject",
        "-noP4",
        "-build",
        "-cook",
        "-stage",
        "-pak",
        "-archive",
        "-target=A302Server",
        "-server",
        "-serverplatform=Win64",
        "-serverconfig=Development",
        "-archivedirectory=$ArchiveRoot",
        "-UbtArgs=-WaitMutex"
    )

    if ($BuildAllMaps) {
        $uatArgs += "-allmaps"
    }
    else {
        $UniqueMaps = @($MapPaths | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
        if (-not $UniqueMaps -or $UniqueMaps.Count -eq 0) {
            throw "At least one map path is required when -CookAllMaps is not used."
        }

        $uatArgs += "-map=$($UniqueMaps -join '+')"
    }

    $engineDir = Split-Path (Split-Path (Split-Path $RunUATBat -Parent) -Parent) -Parent
    $uatLog = Join-Path $engineDir "Programs\AutomationTool\Saved\Logs\Log.txt"
    $ubaLog = Join-Path $engineDir "Programs\AutomationTool\Saved\Logs\UBA-A302Editor-Win64-Development.txt"

    Write-Host "[LOCAL-TEST] Building staged dedicated server via BuildCookRun..."
    Write-Host "[LOCAL-TEST] Archive directory: $ArchiveRoot"

    for ($attempt = 1; $attempt -le 2; $attempt++) {
        & $RunUATBat @uatArgs

        if ($LASTEXITCODE -eq 0) {
            return
        }

        $isMutexConflict = $false
        if (Test-Path $uatLog) {
            $recentUatLog = Get-Content $uatLog -Tail 200
            $isMutexConflict = $recentUatLog | Select-String -Pattern "A conflicting instance of Global\UnrealBuildTool_Mutex" -SimpleMatch -Quiet
        }

        if ($isMutexConflict -and $attempt -lt 2) {
            Write-Host "[LOCAL-TEST] UBT mutex conflict detected. Cleaning stale build processes and retrying once..."
            Stop-StaleUnrealBuildProcesses -EngineDir $engineDir
            Start-Sleep -Seconds 2
            continue
        }

        if (Test-Path $ubaLog) {
            $isLiveCodingLock = Select-String -Path $ubaLog -Pattern "Unable to build while Live Coding is active" -SimpleMatch -Quiet
            if ($isLiveCodingLock) {
                throw "BuildCookRun failed. ExitCode=$LASTEXITCODE (Live Coding lock). Close UnrealEditor or press Ctrl+Alt+F11, then rerun."
            }
        }

        if ($isMutexConflict) {
            throw "BuildCookRun failed. ExitCode=$LASTEXITCODE (UBT mutex conflict). Retry after stale build processes are closed."
        }

        throw "BuildCookRun failed. ExitCode=$LASTEXITCODE"
    }
}

function Close-UnrealEditorForBuild {
    param(
        [int]$WaitSeconds = 10
    )

    $editorProcesses = Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
    if (-not $editorProcesses) {
        return
    }

    Write-Host "[LOCAL-TEST] UnrealEditor is running. Closing it before BuildCookRun (Live Coding lock prevention)..."
    foreach ($proc in $editorProcesses) {
        try {
            if ($proc.MainWindowHandle -ne 0) {
                $null = $proc.CloseMainWindow()
            }
        }
        catch {
            # ignore and continue
        }
    }

    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    do {
        Start-Sleep -Milliseconds 500
        $editorProcesses = Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
    } while ($editorProcesses -and (Get-Date) -lt $deadline)

    if ($editorProcesses) {
        Write-Host "[LOCAL-TEST] UnrealEditor did not close in time. Forcing process termination..."
        $editorProcesses | Stop-Process -Force
    }
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectName = "A302"
$UProject = Join-Path $ProjectRoot "A302.uproject"
$RunUATBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$StopScript = Join-Path $PSScriptRoot "stop-local-test-stack.ps1"
$LobbyExe = (Resolve-Path (Join-Path $ProjectRoot "..\Server\GameServer\build\Release\Server.exe") -ErrorAction SilentlyContinue).Path
if (-not $LobbyExe) {
    $LobbyExe = Join-Path $ProjectRoot "..\Server\GameServer\build\Release\Server.exe"
}

if (-not (Test-Path $UProject)) {
    throw "A302.uproject not found: $UProject"
}

if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
    $ArchiveDirectory = Join-Path (Split-Path $ProjectRoot -Parent) "Artifacts"
}

if (-not $SkipStopExisting -and (Test-Path $StopScript)) {
    Write-Host "[LOCAL-TEST] Stopping existing local-test processes first..."
    & $StopScript -GamePort $GamePort
}

if (-not $SkipLobby) {
    if (-not (Test-Path $LobbyExe)) {
        throw "Lobby server executable not found: $LobbyExe"
    }

    Write-Host "[LOCAL-TEST] Starting lobby/voice server: $LobbyExe"
    Start-Process -FilePath $LobbyExe -WorkingDirectory (Split-Path $LobbyExe -Parent)
}

if (-not $SkipGameServer) {
    $DefaultServerMap = "/Game/PersonalWorkSpace/wjtmd28/MyMap"
    $ServerGameModeClass = "/Script/A302Server.A302GameMode"
    $ServerGameInstanceClass = "/Script/A302Shared.A302SharedGameInstance"
    $LaunchMap = ""
    if ($null -ne $Map) {
        $LaunchMap = $Map.Trim()
    }
    if ([string]::IsNullOrWhiteSpace($LaunchMap)) {
        $LaunchMap = $DefaultServerMap
    }

    if ($LaunchMap -match "^\?|^=") {
        Write-Warning "[LOCAL-TEST] Invalid -Map value '$LaunchMap'. Falling back to $DefaultServerMap."
        $LaunchMap = $DefaultServerMap
    }

    $LaunchMapBase = ($LaunchMap -split "\?")[0]
    if ($LaunchMapBase -notmatch "^/(Game|Engine)/") {
        Write-Warning "[LOCAL-TEST] Invalid map path '$LaunchMapBase'. Falling back to $DefaultServerMap."
        $LaunchMapBase = $DefaultServerMap
        $LaunchMap = $DefaultServerMap
    }

    # Force dedicated server GameMode even if map WorldSettings has a blueprint override.
    # Use URL option instead of relying only on ini defaults.
    $LaunchMap = $LaunchMapBase
    $LaunchURL = "${LaunchMapBase}?game=${ServerGameModeClass}"

    $CookMaps = @(
        $LaunchMapBase,
        "/Game/PersonalWorkSpace/wjtmd28/MyMap"
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique

    $StagedServerExe = Resolve-StagedServerExe -ArchiveRoot $ArchiveDirectory -ProjectRoot $ProjectRoot -ProjectName $ProjectName

    if ($ForceRebuildStaged -or -not $StagedServerExe) {
        if (-not $SkipCloseEditor) {
            Close-UnrealEditorForBuild
        }

        Build-StagedServer `
            -RunUATBat $RunUATBat `
            -UProject $UProject `
            -ArchiveRoot $ArchiveDirectory `
            -MapPaths $CookMaps `
            -BuildAllMaps ([bool]$CookAllMaps)

        $StagedServerExe = Resolve-StagedServerExe -ArchiveRoot $ArchiveDirectory -ProjectRoot $ProjectRoot -ProjectName $ProjectName
    }

    if (-not $StagedServerExe) {
        throw "Staged A302Server executable not found. ArchiveDirectory=$ArchiveDirectory"
    }

    $serverArgs = @(
        $LaunchURL,
        "-log",
        "-port=$GamePort",
        "-nosteam",
        "-nullrhi",
        "-nosound",
        "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:GameInstanceClass=$ServerGameInstanceClass",
        "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:GlobalDefaultServerGameMode=$ServerGameModeClass",
        "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:GlobalDefaultGameMode=$ServerGameModeClass",
        "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:ServerDefaultMap=$LaunchMapBase",
        "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:GameDefaultMap=$LaunchMapBase"
    )

    Write-Host "[LOCAL-TEST] Starting staged A302Server: $StagedServerExe $($serverArgs -join ' ')"
    Start-Process -FilePath $StagedServerExe -ArgumentList $serverArgs -WorkingDirectory (Split-Path $StagedServerExe -Parent)
}

Write-Host ""
Write-Host "[LOCAL-TEST] Ready"
Write-Host "  Lobby WS : ws://127.0.0.1:8001"
Write-Host "  Voice UDP: 127.0.0.1:48100"
Write-Host "  Game     : 127.0.0.1:$GamePort"
Write-Host "  Staged   : $ArchiveDirectory"
Write-Host ""
Write-Host "Now run editor client and test lobby -> start_game."
