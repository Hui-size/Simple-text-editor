param(
    [string]$QtRoot = 'F:\QT\qt',
    [string]$QtVersion = '6.10.3',
    [string]$BuildRoot = 'F:\Codex\huancun\text-editor-20261027',
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $PSScriptRoot
$buildPath = [IO.Path]::GetFullPath($BuildRoot)
$cacheRoot = [IO.Path]::GetFullPath('F:\Codex\huancun') + '\'
if (-not $buildPath.StartsWith($cacheRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'BuildRoot must be inside F:\Codex\huancun.'
}
$kitDir = Join-Path $QtRoot "$QtVersion\mingw_64"
$compilerDir = Join-Path $QtRoot 'Tools\mingw1310_64\bin'
$qmake = Join-Path $kitDir 'bin\qmake.exe'
$make = Join-Path $compilerDir 'mingw32-make.exe'
foreach ($tool in @($qmake, $make)) {
    if (-not (Test-Path -LiteralPath $tool)) { throw "Tool not found: $tool" }
}
New-Item -ItemType Directory -Path $buildPath -Force | Out-Null
$savedTemp = $env:TEMP
$savedTmp = $env:TMP
$savedPath = $env:PATH
$savedTestTemp = $env:EDITOR_TEST_TEMP
try {
    $env:TEMP = $buildPath
    $env:TMP = $buildPath
    $env:PATH = "$kitDir\bin;$compilerDir;$savedPath"

    function Build-QmakeProject([string]$Project, [string]$Output, [string]$Log) {
        New-Item -ItemType Directory -Path $Output -Force | Out-Null
        Push-Location $Output
        try {
            & $qmake $Project 'CONFIG+=release' 'CONFIG-=debug' *> $Log
            if ($LASTEXITCODE -ne 0) { Get-Content $Log -Tail 30; throw 'qmake failed.' }
            & $make -j4 *>> $Log
            if ($LASTEXITCODE -ne 0) { Get-Content $Log -Tail 50; throw 'Build failed.' }
        } finally {
            Pop-Location
        }
    }

    Build-QmakeProject (Join-Path $projectDir 'SimpleTextEditor.pro') (Join-Path $buildPath 'app') (Join-Path $buildPath 'app-build.log')
    Write-Output "Application: $buildPath\app\release\SimpleTextEditor.exe"
    if ($Test) {
        Build-QmakeProject (Join-Path $projectDir 'tests\editor_tests.pro') (Join-Path $buildPath 'tests') (Join-Path $buildPath 'test-build.log')
        $env:EDITOR_TEST_TEMP = Join-Path $buildPath 'test-data'
        & (Join-Path $buildPath 'tests\release\editor_tests.exe') '-o' (Join-Path $buildPath 'test-results.txt,txt')
        Get-Content -LiteralPath (Join-Path $buildPath 'test-results.txt')
        if ($LASTEXITCODE -ne 0) { throw 'Tests failed. See test-results.txt.' }
    }
} finally {
    $env:TEMP = $savedTemp
    $env:TMP = $savedTmp
    $env:PATH = $savedPath
    $env:EDITOR_TEST_TEMP = $savedTestTemp
}
