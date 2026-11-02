$ErrorActionPreference='Stop'
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $root 'MakeRelease.ps1')
& (Join-Path $root 'MakeCleoRelease.ps1')
