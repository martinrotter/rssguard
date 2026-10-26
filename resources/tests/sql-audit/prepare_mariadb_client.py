"""Fetch a hash-verified pure-Python client into the isolated audit directory."""
import hashlib
import json
from pathlib import Path
from urllib.request import urlopen
from urllib.parse import urlparse
import zipfile

OUT=Path(__file__).resolve().parents[3]/'benchmark-results/sql-audit-20261014/maria'
with urlopen('https://pypi.org/pypi/PyMySQL/json',timeout=30) as response:
    meta=json.load(response)
wheel=next(item for item in meta['urls'] if item['filename'].endswith('-py3-none-any.whl'))
assert urlparse(wheel['url']).hostname=='files.pythonhosted.org'
with urlopen(wheel['url'],timeout=30) as response: payload=response.read()
assert hashlib.sha256(payload).hexdigest()==wheel['digests']['sha256']
archive=OUT/wheel['filename']; archive.write_bytes(payload)
deps=OUT/'deps'; deps.mkdir(exist_ok=True)
with zipfile.ZipFile(archive) as package:
    for name in package.namelist():
        target=(deps/name).resolve()
        assert target.is_relative_to(deps.resolve())
    package.extractall(deps)
(OUT/'client-provenance.json').write_text(json.dumps(dict(package='PyMySQL',version=meta['info']['version'],
    url=wheel['url'],sha256=wheel['digests']['sha256']),indent=2)+'\n',encoding='utf-8')
print(f"PyMySQL {meta['info']['version']} ready under {deps}")
