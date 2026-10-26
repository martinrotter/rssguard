"""Write the first-finding evidence and proposal without opening a database."""
import csv
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = ROOT / 'benchmark-results/sql-audit-20261014'
report = json.loads((OUT / 'results.json').read_text(encoding='utf-8'))
inventory = json.loads((HERE / 'query-inventory.json').read_text(encoding='utf-8'))
fixture = json.loads((OUT / 'fixture_metadata.json').read_text(encoding='utf-8'))
assert report['status'] == 'stopped_at_first_potentially_slow_query'
assert hashlib.sha256((HERE / 'query-inventory.json').read_bytes()).hexdigest() == report['inventory_sha256']
assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == digest
           for name,digest in inventory['source_sha256'].items())
slow = report['results'][-1]
samples = slow['samples']
assert slow['status'] == 'potentially_slow'
index = '''CREATE INDEX IF NOT EXISTS idx_Messages_feed_display
ON Messages (feed, date_created DESC, title ASC, id DESC)
WHERE is_deleted = 0 AND is_pdeleted = 0;'''
def readable(sql):
    return sql.replace(', Messages.', ',\n       Messages.').replace(' FROM Messages WHERE ', '\nFROM Messages\nWHERE ').replace(' ORDER BY ', '\nORDER BY ')
(OUT / 'first-slow-query.sql').write_text('-- Measured application SQL; no changes applied.\n' + readable(slow['sql'])+'\n',encoding='utf-8')
(OUT / 'proposed-fix.sql').write_text(
    '-- INDEX-ONLY PROPOSAL. No production application/schema changes applied.\n'
    '-- SQLite-only index for the measured feed/default-sort query.\n' + index + '\n\n'
    '-- Original SELECT remains unchanged, without LIMIT.\n'
    '-- Lazy loading stays optional and defaults off.\n',encoding='utf-8')
coverage = []
for entry in inventory['queries']:
    state = 'not_benchmarked_before_stop'
    if entry['id'] == slow['inventory_id']: state = 'partially_benchmarked_first_slow_variant_found'
    if entry.get('database_scope') == 'current-schema-initialization': state = 'executed_during_fixture_creation_not_latency_benchmarked'
    if entry.get('database_scope','').startswith('legacy-'): state = 'requires_separate_legacy_source_schema_not_reached'
    if entry.get('database_scope') == 'historical-migration': state = 'requires_pre_migration_schema_not_reached'
    coverage.append(dict(id=entry['id'],source=f"{entry['file']}:{entry['line']}",function=entry['function'],status=state))
(OUT / 'coverage-after-stop.json').write_text(json.dumps(coverage,indent=2)+'\n',encoding='utf-8')
with (OUT / 'samples.csv').open('w',encoding='utf-8',newline='') as stream:
    writer = csv.DictWriter(stream,fieldnames=['case_id','sample','cache','total_ms','exec_including_prepare_ms','rows','text_characters'])
    writer.writeheader()
    for result in report['results']:
        for sample in result['samples']:
            writer.writerow({'case_id':result['id'],**{k:sample[k] for k in writer.fieldnames if k!='case_id'}})
lines = [
    'RSS Guard SQLite performance audit - stopped for approval',
    '========================================================', '',
    f"FIRST FINDING: {slow['source']} ({slow['inventory_id']})",
    f"Selecting feed 1 using the default profile fetched {samples[0]['rows']:,} visible articles.",
    f"Median of three repeated full-result samples: {slow['median_repeated_ms']:.3f} ms.",
    f"Predeclared interactive threshold: {slow['threshold_ms']:.0f} ms; result {slow['median_repeated_ms']/slow['threshold_ms']:.2f} times over threshold.",
    'No later audit query families were executed. No production index/query/settings changes were applied.',
    'User scope: SQL-only or small C++ speedups; lazy loading remains optional/default off.',
    'Every proposed change must be measured before approval. See index-only-finding.txt for the trial.', '',
    'Inventory and scope', '-------------------',
    '172 application SQLite query/site entries: 124 current-schema runtime/setup/transaction sites,',
    '28 initialization statements, 7 historical migration statements, 13 legacy-source import queries.',
    'Nine shared SQL fragment builders, 49 core nested structural variants, and the message-list',
    'filter/sort/limit generators are included. Static cross-check: 139 SQL input operations, 0 unmatched.',
    '19 MariaDB-only entries deferred. Arbitrary user-written SQL WHERE clauses and variable values/list',
    'lengths are infinite families: the inventory records all their generators, not every possible text.',
    'Qt driver internal SQL is not claimed to be exhaustively traced; vendored/test-only SQL excluded.', '',
    'Fixture', '-------',
    f"File: {fixture['database']}",
    f"Size: {fixture['database_bytes']:,} bytes ({fixture['database_bytes']/1024**3:.2f} GiB).",
    'Current canonical schema 105; 2,000,000 articles; 8 accounts; 6,000 feeds; 192 labels; 300,000 label links.',
    '2016-2026 append/update/delete simulation: 2.05M insertions, 50,000 physical purges, 12,625 edits.',
    f"Freelist: {fixture['freelist_count']:,} pages; integrity_check=ok; foreign_key_check empty.",
    'No extra indexes, ANALYZE or VACUUM applied. All accounts are synthetic std-rss accounts.', '',
    'Measurement', '-----------',
    f"Qt {report['qt_version']}, QSQLITE SQLite {report['sqlite_version']}. Generator SQLite {fixture['sqlite_version']}; measurements use Qt's engine.",
    'Same source PRAGMAs: page 32768, cache 64 MiB, mmap 100 MB, synchronous OFF, temp MEMORY, WAL, FK ON, busy 5000 ms.',
    'Read-only connection; forwardOnly=true; direct q.exec(sql); all columns consumed until exhaustion.',
    'One first-in-connection sample plus three repetitions of that same SQL. OS cache was not flushed.',
    'Timings include SQL execution and retrieving each row/value; they exclude Message conversion,',
    'retaining the QList<Message>, worker scheduling and GUI work. This is SQL/retrieval evidence,',
    'not a measured full application/UI load time.', '',
    'Samples (ms;returned rows)',
    *[f"  {s['cache']}, run {s['sample']}: {s['total_ms']:.3f} ms; {s['rows']:,} rows; exec phase {s['exec_including_prepare_ms']:.3f} ms" for s in samples],
    f"Each run retrieved {samples[0]['text_characters']:,} text characters including article bodies.", '',
    'Measured query plan', '-------------------',
    *['  '+p['detail'] for p in slow['plan']],
    'The existing feed/status index narrows matching rows but does not supply the requested ordering.',
    'SQLite builds a temporary ORDER BY tree and performs a correlated label lookup per output row.',
    'The initial default query has no LIMIT. Lazy loading defaults false in settingskeys.cpp:165;',
    'messagesmodel.cpp:247 supplies limit 0; messagesview.cpp:508-510 supplies the default sort.',
    'Plan interpretation follows SQLite documentation: https://www.sqlite.org/eqp.html', '',
    'Proposed enhancement for approval', '---------------------------------',
    'Add this SQLite partial composite index through initialization and an existing-DB migration:',
    index,
    '   It is designed to supply the measured feed/default sort while indexing only visible rows.',
    '   The query continues transferring all 235,454 bodies, with the same values and ordering.',
    '   Index-only performance and correctness measurements are in index-only-finding.txt.',
    '   Other sorts/account/probe queries are outside this first finding and remain unbenchmarked.',
    '   Costs: additional index space and index maintenance on message insertion/updates/deletion.',
    'Planner references: https://www.sqlite.org/queryplanner.html; https://www.sqlite.org/partialindex.html',
    'Reviewable SQL is in proposed-fix.sql; the synthetic DB trial is rolled back after measurement.', '',
    'Approval is for the measured index-only change. Lazy loading and initial result size stay unchanged.',
    'After approval: apply the app SQLite migration/index change, then resume the remaining queue.', '',
    'Coverage limitation', '-------------------',
    'Before the finding, two empty-result controls completed. The planned ordinary-feed case used',
    'feed 6000, which returned 0 articles; it is an empty-feed control, not populated small-feed evidence.',
    'That mistake is preserved in raw results; it will be corrected to a populated feed after approval.',
    'Only one runtime SQL family was reached. 48 later model scenarios and every later runtime family',
    'remain unbenchmarked. Initialization ran as fixture creation; legacy imports/migrations need',
    'their matching source/previous schemas for later testing, not the current-schema fixture.', '',
    'Files', '-----',
    'resources/tests/sql-audit/query-inventory.txt:full readable inventory and callers.',
    'resources/tests/sql-audit/query-inventory.json:machine-readable templates/variants/bindings.',
    'resources/tests/sql-audit/inventory-coverage.json:source input-site cross-check.',
    'benchmark-plan.json:the frozen pre-benchmark order and exact SQL cases.',
    'results.json and samples.csv: raw measured samples and plans.',
    'first-slow-query.sql:the exact measured SQL.',
    'proposed-fix.sql: proposed index only; original SELECT has no LIMIT.',
    'coverage-after-stop.json: explicit status for every SQLite inventory entry.',
    'fixture_metadata.json: schema hashes, counts, states/history, settings and validation.',
]
(OUT / 'first-finding.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(json.dumps({'first_case':slow['id'],'median_ms':slow['median_repeated_ms'],'rows':samples[0]['rows'],
                  'report':str(OUT/'first-finding.txt'),'source_snapshot_matches':True},indent=2))
