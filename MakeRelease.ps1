# Builds the public beta packages in .\release\. Run after Build.cmd. Packs the owner's voice clips
# and the three original menu pictures; never saves, logs or rollback folders.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root
$utf8 = New-Object Text.UTF8Encoding($false)
$plugin = [IO.File]::ReadAllText("$root\source\Plugin.cpp")
$version = [regex]::Match($plugin, 'SA People ([0-9.]+) initialized').Groups[1].Value
if (-not $version) { throw 'Version not found in Plugin.cpp' }
if (-not (Test-Path "$root\build\SA-People.asi")) { throw 'Run Build.cmd first' }

$out = "$root\release"
$name = "SA-People-$version-beta"
$stage = "$out\$name"
if ([IO.Path]::GetFullPath($out) -ne [IO.Path]::GetFullPath((Join-Path $root 'release'))) { throw 'Unexpected release cleanup path' }
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Force "$stage\SA-People\assets", "$stage\SA-People\audio", "$stage\SA-People\backgrounds" | Out-Null

Copy-Item "$root\build\SA-People.asi" $stage
Copy-Item "$root\languages.tsv" "$stage\SA-People\"
Copy-Item "$root\assets\*.png" "$stage\SA-People\assets\"
New-Item -ItemType Directory -Force "$stage\loader", "$stage\SA-People\licenses" | Out-Null
Copy-Item "$root\loader\dinput8.dll" "$stage\loader\"
Copy-Item "$root\loader\Ultimate-ASI-Loader-LICENSE.txt" "$stage\loader\"
Copy-Item "$root\loader\Ultimate-ASI-Loader-LICENSE.txt" "$stage\SA-People\licenses\"
if (Test-Path "$root\audio") { Copy-Item "$root\audio\*" "$stage\SA-People\audio\" -Include *.wav, *.mp3 }
foreach ($n in 8, 9, 10) {
    if (Test-Path "$root\backgrounds\gtasanandreas$n.*") { Copy-Item "$root\backgrounds\gtasanandreas$n.*" "$stage\SA-People\backgrounds\" }
} # Owner-made art only; source-only checkouts may supply media separately.
foreach ($f in 'LICENSE.txt', 'Install.cmd', 'Uninstall.cmd') {
    Copy-Item "$root\release-template\$f" $stage
}
Copy-Item "$root\README.md" $stage

[IO.File]::WriteAllText("$stage\SA-People\settings.ini", @"
[World]
CommunityLife=1
AutoRestore=1
SideEvents=1
FirstEventSeconds=45

[Interface]
ScalePercent=100
AutoCloseSeconds=0
Language=en
ModernMenu=1

[Audio]
Enabled=1
HonorPercent=10
VoicePercent=45
NpcVoicePercent=30
"@, $utf8)

Add-Type -AssemblyName System.IO.Compression.FileSystem
# Zip with forward-slash entry names so every unzip tool agrees.
function New-Zip($dir, $zipPath) {
    $base = (Split-Path -Parent $dir).TrimEnd('\') + '\'
    $zip = [IO.Compression.ZipFile]::Open($zipPath, 'Create')
    foreach ($f in Get-ChildItem $dir -Recurse -File) {
        $entry = $f.FullName.Substring($base.Length).Replace('\', '/')
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $f.FullName, $entry, [IO.Compression.CompressionLevel]::Optimal)
    }
    $zip.Dispose()
}
# Single-file setup: Installer.exe followed by the mod files and an index (see Installer.cpp).
function New-Setup($stageDir, $setupPath) {
    Copy-Item "$root\build\Installer.exe" $setupPath
    $fs = [IO.File]::Open($setupPath, 'Open', 'ReadWrite')
    $fs.Seek(0, 'End') | Out-Null
    $entries = @()
    $files = @(Get-Item "$stageDir\SA-People.asi") + @(Get-ChildItem "$stageDir\SA-People" -Recurse -File) + @(Get-Item "$stageDir\loader\dinput8.dll")
    foreach ($f in $files) {
        $rel = $f.FullName.Substring($stageDir.Length + 1).Replace('\', '/')
        $mode = if ($rel -eq 'SA-People/settings.ini') { 1 } elseif ($rel -eq 'loader/dinput8.dll') { 2 } else { 0 }
        if ($mode -eq 2) { $rel = 'dinput8.dll' } # Lands in the game root, only if no loader exists.
        $bytes = [IO.File]::ReadAllBytes($f.FullName)
        $entries += [pscustomobject]@{ Path = $rel; Mode = [byte]$mode; Offset = [uint64]$fs.Position; Size = [uint64]$bytes.Length }
        $fs.Write($bytes, 0, $bytes.Length)
    }
    $indexAt = [uint64]$fs.Position
    $w = New-Object IO.BinaryWriter($fs)
    $w.Write([uint32]$entries.Count)
    foreach ($e in $entries) {
        $name = [Text.Encoding]::UTF8.GetBytes($e.Path)
        $w.Write([uint16]$name.Length); $w.Write($name); $w.Write($e.Mode); $w.Write($e.Offset); $w.Write($e.Size)
    }
    $w.Write($indexAt)
    $w.Write([Text.Encoding]::ASCII.GetBytes('SAPPKG01'))
    $w.Flush(); $fs.Dispose()
}
$setup = "$out\$name-Setup.exe"
New-Setup $stage $setup
# The standalone setup is distributed separately; keep the manual ZIP free of EXEs.
$binaryZip = "$out\$name.zip"
New-Zip $stage $binaryZip

# ModLoader: a single flat folder, no installer or replacement loader DLL.
$modStage = "$out\SA-People"
New-Item -ItemType Directory -Path $modStage | Out-Null
Copy-Item "$stage\SA-People.asi" $modStage
Copy-Item "$stage\SA-People\*" $modStage -Recurse
Move-Item -LiteralPath "$modStage\settings.ini" -Destination "$modStage\settings.example.ini"
$modZip = "$out\$name-modloader.zip"
New-Zip $modStage $modZip

# Source package.
$src = "$out\$name-source"
New-Item -ItemType Directory -Force "$src\source" | Out-Null
Copy-Item "$root\source\*" "$src\source\" -Include *.h, *.cpp, *.inc
foreach ($f in 'Build.cmd', 'README.md', 'LICENSE.txt', 'languages.tsv', '.clang-format', 'MakeRelease.ps1', 'settings.example.ini') { Copy-Item "$root\$f" $src }
Copy-Item "$root\assets" "$src\assets" -Recurse
foreach ($folder in 'loader', 'release-template', 'tests') {
    if (Test-Path "$root\$folder") { Copy-Item "$root\$folder" "$src\$folder" -Recurse }
}
New-Zip $src "$out\$name-source.zip"

# One public download. Auxiliary archives above are local build/test artifacts only.
$publicStage = "$out\SA-People-$version"
New-Item -ItemType Directory -Force "$publicStage\Manual", "$publicStage\ModLoader" | Out-Null
Copy-Item "$root\README.md" $publicStage
Copy-Item "$root\LICENSE.txt" $publicStage
Copy-Item $setup "$publicStage\SA-People-Setup.exe"
Get-ChildItem $stage | Where-Object { $_.Name -ne 'README.md' } | Copy-Item -Destination "$publicStage\Manual" -Recurse
Copy-Item $modStage "$publicStage\ModLoader\SA-People" -Recurse
# Ready-to-copy layout: no scripts and no live settings.ini to overwrite on updates.
$copyStage = "$publicStage\Manual\Copy-to-Game-Folder"
New-Item -ItemType Directory -Path $copyStage | Out-Null
Copy-Item "$stage\SA-People.asi" $copyStage
Copy-Item "$stage\loader\dinput8.dll" $copyStage
Copy-Item "$stage\SA-People" "$copyStage\SA-People" -Recurse
Move-Item -LiteralPath "$copyStage\SA-People\settings.ini" -Destination "$copyStage\SA-People\settings.example.ini"
$publicZip = "$out\$name-complete.zip"
New-Zip $publicStage $publicZip

# Verify: nothing private or third-party may be inside.
$bad = @()
foreach ($zip in $binaryZip, $modZip, "$out\$name-source.zip", $publicZip) {
    $archive = [IO.Compression.ZipFile]::OpenRead($zip)
    foreach ($e in $archive.Entries) {
        $allowed = ($zip -ne "$out\$name-source.zip") -and ($e.FullName -match '(?:^|/)SA-People/audio/[^/]+\.(wav|mp3)$|(?:^|/)SA-People/backgrounds/gtasanandreas(8|9|10)\.(jpg|png)$')
        if (-not $allowed -and $e.FullName -match '\.(wav|mp3|ogg|jpg|jpeg|bmp|log|bak|webp)$|people\.dat|people\.pre|rollback') { $bad += "$zip : $($e.FullName)" }
    }
    $archive.Dispose()
}
foreach ($cleanup in $stage, $src, $modStage, $publicStage) {
    if ([IO.Path]::GetFullPath((Split-Path -Parent $cleanup)) -ne [IO.Path]::GetFullPath($out)) { throw 'Unexpected staging cleanup path' }
    Remove-Item -LiteralPath $cleanup -Recurse -Force
}
if ($bad) { $bad; throw 'Forbidden files found in a package' }
Get-ChildItem $out -Filter *.zip | Select-Object Name, @{n = 'KB'; e = { [int]($_.Length / 1KB) } }
