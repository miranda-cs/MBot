$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$SolutionPath = Join-Path $ProjectRoot "KBotExt.sln"
$OutputPath = Join-Path $ProjectRoot "x64"
$VsWherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

try {
    Write-Host "KBotExt - Release x64 build" -ForegroundColor Cyan
    Write-Host ""

    if (-not (Test-Path -LiteralPath $SolutionPath)) {
        throw "Solution not found: $SolutionPath"
    }

    if (-not (Test-Path -LiteralPath $VsWherePath)) {
        throw "vswhere.exe not found: $VsWherePath"
    }

    if (Test-Path -LiteralPath $OutputPath) {
        Write-Host "Cleaning output folder: $OutputPath"
        Remove-Item -LiteralPath $OutputPath -Recurse -Force
    }

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

    $ExePath = Join-Path $ProjectRoot "x64\Release\KBotExt.exe"
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
