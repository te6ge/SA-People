"""Validate the single public download without executing its files."""
from pathlib import Path, PurePosixPath
import hashlib, json, struct, zipfile

root=Path(__file__).resolve().parents[1]
package=root/'release/SA-People-0.14.2-beta-complete.zip'
prefix='SA-People-0.14.2/'
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
    for method in ['Manual/SA-People/','ModLoader/SA-People/']:
        assert sum(n.startswith(method+'audio/') and n.endswith(('.wav','.mp3')) for n in files)==80
        assert sum(n.startswith(method+'assets/') and n.endswith('.png') for n in files)==3
        assert sum(n.startswith(method+'backgrounds/') and n.endswith(('.jpg','.png')) for n in files)==3
    assert files['Manual/SA-People.asi']==files['ModLoader/SA-People/SA-People.asi']==(root/'build/SA-People.asi').read_bytes()
    for name,data in files.items():
        if name.startswith('Manual/SA-People/'):
            other='ModLoader/SA-People/'+name.removeprefix('Manual/SA-People/')
            if other.endswith('/settings.ini'):other=other.removesuffix('settings.ini')+'settings.example.ini'
            assert files[other]==data,other
    # The bundled EXE must contain exactly the verified manual payload bytes.
    setup=files['SA-People-Setup.exe']
    assert setup==(root/'release/SA-People-0.14.2-beta-Setup.exe').read_bytes()
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
        'setup_sha256':sha(setup),'manual_zip_sha256':sha((root/'release/SA-People-0.14.2-beta.zip').read_bytes()),
        'passed':True}
(root/'build/public-package-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
