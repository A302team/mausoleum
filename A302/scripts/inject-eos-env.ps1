param(
    [string]$EnvFile = ".env",
    [string]$EngineIni = "Config/DefaultEngine.ini"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Read-DotEnv {
    param([string]$Path)
    $map = @{}
    if (-not (Test-Path -LiteralPath $Path)) {
        return $map
    }

    foreach ($line in Get-Content -LiteralPath $Path) {
        $trimmed = $line.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed)) { continue }
        if ($trimmed.StartsWith("#")) { continue }
        $idx = $trimmed.IndexOf("=")
        if ($idx -lt 1) { continue }

        $key = $trimmed.Substring(0, $idx).Trim()
        $value = $trimmed.Substring($idx + 1).Trim()
        if (($value.StartsWith('"') -and $value.EndsWith('"')) -or ($value.StartsWith("'") -and $value.EndsWith("'"))) {
            $value = $value.Substring(1, $value.Length - 2)
        }
        $map[$key] = $value
    }

    return $map
}

function Get-Value {
    param(
        [hashtable]$DotEnv,
        [string]$Name
    )

    $envValue = [Environment]::GetEnvironmentVariable($Name)
    if (-not [string]::IsNullOrWhiteSpace($envValue)) {
        return $envValue
    }
    if ($DotEnv.ContainsKey($Name) -and -not [string]::IsNullOrWhiteSpace($DotEnv[$Name])) {
        return $DotEnv[$Name]
    }
    return $null
}

$dotEnv = Read-DotEnv -Path $EnvFile

$required = @(
    "EOS_ARTIFACT_NAME",
    "EOS_CLIENT_ID",
    "EOS_CLIENT_SECRET",
    "EOS_PRODUCT_ID",
    "EOS_SANDBOX_ID",
    "EOS_DEPLOYMENT_ID",
    "EOS_ENCRYPTION_KEY"
)

$vals = @{}
foreach ($name in $required) {
    $val = Get-Value -DotEnv $dotEnv -Name $name
    if ([string]::IsNullOrWhiteSpace($val)) {
        throw "Missing required value: $name (env or $EnvFile)"
    }
    $vals[$name] = $val
}

if ($vals["EOS_ENCRYPTION_KEY"] -notmatch '^[0-9A-Fa-f]{64}$') {
    throw "EOS_ENCRYPTION_KEY must be exactly 64 hex chars."
}

if (-not (Test-Path -LiteralPath $EngineIni)) {
    throw "Engine ini not found: $EngineIni"
}

$content = Get-Content -LiteralPath $EngineIni -Raw

$artifactLine = "DefaultArtifactName=$($vals["EOS_ARTIFACT_NAME"])"
$artifactsValue = '+Artifacts=(ArtifactName="{0}",ClientId="{1}",ClientSecret="{2}",ProductId="{3}",SandboxId="{4}",DeploymentId="{5}",EncryptionKey="{6}")' -f `
    $vals["EOS_ARTIFACT_NAME"], `
    $vals["EOS_CLIENT_ID"], `
    $vals["EOS_CLIENT_SECRET"], `
    $vals["EOS_PRODUCT_ID"], `
    $vals["EOS_SANDBOX_ID"], `
    $vals["EOS_DEPLOYMENT_ID"], `
    $vals["EOS_ENCRYPTION_KEY"]

$content = [regex]::Replace($content, '(?m)^DefaultArtifactName=.*$', [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $artifactLine }, 1)
$content = [regex]::Replace($content, '(?m)^\+Artifacts=\(.*\)$', [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $artifactsValue }, 1)

if ($content -notmatch '(?m)^DefaultArtifactName=') {
    $content += "`r`nDefaultArtifactName=$($vals["EOS_ARTIFACT_NAME"])"
}
if ($content -notmatch '(?m)^\+Artifacts=\(') {
    $content += "`r`n$artifactsValue"
}

Set-Content -LiteralPath $EngineIni -Value $content -Encoding UTF8
Write-Host "Updated $EngineIni from environment values."
