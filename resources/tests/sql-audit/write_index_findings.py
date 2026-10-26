"""Render measured index-only results; no database access or app changes."""
import csv
import hashlib
import json
import statistics
from pathlib import Path

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
OUT=ROOT/'benchmark-results/sql-audit-20261014'
d=json.loads((OUT/'index-trial-literal-results.json').read_text(encoding='utf-8'))
assert d['status']=='complete_rolled_back'
assert all(d[key] for key in ('equivalence_passed','edge_updates_equivalent','trial_index_removed','trial_rows_removed','baseline_restored'))
assert d['original_fingerprint']==d['indexed_fingerprint']==d['baseline_restored_fingerprint']
assert not d['lazy_loading_changed'] and not d['limit_added']
assert not any('TEMP B-TREE' in p['detail'] for p in d['indexed_plan'])
assert d['baseline_plan']==d['interleaved_baseline_plan']
before=d['baseline_stats']['median_ms']; after=d['indexed_stats']['median_ms']
percent=(1-after/before)*100
write_before=d['baseline_write_stats']['median_ms']; write_after=d['indexed_write_stats']['median_ms']
paired=[(1-b['total_ms']/a['total_ms'])*100 for a,b in zip(d['interleaved_baseline_samples'],d['interleaved_indexed_samples'])]
lines=[
    'Measured SQLite index-only proposal - awaiting approval', '='*55, '',
    'Lazy loading stays optional and OFF by default. The application SELECT remains unchanged:',
    'no LIMIT, identical 18 columns, label aggregation, filters, and default ordering.',
    'Application code/schema/settings have not been changed. The synthetic trial was rolled back.', '',
    'Proposed SQL', '------------',
    'CREATE INDEX idx_Messages_feed_display',
    'ON Messages (feed, date_created DESC, title ASC, id DESC)',
    'WHERE is_deleted = 0 AND is_pdeleted = 0;', '',
    'Read measurements', '-----------------',
    f'Median full-result retrieval: {before:.3f} -> {after:.3f} ms ({percent:.2f}% lower; {before/after:.3f}x throughput).',
    f'Savings: {before-after:.3f} ms per measured full feed load; all {d["original_fingerprint"]["rows"]:,} articles returned.',
    f'Baseline range: {d["baseline_stats"]["min_ms"]:.3f}-{d["baseline_stats"]["max_ms"]:.3f} ms.',
    f'Indexed range: {d["indexed_stats"]["min_ms"]:.3f}-{d["indexed_stats"]["max_ms"]:.3f} ms.',
    f'All six pairs improved: {min(paired):.2f}-{max(paired):.2f}%; median paired reduction {statistics.median(paired):.2f}%.',
    'Six interleaved pairs on one connection and the same 2M-article database; order alternates A/B and B/A.',
    'A replays the original idx_Messages3 plan using an INDEXED BY hint. That hint is ONLY in the benchmark.',
    'B uses the exact original SQL, with the optimizer choosing the proposed index normally.',
    'The replay plan matches the original pre-index plan; ordered full-value fingerprints also match.',
    f'Qt {d["qt_version"]}, SQLite {d["sqlite_version"]}, forwardOnly=true; same source PRAGMAs as first audit.',
    'Timings include q.exec and retrieval of every value. Hashing is separate and untimed.',
    'They exclude Message conversion/retention, worker scheduling and GUI work; OS cache is not flushed.',
    'The older first-pass 2.72s median is historical context; the speedup above uses contemporaneous A/B runs.',
    'This improves the query but it remains above the initial 250ms interactive threshold.', '',
    'Pairs (original plan ms -> index ms)',
    *[f'  {i+1}: {a["total_ms"]:.3f} -> {b["total_ms"]:.3f}' for i,(a,b) in enumerate(zip(d['interleaved_baseline_samples'],d['interleaved_indexed_samples']))], '',
    'Correctness checks', '------------------',
    'PASS: all 235,454 rows, all 18 QVariant values including NULL/type and complete row ordering match.',
    'Untimed SHA-256 streams serialize each projected QVariant using Qt QDataStream, without retaining bodies.',
    f'Full ordered-value digest: {d["original_fingerprint"]["ordered_all_values_sha256"]}.',
    'PASS: populated feed 750 plus eight transaction-local edge rows; mixed-case Alpha/alpha title ties,',
    'equal dates, id DESC tie-breaker, differing read flags, NULL/empty fields and zero/one/two labels.',
    'PASS: all four is_deleted/is_pdeleted combinations; only visible rows included.',
    'PASS: date/title/status changes affecting ordering and membership, compared with original planner choice.',
    'PASS: after rollback, trial index/data removed and original full-feed fingerprint restored.', '',
    'Query plan', '----------',
    'Before: idx_Messages3 feed/status lookup + USE TEMP B-TREE FOR ORDER BY.',
    'After: idx_Messages_feed_display feed lookup; no temporary ORDER BY tree.',
    'The correlated label subquery remains unchanged in both plans.', '',
    'Costs measured', '--------------',
    f'Index creation: {d["index_build_ms"]:.3f} ms (~{d["index_build_ms"]/1000:.2f}s).',
    f'Incremental allocated database pages: {d["index_allocated_bytes"]:,} bytes (~{d["index_allocated_bytes"]/1024**2:.2f} MiB).',
    'Allocation is a page-count/freelist estimate and includes the tiny edge fixture; not an exact per-index dbstat total.',
    f'1000-article bulk INSERT median: {write_before:.3f} -> {write_after:.3f} ms (+{write_after-write_before:.3f} ms, +{(write_after/write_before-1)*100:.2f}%).',
    'One warmup then four samples per write configuration, using actual application 15-column literal bulk SQL.',
    'Writes run in rolled-back savepoints. SQL generation, commit and rollback are outside timing.',
    'This measures insertion/index-maintenance cost; it is not a complete autocommit refresh benchmark.',
    'Update and deletion index maintenance remains a cost; quantified write evidence here covers insertions only.', '',
    'Approval scope', '--------------',
    'Add the measured SQLite partial index to new-DB initialization and a current-DB migration.',
    'Keep existing message query/projection/order and optional lazy-loading default unchanged.',
    'No hard-coded INDEXED BY hint should be added to production code.',
    'No C++ refactoring is proposed in this finding. Later query families remain stopped pending approval.',
    'Future proposals must be SQL-only or small C++ speedups, measured with result equivalence before approval.', '',
    'Evidence files', '--------------',
    'index-trial-literal-results.json: raw read/write samples, plans, fingerprints, cleanup checks.',
    'index-trial-samples.csv: individual measured samples.',
    'proposed-fix.sql: index-only SQL for review.',
    'resources/tests/sql-audit/qt_index_trial.cpp: reproducible native Qt trial; all changes rollback.',
    'resources/tests/sql-audit/query-inventory.txt: unchanged complete SQLite inventory.',
]
(OUT/'index-only-finding.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
with (OUT/'index-trial-samples.csv').open('w',encoding='utf-8',newline='') as stream:
    writer=csv.DictWriter(stream,fieldnames=['kind','configuration','sample','total_ms','rows'])
    writer.writeheader()
    for kind,config,key in [('read','original','interleaved_baseline_samples'),('read','index','interleaved_indexed_samples'),
                            ('insert_1000','original','baseline_write_samples'),('insert_1000','index','indexed_write_samples')]:
        for sample in d[key]:
            writer.writerow(dict(kind=kind,configuration=config,**{k:sample[k] for k in ('sample','total_ms','rows')}))
hashes={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest()
        for path in [HERE/'qt_index_trial.cpp',HERE/'qt_benchmark.cpp',OUT/'index-trial-literal-results.json']}
(OUT/'index-trial-source-hashes.json').write_text(json.dumps(hashes,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(before_ms=before,after_ms=after,reduction_percent=percent,
                      write_before_ms=write_before,write_after_ms=write_after,
                      index_bytes=d['index_allocated_bytes'],report=str(OUT/'index-only-finding.txt')),indent=2))
