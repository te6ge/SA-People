"""Exercise release EXE/CMD installers against disposable fake game folders only."""
from pathlib import Path
import ctypes
import hashlib
import json
import stat
import struct
import subprocess
import time
import zipfile

project = Path(__file__).resolve().parents[1]
base = project / 'build' / ('installer-tests-' + time.strftime('%Y%m%d-%H%M%S'))
base.mkdir(parents=True)
release = project / 'release'
setup = next(release.glob('*-beta-Setup.exe'))
archive = next(p for p in release.glob('*-beta.zip'))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    z.extractall(base / 'package')
stage = next((base / 'package').iterdir())
results = []

def check(name, condition):
    if not condition:
        raise AssertionError(name)
    results.append(name)
    print('PASS:', name, flush=True)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def fake(name, size=14383616):
    game = base / name
    game.mkdir()
    with (game / 'gta_sa.exe').open('wb') as f:
        f.truncate(size)
    return game

def run(kind, game, exe=setup):
    if kind == 'cmd':
        command = 'cmd.exe /d /s /c ""' + str(stage / 'Install.cmd') + '" "' + str(game) + '""'
    else:
        command = [str(exe), '--silent', str(game)]
    r = subprocess.run(command, input=b'N\r\n', capture_output=True, timeout=45)
    (base / (game.name + '-' + kind + '.stdout.txt')).write_bytes(r.stdout)
    (base / (game.name + '-' + kind + '.stderr.txt')).write_bytes(r.stderr)
    return r

def old_state(game):
    data = game / 'SA-People'
    data.mkdir(exist_ok=True)
    for relative, content in {
        'SA-People.asi': b'previous ASI',
        'SA-People/people.dat': b'user identities',
        'SA-People/people.dat.bak': b'user identity backup',
        'SA-People/settings.ini': b'[Interface]\r\nLanguage=tr\r\n',
        'SA-People/languages.tsv': b'previous translations',
        'SA-People/audio/custom.wav': b'custom sound',
        'SA-People/backgrounds/custom.png': b'custom image',
    }.items():
        p = game / relative
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(content)

def preserved(game):
    return {str(p.relative_to(game)): p.read_bytes() for p in (game / 'SA-People').rglob('*')
            if p.is_file() and not any(part.startswith('backup-') for part in p.relative_to(game).parts)
            and p.name in ('people.dat', 'people.dat.bak', 'settings.ini', 'custom.wav', 'custom.png')}

try:
    stub = (project / 'build/Installer.exe').read_bytes()
    check('installer stub has explicit asInvoker manifest', b'requestedExecutionLevel' in stub and b'asInvoker' in stub)
    dll = (stage / 'loader/dinput8.dll').read_bytes()
    pe = struct.unpack_from('<I', dll, 60)[0]
    check('bundled loader is x86 PE', dll[:2] == b'MZ' and struct.unpack_from('<H', dll, pe+4)[0] == 0x14c)
    for kind in ('cmd', 'exe'):
        for loader in ('none', 'stock-vorbis', 'existing-dinput', 'existing-vorbis'):
            game = fake(kind + ' ' + loader + ' Türkçe & Mods')
            if loader == 'stock-vorbis': (game / 'vorbisFile.dll').write_bytes(b'V'*65536)
            if loader == 'existing-dinput': (game / 'dinput8.dll').write_bytes(b'keep existing loader')
            if loader == 'existing-vorbis': (game / 'vorbisFile.dll').write_bytes(b'V'*200001)
            before_loader = {p.name:p.read_bytes() for p in game.glob('*.dll')}
            r = run(kind, game)
            check(kind + ' fresh ' + loader + ' succeeds without copy errors', r.returncode == 0 and not r.stderr.strip())
            check(kind + ' installs matching ASI ' + loader, digest(game/'SA-People.asi') == digest(stage/'SA-People.asi'))
            check(kind + ' includes loader license ' + loader, (game/'SA-People/licenses/Ultimate-ASI-Loader-LICENSE.txt').exists())
            for name, content in before_loader.items():
                check(kind + ' preserves ' + name + ' ' + loader, (game/name).read_bytes() == content)
            check(kind + ' loader policy ' + loader,
                  (game/'dinput8.dll').read_bytes() == dll if loader in ('none','stock-vorbis')
                  else not (game/'dinput8.dll').exists() if loader == 'existing-vorbis'
                  else (game/'dinput8.dll').read_bytes() == before_loader['dinput8.dll'])
            if loader == 'none':
                # Verify every bundled data file, not just the entry-point ASI.
                check(kind + ' all installed data hashes match', all(digest(game / ('SA-People/settings.ini' if p.name=='settings.example.ini' else p.relative_to(stage))) == digest(p)
                      for p in (stage/'SA-People').rglob('*') if p.is_file()))
        game = fake(kind + ' update')
        old_state(game)
        before = preserved(game)
        r = run(kind, game)
        check(kind + ' update preserves user state and extras', r.returncode == 0 and preserved(game) == before)
        backups = list((game/'SA-People').glob('backup-*'))
        check(kind + ' backs up ASI, settings and both identity files', len(backups)==1 and
              (backups[0]/'SA-People.asi').read_bytes()==b'previous ASI' and
              (backups[0]/'people.dat').read_bytes()==b'user identities' and
              (backups[0]/'people.dat.bak').read_bytes()==b'user identity backup' and
              (backups[0]/'settings.ini').read_bytes()==before['SA-People\\settings.ini'])
        r = run(kind, game)
        check(kind + ' repeat install retains state and creates another backup', r.returncode==0 and
              preserved(game)==before and len(list((game/'SA-People').glob('backup-*')))==2)
        game = fake(kind + ' folder conflict')
        old_state(game)
        blocking = game/'SA-People/audio'
        # Use a different required folder, leaving custom sounds untouched.
        blocking = game/'SA-People/assets'
        blocking.write_bytes(b'not a folder')
        r = run(kind, game)
        check(kind + ' folder conflict fails before replacing old ASI', r.returncode != 0 and
              (game/'SA-People.asi').read_bytes()==b'previous ASI' and blocking.read_bytes()==b'not a folder')
        game = fake(kind + ' read only')
        old_state(game)
        relative = next((stage/'SA-People/assets').glob('*.png')).relative_to(stage)
        protected = game/relative
        protected.parent.mkdir(exist_ok=True)
        protected.write_bytes(b'protected asset')
        protected.chmod(stat.S_IREAD)
        try:
            r = run(kind, game)
            check(kind + ' read-only copy failure returns failure', r.returncode!=0 and
                  b'SA People installed.' not in r.stdout and protected.read_bytes()==b'protected asset')
        finally:
            protected.chmod(stat.S_IREAD|stat.S_IWRITE)
        game = base/(kind+' missing game')
        game.mkdir()
        check(kind + ' rejects folder without gta_sa.exe', run(kind,game).returncode != 0 and not (game/'SA-People.asi').exists())
        game = fake(kind + ' unsupported exe', 10)
        check(kind + ' rejects unsupported exe without confirmation', run(kind,game).returncode != 0 and not (game/'SA-People.asi').exists())
    # Corrupt/tampered self-extracting overlays must fail silently before any writes.
    for name, entrypath, offset, size, mode in [
        ('traversal','../escape.dll',len(stub),1,0),
        ('absolute','C:/outside.dll',len(stub),1,0),
        ('overflow','safe.dll',2**64-1,8,0),
        ('unknown-mode','safe.dll',len(stub),1,3),
    ]:
        encoded=entrypath.encode()
        index_at=len(stub)+1
        index=struct.pack('<I',1)+struct.pack('<H',len(encoded))+encoded+struct.pack('<BQQ',mode,offset,size)
        broken=base/(name+'.exe')
        broken.write_bytes(stub+b'x'+index+struct.pack('<Q',index_at)+b'SAPPKG01')
        game=fake('malformed '+name)
        r=run('exe',game,broken)
        check('rejects malformed package '+name, r.returncode != 0 and list(game.iterdir())==[game/'gta_sa.exe'])
    with zipfile.ZipFile(next(release.glob('*-source.zip'))) as z:
        names=z.namelist()
        check('source archive includes loader, license, templates and tests', all(any(n.endswith(suffix) for n in names)
              for suffix in ['/loader/dinput8.dll','/loader/Ultimate-ASI-Loader-LICENSE.txt','/release-template/Install.cmd','/tests/InstallerTests.py']))
    report={'passed':len(results),'checks':results,'folder':str(base),'setup_sha256':digest(setup),'zip_sha256':digest(archive)}
    (base/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print('ALL PASSED:',len(results))
    print('Report:',base/'report.json')
except Exception:
    (base/'partial-report.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
    raise
