"""Generate a review-only integration patch; never edit application files."""
from pathlib import Path
import difflib

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = ROOT / 'benchmark-results/sql-audit-20261014'
index = ('CREATE INDEX IF NOT EXISTS idx_Messages_feed_display\n'
         'ON Messages (feed, date_created DESC, title ASC, id DESC)\n'
         'WHERE is_deleted = 0 AND is_pdeleted = 0;\n')
changes = {}
init = ROOT / 'resources/sql/db_init_sqlite.sql'
changes['resources/sql/db_init_sqlite.sql'] = init.read_text(encoding='utf-8').rstrip()+'\n-- !\n'+index
definitions = ROOT / 'src/librssguard/definitions/definitions.h'
old_definitions = definitions.read_text(encoding='utf-8')
assert '#define APP_DB_SCHEMA_VERSION                "105"' in old_definitions
changes['src/librssguard/definitions/definitions.h'] = old_definitions.replace(
    '#define APP_DB_SCHEMA_VERSION                "105"', '#define APP_DB_SCHEMA_VERSION                "106"')
changes['resources/sql/db_update_sqlite_105_106.sql'] = index
# Schema version is global across backends. Keep the other backend's upgrade
# valid without introducing SQLite's partial index into MariaDB.
changes['resources/sql/db_update_mysql_105_106.sql'] = 'SELECT 1;\n'
resource = ROOT / 'resources/sql.qrc'
old_resource = resource.read_text(encoding='utf-8')
changes['resources/sql.qrc'] = old_resource.replace('  </qresource>',
    '\n    <file>sql/db_update_sqlite_105_106.sql</file>\n'
    '    <file>sql/db_update_mysql_105_106.sql</file>\n  </qresource>')
chunks=[]
for name,new in changes.items():
    path=ROOT/name
    old=path.read_text(encoding='utf-8') if path.exists() else ''
    chunks.append(f'diff --git a/{name} b/{name}\n')
    if not path.exists(): chunks.append('new file mode 100644\n')
    for line in difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),
                                    fromfile='a/'+name if path.exists() else '/dev/null',tofile='b/'+name):
        if line.endswith('\n'): chunks.append(line)
        else: chunks.append(line+'\n\\ No newline at end of file\n')
patch=OUT/'index-only-proposal.patch'
patch.write_text(''.join(chunks),encoding='utf-8',newline='\n')
print(str(patch))
