$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$SolutionPath = Join-Path $ProjectRoot "MBot.sln"
$OutputPath = Join-Path $ProjectRoot "x64"
$IntermediatePath = Join-Path $ProjectRoot "MBot\x64"
$VsWherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

function Clear-BuildDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$ExpectedRelativePath
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    $ResolvedProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
    $ResolvedPath = (Resolve-Path -LiteralPath $Path).Path
    $ExpectedPath = [System.IO.Path]::GetFullPath((Join-Path $ResolvedProjectRoot $ExpectedRelativePath))

    if ($ResolvedPath -ne $ExpectedPath) {
        throw "Refusing to clean unexpected path: $ResolvedPath"
    }

    Write-Host "Cleaning output folder: $ResolvedPath"
    Remove-Item -LiteralPath $ResolvedPath -Recurse -Force
}

try {
    Write-Host "MBot - Release x64 build" -ForegroundColor Cyan
    Write-Host ""

    if (-not (Test-Path -LiteralPath $SolutionPath)) {
        throw "Solution not found: $SolutionPath"
    }

    if (-not (Test-Path -LiteralPath $VsWherePath)) {
        throw "vswhere.exe not found: $VsWherePath"
    }

    Clear-BuildDirectory -Path $OutputPath -ExpectedRelativePath "x64"
    Clear-BuildDirectory -Path $IntermediatePath -ExpectedRelativePath "MBot\x64"

    $VisualStudioPath = & $VsWherePath `
        -latest `
        -products * `
        -requires Microsoft.Component.MSBuild `
        -property installationPath

    if ([string]::IsNullOrWhiteSpace($VisualStudioPath)) {
        throw "Visual Studio with MSBuild was not found."
    }

    $MSBuildPath = Join-Path $VisualStudioPath "MSBuild\Current\Bin\MSBuild.exe"
    if (-not (Test-Path -LiteralPath $MSBuildPath)) {
        throw "MSBuild.exe not found: $MSBuildPath"
    }

    Write-Host "Building Release x64..."
    Write-Host ""

    & $MSBuildPath $SolutionPath /m /p:Configuration=Release /p:Platform=x64

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE."
    }

    $ExePath = Join-Path $ProjectRoot "x64\Release\MBot.exe"
    Write-Host ""
    Write-Host "Build completed successfully." -ForegroundColor Green
    Write-Host "Executable: $ExePath"
}
catch {
    Write-Host ""
    Write-Host "Build failed:" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
finally {
    Write-Host ""
    Read-Host "Press Enter to close"
}
