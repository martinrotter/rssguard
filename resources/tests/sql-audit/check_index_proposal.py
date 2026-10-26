"""Check index definition and new/upgrade/retry paths entirely in memory."""
import json
import re
import sqlite3
from pathlib import Path
import build_index_proposal as proposal
from generate_fixture import app_schema

schema,version,_=app_schema()
trial=json.loads((proposal.OUT/'index-trial-literal-results.json').read_text(encoding='utf-8'))
reference=sqlite3.connect(':memory:')
fresh=sqlite3.connect(':memory:')
upgraded=sqlite3.connect(':memory:')
reference.executescript(schema)
reference.execute(trial['index_sql'])

# Mirror the app's include + -- ! splitting for the proposed initialization file.
for chunk in re.split(r'-- !\r?\n',proposal.changes['resources/sql/db_init_sqlite.sql']):
    if chunk.startswith('!!'): fresh.executescript(schema)
    elif chunk: fresh.execute(chunk)
fresh.execute("INSERT INTO Information VALUES ('schema_version','106')")
upgraded.executescript(schema)
upgraded.execute("INSERT INTO Information VALUES ('schema_version','105')")
upgraded.execute(proposal.changes['resources/sql/db_update_sqlite_105_106.sql'])
upgraded.execute("UPDATE Information SET inf_value='106' WHERE inf_key='schema_version'")

def index_sql(db):
    sql=db.execute("SELECT sql FROM sqlite_master WHERE name='idx_Messages_feed_display'").fetchone()[0]
    return re.sub(r'\s+','',sql.replace('IF NOT EXISTS','')).rstrip(';')
def schema_rows(db):
    return db.execute('SELECT type,name,tbl_name,sql FROM sqlite_master ORDER BY type,name').fetchall()
def index_info(db):
    return db.execute('PRAGMA index_xinfo(idx_Messages_feed_display)').fetchall()

before=schema_rows(upgraded)
upgraded.execute(proposal.changes['resources/sql/db_update_sqlite_105_106.sql'])
result=dict(
    physical_index_definition_matches_measured=index_sql(reference)==index_sql(fresh)==index_sql(upgraded),
    index_columns_collation_order_matches=index_info(reference)==index_info(fresh)==index_info(upgraded),
    new_and_upgraded_schema_identical=schema_rows(fresh)==schema_rows(upgraded),
    retry_does_not_change_schema=before==schema_rows(upgraded),
    index_xinfo=index_info(fresh),
    database_scope='In-memory only; application files and huge fixture untouched.',
)
assert all(result[k] for k in list(result)[:4]),result
(proposal.OUT/'proposal-integration-checks.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result,indent=2))
