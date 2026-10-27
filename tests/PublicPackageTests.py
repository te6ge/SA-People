"""Validate the single public download without executing its files."""
from pathlib import Path, PurePosixPath
import hashlib, json, re, struct, zipfile

root=Path(__file__).resolve().parents[1]
version=re.search(r'SA People ([0-9.]+) initialized', (root/'source/Plugin.cpp').read_text(encoding='utf-8'))[1]
package=root/f'release/SA-People-{version}-beta-complete.zip'
prefix=f'SA-People-{version}/'
sha=lambda data:hashlib.sha256(data).hexdigest()
with zipfile.ZipFile(package) as archive:
    assert archive.testzip() is None
    names=archive.namelist()
    assert len(names)==len(set(n.casefold() for n in names))
    assert all(n.startswith(prefix) and '..' not in PurePosixPath(n).parts and '\\' not in n for n in names)
    files={n[len(prefix):]:archive.read(n) for n in names if not n.endswith('/')}
    readmes=[n for n in files if PurePosixPath(n).name.lower().startswith('readme')]
    assert readmes==['README.md'],readmes
    assert files['README.md']==(root/'README.md').read_bytes()
    assert b'Installation' in files['README.md'] and b'choose ONE method' in files['README.md']
    assert [n for n in files if n.lower().endswith('.exe')]==['SA-People-Setup.exe']
    assert sorted(n for n in files if n.endswith('.cmd'))==['Manual/Install.cmd','Manual/Uninstall.cmd']
    forbidden={'.cpp','.h','.inc','.py','.obj','.pdb','.log','.bak','.mp4','.pdf'}
    assert not any(PurePosixPath(n).suffix.lower() in forbidden or
        PurePosixPath(n).name.startswith(('people.','backup-','rollback')) for n in files)
    assert not any('VALIDATION' in n or 'CHECKLIST' in n or 'CHANGELOG' in n for n in files)
    for method in ['Manual/SA-People/','ModLoader/SA-People/','Manual/Copy-to-Game-Folder/SA-People/']:
        expected_audio={p.name for p in (root/'audio').iterdir() if p.suffix.lower() in ('.wav','.mp3')}
        actual_audio={n.removeprefix(method+'audio/') for n in files if n.startswith(method+'audio/') and n.endswith(('.wav','.mp3'))}
        assert actual_audio==expected_audio,(method,len(actual_audio),len(expected_audio))
        assert sum(n.startswith(method+'assets/') and n.endswith('.png') for n in files)==3
        assert sum(n.startswith(method+'backgrounds/') and n.endswith(('.jpg','.png')) for n in files)==3
    assert files['Manual/SA-People.asi']==files['ModLoader/SA-People/SA-People.asi']==(root/'build/SA-People.asi').read_bytes()
    copy='Manual/Copy-to-Game-Folder/'
    assert files[copy+'SA-People.asi']==files['Manual/SA-People.asi']
    assert files[copy+'dinput8.dll']==files['Manual/loader/dinput8.dll']
    assert {name[len(copy):].split('/')[0] for name in files if name.startswith(copy)}=={'SA-People.asi','dinput8.dll','SA-People'}
    assert copy+'SA-People/settings.ini' not in files
    assert files[copy+'SA-People/settings.example.ini']==files['Manual/SA-People/settings.ini']
    assert not any(name.startswith(copy) and name.endswith(('.cmd','.exe')) for name in files)
    for name,data in files.items():
        if name.startswith('Manual/SA-People/'):
            other='ModLoader/SA-People/'+name.removeprefix('Manual/SA-People/')
            if other.endswith('/settings.ini'):other=other.removesuffix('settings.ini')+'settings.example.ini'
            assert files[other]==data,other
            assert files[copy+'SA-People/'+other.removeprefix('ModLoader/SA-People/')]==data
    # The bundled EXE must contain exactly the verified manual payload bytes.
    setup=files['SA-People-Setup.exe']
    assert setup==(root/f'release/SA-People-{version}-beta-Setup.exe').read_bytes()
    index=struct.unpack_from('<Q',setup,len(setup)-16)[0]
    assert setup[-8:]==b'SAPPKG01'
    count=struct.unpack_from('<I',setup,index)[0]; cursor=index+4
    for _ in range(count):
        length=struct.unpack_from('<H',setup,cursor)[0];cursor+=2
        name=setup[cursor:cursor+length].decode('utf-8');cursor+=length
        mode,offset,size=struct.unpack_from('<BQQ',setup,cursor);cursor+=17
        target='Manual/'+('loader/dinput8.dll' if name=='dinput8.dll' else name)
        assert setup[offset:offset+size]==files[target],name
        assert mode in (0,1,2) and offset+size<=index
    assert cursor==len(setup)-16
report={'package':str(package),'sha256':sha(package.read_bytes()),'size':package.stat().st_size,
        'readme_count':len(readmes),'files':len(files),'setup_payload_files':count,
        'setup_sha256':sha(setup),'manual_zip_sha256':sha((root/f'release/SA-People-{version}-beta.zip').read_bytes()),
        'passed':True}
(root/'build/public-package-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
