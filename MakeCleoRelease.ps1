$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$version = [regex]::Match([IO.File]::ReadAllText("$root\source\Plugin.cpp"), 'SA People ([0-9.]+) initialized').Groups[1].Value
if (-not $version -or -not (Test-Path "$root\build\SA-People.cleo")) { throw 'Run Build.cmd first' }
$out = Join-Path $root 'release-cleo'
$name = "SA-People-$version-beta-CLEO"
$stage = Join-Path $out $name
if ([IO.Path]::GetFullPath($out) -ne [IO.Path]::GetFullPath((Join-Path $root 'release-cleo')) -or
    [IO.Path]::GetFullPath((Split-Path -Parent $stage)) -ne [IO.Path]::GetFullPath($out)) { throw 'Unexpected CLEO staging path' }
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
$payload = Join-Path $stage 'Copy to game folder'
New-Item -ItemType Directory -Force "$payload\CLEO\cleo_plugins", "$payload\SA-People" | Out-Null
Copy-Item -LiteralPath "$root\build\SA-People.cleo" -Destination "$payload\CLEO\cleo_plugins\SA-People.cleo"
Copy-Item -LiteralPath "$root\settings.example.ini", "$root\languages.tsv" -Destination "$payload\SA-People"
foreach ($dir in 'audio', 'assets', 'backgrounds') {
    New-Item -ItemType Directory -Force "$payload\SA-People\$dir" | Out-Null
    Get-ChildItem -LiteralPath "$root\$dir" -File | Where-Object {
        ($_.Extension -in '.mp3', '.wav', '.png', '.jpg', '.jpeg') -and
        ($dir -ne 'backgrounds' -or $_.Name -match '^gtasanandreas(8|9|10)\.(jpg|png)$')
    } | Copy-Item -Destination "$payload\SA-People\$dir"
}
Copy-Item -LiteralPath "$root\release-template\LICENSE.txt" -Destination $stage
$readme = @"
# SA People $version beta - CLEO plugin edition

This is a native C++ CLEO plugin, not a .cs script. It uses the same gameplay code,
phone, identities, relationships and audio as the ASI edition.

## Requirements
GTA San Andreas PC 1.0 US, single player, and CLEO 5 with its required ASI loader.
CLEO 4, Steam/3.0, 1.01, Definitive Edition and multiplayer are not supported.
Install CLEO from https://cleo.li/ . This archive does not bundle CLEO or loader DLLs.
It does not guarantee compatibility with every third-party script or mod pack.

## Install or switch from ASI
1. Close the game. Back up your existing SA-People folder.
2. Remove SA-People.asi from the game root and any ModLoader SA People installation.
   Keep the game's SA-People folder: it holds your settings and relationships.
3. Copy the CONTENTS of 'Copy to game folder' beside gta_sa.exe. Merge folders.
   The plugin must end up at CLEO/cleo_plugins/SA-People.cleo.
4. Start the game normally. Do not run this file and do not rename it to .asi.

Existing settings and the legacy people.dat are preserved. Version 0.19 uses separate
social checkpoints for each GTA save, matching the ASI edition. Save GTA after social
progress. To carry older contacts into a selected save, load it and choose phone →
Settings → Import shared archive, then confirm. Other saves remain separate.
Back up and transfer the complete SA-People/worlds folder with your GTA saves.
The example INI is
used only when no settings.ini exists. Do not run the ASI and CLEO editions together;
duplicate-instance protection allows only the first one to initialize.

## Grove cashbook and home supplies
Walk to the outdoor stand in front of CJ's house and press L to open or close the paper cashbook.
W/S or Up/Down select a row; E/Enter confirms; Q/A/Left goes back. Mouse controls also work.
Collect unlocked supplies from the two tables. Contributions are $100/$1,000/$10,000.
Polish installer language selection is fixed in the normal edition.
Buy two levels each of food supplies ($2,500/$6,500), workshop ($5,000/$10,000),
protective equipment ($6,000/$12,000) and weapon cache ($20,000/$45,000).
Health/armor pickups appear outside the house, restocking after 24/12 tracked hours.
Weapons: pistol (34 rounds), then shotgun (12 shells) and Micro SMG (60 rounds), each
restocking after 24 hours. The game's native pickup and weapon-swap controls apply.
Workshop engine repairs retain +250/+500 health with 24/12-hour cooldowns.
Runtime pickups are removed before GTA saves/loads and during story missions.
Per-save archive V20 preserves old V19 balances/upgrades using a migration credit.
Back up the entire SA-People folder before testing; older plugins cannot read V20.

## Controls and optional menu
Hold Left Alt to focus. G: greet, H: provoke, J: calm down, R: rob.
F6 or P: phone. Hold TAB: honor. Rebind through phone settings or settings.ini.
The original GTA menu is the default. Set [Interface] ModernMenu=1 in settings.ini
to opt into the SA People main/pause menu, then restart. ModernMenu=0 restores it.
Languages: English, Turkish, Russian, Spanish, Simplified Chinese, Japanese, Polish. The phone remains available either way.

## Optional GInput support
Install GInputSA separately from https://silentsblog.com/mods/gta-sa/#ginput .
This package does not bundle or replace GInput or loader DLLs. Both SA People editions
use the same controller code. Hold LB+RB (L1+R1) to focus; A/Cross: greet,
B/Circle: provoke, X/Square: defuse, Y/Triangle: rob. Hold focus + D-pad Right to repair.
LB+D-pad Up opens the phone; LB+D-pad Down shows honor. In the phone use D-pad,
A/Cross to select, B/Circle to go back and Y/Triangle to close. Keyboard controls remain usable.
Phone and focus prompts follow GInput's Xbox/PlayStation setting. In settings.ini,
[Controller] Enabled=0 disables this integration; Glyphs=-1 follows GInput, 0 forces Xbox,
1 forces PlayStation. Gamepad chords are fixed; phone key rebinding edits keyboard keys.
Physical-controller gameplay still requires wider live testing.

## Troubleshooting and uninstall
Check SA-People/SA-People.log for 'CLEO 5 plugin active' after starting a session.
If no log is created, check CLEO's own log, plugin location and game version.
This is an experimental adapter. Build, callback-host and gameplay simulation tests
do not replace a complete live-game test with the real CLEO runtime.
Native hooks stay loaded until game exit; hot unloading/reloading is not supported.
To uninstall, close the game and remove CLEO/cleo_plugins/SA-People.cleo.
Keep SA-People if you want to retain your data or switch back to ASI.

Source (MIT): https://github.com/te6ge/SA-People
Voice lines include AI-generated recordings. Not affiliated with Rockstar Games.
No original GTA game files are included.
"@
[IO.File]::WriteAllText("$stage\README.md", $readme, (New-Object Text.UTF8Encoding($false)))
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zipPath = Join-Path $out "$name.zip"
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
$zip = [IO.Compression.ZipFile]::Open($zipPath, 'Create')
try {
    foreach ($file in Get-ChildItem -LiteralPath $stage -Recurse -File) {
        $entry = $file.FullName.Substring($stage.Length + 1).Replace('\', '/')
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $file.FullName, $entry, [IO.Compression.CompressionLevel]::Optimal)
    }
} finally { $zip.Dispose() }
Get-FileHash -LiteralPath $zipPath -Algorithm SHA256
Get-Item -LiteralPath $zipPath | Select-Object FullName, Length
