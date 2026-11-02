"""Validate the separate CLEO archive against the current build and every resource."""
from pathlib import Path
import hashlib
import re
import zipfile

root = Path(__file__).resolve().parents[1]
version = re.search(r"SA People ([0-9.]+) initialized", (root / "source/Plugin.cpp").read_text(encoding="utf-8"))[1]
name = f"SA-People-{version}-beta-CLEO"
archive = root / "release-cleo" / f"{name}.zip"
with zipfile.ZipFile(archive) as z:
    files = {i.filename: z.read(i) for i in z.infolist() if not i.is_dir()}
    assert len(files) == len(z.namelist()), "Duplicate or unexpected directory entries"
    assert sum(Path(n).name.lower() == "readme.md" for n in files) == 1
    assert not any(Path(n).suffix.lower() in {".asi", ".dll", ".exe", ".cpp", ".obj", ".log", ".dat"} for n in files)
    assert not any(Path(n).name.lower() == "settings.ini" for n in files)
    expected = {
        "README.md",
        "LICENSE.txt",
        f"Copy to game folder/CLEO/cleo_plugins/SA-People.cleo",
    }
    assert files[f"Copy to game folder/CLEO/cleo_plugins/SA-People.cleo"] == (root / "build/SA-People.cleo").read_bytes()
    resources = [root / "settings.example.ini", root / "languages.tsv"]
    for directory in ("audio", "assets", "backgrounds"):
        resources += [p for p in (root / directory).iterdir() if p.is_file() and p.suffix.lower() in {".mp3", ".wav", ".png", ".jpg", ".jpeg"}
                      and (directory != "backgrounds" or re.fullmatch(r"gtasanandreas(8|9|10)\.(jpg|png)", p.name))]
    for p in resources:
        entry = f"Copy to game folder/SA-People/{p.relative_to(root).as_posix()}"
        assert files[entry] == p.read_bytes(), f"Missing or changed resource: {p}"
        expected.add(entry)
    assert set(files) == expected, "Unexpected package contents"
    assert len([p for p in resources if p.parent.name == "audio"]) == 198
    assert b"ModernMenu=0" in files[f"Copy to game folder/SA-People/settings.example.ini"]
print("PASS: CLEO ZIP contains the current plugin, all 198 recordings, matching art/translations/default settings, one English README and no loader DLLs or player data.")
print("SHA256", hashlib.sha256(archive.read_bytes()).hexdigest())
