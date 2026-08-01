[CmdletBinding(SupportsShouldProcess)]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$RepositoryRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
foreach ($Marker in @('premake5.lua', 'Config/Dependencies.lock')) {
    if (-not (Test-Path -LiteralPath (Join-Path $RepositoryRoot $Marker))) {
        throw "Cleanup could not validate the ProjectTemplate repository root: $RepositoryRoot"
    }
}

function Remove-ProjectPath {
    [CmdletBinding(SupportsShouldProcess)]
    param([Parameter(Mandatory)][string] $Path)

    $ResolvedPath = [System.IO.Path]::GetFullPath($Path)
    $RepositoryPrefix = $RepositoryRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $ResolvedPath.StartsWith($RepositoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Cleanup target is outside the repository: $ResolvedPath"
    }
    if (-not (Test-Path -LiteralPath $ResolvedPath)) { return }

    $RelativePath = $ResolvedPath.Substring($RepositoryPrefix.Length)
    if ($PSCmdlet.ShouldProcess($RelativePath, 'Remove generated project state')) {
        Remove-Item -LiteralPath $ResolvedPath -Recurse -Force
        Write-Host "Removed $RelativePath"
    }
}

foreach ($Directory in @('Binaries', 'Intermediate', 'Saved', 'External/Premake', '.idea', '.vs')) {
    Remove-ProjectPath -Path (Join-Path $RepositoryRoot $Directory)
}
foreach ($File in @('Makefile', 'compile_commands.json', '.DS_Store', 'Desktop.ini', 'Thumbs.db')) {
    Remove-ProjectPath -Path (Join-Path $RepositoryRoot $File)
}
foreach ($Pattern in @('*.code-workspace', '*.make', '*.sln', '*.slnx', '*.suo', '*.user', '*.vcxproj', '*.vcxproj.filters', '*.vcxproj.user', '*.workspace')) {
    foreach ($File in Get-ChildItem -LiteralPath $RepositoryRoot -File -Filter $Pattern) {
        Remove-ProjectPath -Path $File.FullName
    }
}

Write-Host $(if ($WhatIfPreference) { 'Cleanup dry run completed.' } else { 'Generated project state is clean.' })
