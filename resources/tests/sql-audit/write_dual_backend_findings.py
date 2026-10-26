"""Render the first finding after both backend trials; no database access."""
import csv
import hashlib
import json
import statistics
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = ROOT / 'benchmark-results/sql-audit-20261014'


def read(name):
    return json.loads((OUT / name).read_text(encoding='utf-8'))


def checked_result(name, status):
    value = read(name)
    assert value['status'] == status, (name, value['status'])
    for key in ('equivalence_passed', 'edge_updates_equivalent', 'trial_index_removed',
                'trial_rows_removed', 'baseline_restored'):
        assert value[key], (name, key)
    assert value['original_fingerprint'] == value['indexed_fingerprint'] == value['baseline_restored_fingerprint']
    assert not value['lazy_loading_changed'] and not value['limit_added']
    return value


def result_lines(label, value):
    before, after = (value[key]['median_ms'] for key in ('baseline_stats', 'indexed_stats'))
    wb, wa = (value[key]['median_ms'] for key in ('baseline_write_stats', 'indexed_write_stats'))
    pairs = [(1 - b['total_ms'] / a['total_ms']) * 100 for a, b in
             zip(value['interleaved_baseline_samples'], value['interleaved_indexed_samples'])]
    return [label, '-' * len(label),
            f'Median full retrieval: {before:.3f} -> {after:.3f} ms ({(1-after/before)*100:.2f}% lower).',
            f'Baseline range: {value["baseline_stats"]["min_ms"]:.3f}-{value["baseline_stats"]["max_ms"]:.3f} ms.',
            f'Candidate range: {value["indexed_stats"]["min_ms"]:.3f}-{value["indexed_stats"]["max_ms"]:.3f} ms.',
            f'Six balanced pairs: {min(pairs):.2f}-{max(pairs):.2f}% lower; median paired reduction {statistics.median(pairs):.2f}%.',
            f'1000-row literal INSERT: {wb:.3f} -> {wa:.3f} ms ({wa-wb:+.3f} ms; {(wa/wb-1)*100:+.2f}%).',
            f'Index creation: {value["index_build_ms"]:.3f} ms.',
            'Pairs (baseline ms -> candidate ms):',
            *[f'  {i+1}: {a["total_ms"]:.3f} -> {b["total_ms"]:.3f}' for i, (a, b) in
              enumerate(zip(value['interleaved_baseline_samples'], value['interleaved_indexed_samples']))],
            f'Ordered all-value SHA256: {value["original_fingerprint"]["ordered_all_values_sha256"]}.', '']


def main():
    sqlite = checked_result('index-trial-literal-results.json', 'complete_rolled_back')
    maria = checked_result('maria/mariadb-force-index-trial-results.json', 'complete_explicit_cleanup')
    unforced = checked_result('maria/mariadb-index-trial-results.json', 'complete_explicit_cleanup')
    controls = read('maria/mariadb-representative-feed-controls.json')
    assert controls['status'] == 'complete_representative_controls_explicit_cleanup'
    for flag in ('baseline_restored', 'trial_index_removed', 'trial_rows_removed', 'auto_increment_restored',
                 'trial_label_links_removed', 'message_count_restored'):
        assert controls[flag]
    for case in controls['representative_feed_cases']:
        assert case['equivalence_passed'] and case['restored']
        assert case['original_fingerprint'] == case['candidate_fingerprint'] == case['restored_fingerprint']
        assert case['candidate_chosen_plan']['key'].lower() == 'idx_messages_feed_display'
        assert 'filesort' not in case['candidate_chosen_plan']['Extra'].lower()
    assert maria['force_candidate'] and maria['auto_increment_restored']
    assert maria['trial_label_links_removed'] and maria['message_count_restored']
    assert unforced['trial_label_links_removed'] and unforced['message_count_restored'] and unforced['auto_increment_restored']
    assert maria['indexed_chosen_plan']['key'].lower() == 'idx_messages_feed_display'
    assert 'filesort' not in maria['indexed_chosen_plan']['Extra'].lower()
    assert not any('TEMP B-TREE' in p['detail'] for p in sqlite['indexed_plan'])
    assert sqlite['baseline_plan'] == sqlite['interleaved_baseline_plan']
    assert maria['baseline_chosen_plan'] == maria['interleaved_baseline_chosen_plan']
    label_plan = lambda plan: [{k: row[k] for k in ('table', 'key', 'type', 'Extra')}
                              for row in plan if row['table'].lower() != 'messages']
    assert label_plan(maria['baseline_plan']) == label_plan(maria['indexed_plan'])
    integration = read('dual-backend-proposal-integration-checks.json')
    assert integration['patch_sha256'] == hashlib.sha256((OUT / 'dual-backend-proposal.patch').read_bytes()).hexdigest()
    assert integration['sqlite']['status'] == integration['mariadb']['status'] == 'complete'
    cpp = read('cpp-proposal-checks.json')
    assert cpp['status'] == 'passed'
    for backend in (sqlite, maria):
        assert backend['original_fingerprint']['columns'] == 18
        assert backend['original_fingerprint']['rows'] == 235454
        assert backend['indexed_stats']['median_ms'] < backend['baseline_stats']['median_ms']
    lines = [
        'First slow query: measured SQLite + MariaDB proposal, awaiting approval', '=' * 69, '',
        'Application files have not been changed. Later query families remain stopped.',
        'Lazy loading remains optional and OFF by default. All matching rows are retrieved;',
        'no LIMIT, reduced projection, pagination, or setting change is proposed.', '',
        'Finding and fixture', '-------------------',
        'Call site: src/librssguard/core/messagesmodelsqllayer.cpp:106; builder:194.',
        'Canonical single visible feed; ORDER BY date_created DESC,title ASC,id DESC.',
        'The first baseline exceeded the 250ms stop threshold (SQLite first-audit median 2716.691ms).',
        'Both schema105 fixtures contain 2,000,000 messages, 6000 feeds, 8 accounts and 300,000 label links.',
        'The measured large feed returns 235,454 rows with all 18 columns and native label aggregation.',
        'Every logical table row hash matches between the fixtures. SQLite has simulated append,',
        'edit, soft-delete and physical-purge history; MariaDB imported its logical state.',
        'Physical InnoDB fragmentation/history was not reproduced by that import.', '',
        'Exact change', '------------',
        'SQLite, fresh initialization and 105->106 migration:',
        'CREATE INDEX IF NOT EXISTS idx_Messages_feed_display',
        'ON Messages (feed,date_created DESC,title ASC,id DESC)',
        'WHERE is_deleted=0 AND is_pdeleted=0;', '',
        'MariaDB, fresh initialization and 105->106 migration:',
        'CREATE INDEX IF NOT EXISTS idx_Messages_feed_display',
        'ON Messages (feed,is_deleted,is_pdeleted,date_created DESC,title ASC,id DESC);', '',
        'SQLite keeps its SELECT unchanged and selects the partial index automatically.',
        'MariaDB needs FROM Messages FORCE INDEX (idx_Messages_feed_display) for the measured SELECT.',
        'The small C++ guard enables this hint only after startup verifies the physical index:',
        'full columns, ascending equality columns, date DESC/title ASC/id DESC, usable BTREE.',
        'It also requires exactly the canonical positive single-feed predicate, default sort,',
        'no additional-article OR, no LIMIT and zero offset. Other shapes retain the existing query.',
        'Capability is cached during initial schema setup before the database worker starts.',
        'There is no per-view capability query. Missing/unsupported index metadata falls back.',
        'Only the exact optional MariaDB index DDL may fail without aborting schema initialization;',
        'that failure is logged. All other schema errors retain their existing failure behavior.',
        'A hinted SELECT whose index disappears/gets ignored (error1176) retries the original query.',
        'Review-only integration: dual-backend-proposal.patch (not applied).', '',
        'Integration validation', '----------------------',
        'PASS: new schema, 105->106 upgrade and retry on SQLite and isolated MariaDB empty schemas.',
        'PASS: physical index columns/directions/collation match the measured indexes.',
        'PASS: exact startup metadata SQL returns the usable signature on fresh/upgraded MariaDB schemas.',
        'PASS: real ignored-index control disables capability, forced use raises1176 and unhinted SQL succeeds.',
        'PASS: disposable integration schemas removed; ordinary git apply --check passes.',
        'PASS: actual proposed database/model translation units compile in a disposable overlay.',
        f'PASS: {cpp["checks_passed"]} native C++ guard/query-generation checks.',
        'A complete application build/GUI test was not run for the unapplied proposal.',
        'Approximate native render-only overhead (ns/call; upper-middle of six samples per variant):',
        'The isolated render executable omitted production QStringBuilder defines; these tiny',
        'overheads are approximate controls, not a full application rendering benchmark.',
        *[f'  {v["mode"]}: {v["original_median_ns_per_call"]:.1f} -> '
          f'{v["proposed_median_ns_per_call"]:.1f} ns ({v["delta_ns_per_call"]:+.1f} ns).'
          for v in cpp['render_only_overhead']], '',
    ]
    lines += result_lines('SQLite native QSQLITE', sqlite)
    lines += [f'Index allocation estimate: {sqlite["index_allocated_bytes"]:,} bytes '
              f'({sqlite["index_allocated_bytes"]/1024**2:.2f} MiB); page/freelist delta, includes tiny edge fixture.', '']
    lines += result_lines('MariaDB native QMYSQL with guarded hint', maria)
    lines += ['MariaDB index size measurements: ' + json.dumps(maria['index_size'], sort_keys=True), '',
              'MariaDB representative single-feed controls', '------------------------------------------',
              'Six balanced pairs per feed, every value/order equal, forced index/no filesort verified:',
              *[f'  Feed {c["feed"]}, {c["original_fingerprint"]["rows"]:,} rows: '
                f'{c["baseline_stats"]["median_ms"]:.3f} -> {c["candidate_stats"]["median_ms"]:.3f} ms.'
                for c in controls['representative_feed_cases']],
              'Empty-feed difference is noise; no material regression in these controls.', '',
              'Rejected unforced MariaDB variant', '---------------------------------',
              f'Index alone: {unforced["baseline_stats"]["median_ms"]:.3f} -> '
              f'{unforced["indexed_stats"]["median_ms"]:.3f} ms; no meaningful improvement.',
              'It retains ALL/Using filesort. The recommended MariaDB variant includes the measured hint.', '',
              f'MariaDB INSERT variability: the earlier unforced trial was '
              f'{unforced["baseline_write_stats"]["median_ms"]:.3f} -> '
              f'{unforced["indexed_write_stats"]["median_ms"]:.3f} ms with the same physical index.',
              'The latest slight write decrease is not evidence that adding an index speeds insertion;',
              'pre/post write samples are not interleaved and include cache/run variability.', '',
              'Evidence and limits', '-------------------',
              f'Qt {sqlite["qt_version"]}; SQLite {sqlite["sqlite_version"]}; '
              f'MariaDB {maria["profile"]["server_version"]}.',
              'Native Qt SQL, forwardOnly=true, direct exec and retrieval of every projected value.',
              'QMYSQL buffers complete results internally; this matches its application driver behavior.',
              'Six alternating A/B and B/A pairs on each backend, matched pre-index execution plans.',
              'Full EXPLAIN records include unchanged dependent label lookups; inspect raw results for estimates.',
              'Hashing is separate and untimed. Timings exclude Message construction/retention, worker',
              'scheduling and GUI work; OS cache is not flushed. These are measured query costs, not UI latency.',
              'Each backend is compared to its own native collation/order/type baseline, not the other engine.',
              'Both: all values/order, NULL/type, mixed-case ASCII title ties, tie-breaking IDs,',
              'zero/one/two labels, all deletion flags and date/title/status updates match.',
              'MariaDB additionally verifies native Unicode and trailing-space title ties.',
              'SQLite trial rolled back; MariaDB DDL committed and was explicitly removed.',
              'Original full-result hashes/counts and MariaDB AUTO_INCREMENT restored.',
              'INSERT timing includes execution/index maintenance; excludes SQL generation, commit and rollback.',
              'Update/delete maintenance and durable refresh latency were not performance-measured.',
              'Both improved full-feed queries remain above the 250ms interactive threshold.',
              'MariaDB measurements cover 11.8.8, InnoDB DYNAMIC/16KiB, utf8mb4_unicode_ci, 512MiB buffer pool.',
              'Older versions/formats are not performance-tested; ignored DESC metadata disables the hint.',
              'The full utf8mb4 title key requires sufficient InnoDB key-length capacity.',
              'If optional index creation fails, schema106 can still be installed with the baseline query.',
              'A later server/row-format upgrade does not automatically retry that skipped index creation.',
              'Schema version becomes106. Builds requiring<=105 cannot open upgraded databases.', '',
              'Files', '-----',
              'resources/tests/sql-audit/all-query-sites.txt: 191 SQL/site entries +9 builders, both backends.',
              'index-trial-literal-results.json: SQLite raw samples/plans/hash/cleanup.',
              'maria/mariadb-force-index-trial-results.json: measured hinted MariaDB candidate.',
              'maria/mariadb-index-trial-results.json: rejected automatic MariaDB index-only candidate.',
              'maria/mariadb-representative-feed-controls.json: smaller and empty canonical single-feed controls.',
              'dual-backend-proposal.patch: exact review-only schema and C++ integration.',
              'dual-backend-proposal-integration-checks.json and cpp-proposal-checks.json: integration/guard verification.', '']
    (OUT / 'dual-backend-finding.txt').write_text('\n'.join(lines), encoding='utf-8')
    with (OUT / 'dual-backend-samples.csv').open('w', newline='', encoding='utf-8') as stream:
        writer = csv.writer(stream)
        writer.writerow(['backend', 'kind', 'configuration', 'sample', 'total_ms', 'rows'])
        for name, d in [('SQLite', sqlite), ('MariaDB', maria)]:
            for kind, cfg, key in [('read', 'baseline', 'interleaved_baseline_samples'),
                                   ('read', 'candidate', 'interleaved_indexed_samples'),
                                   ('insert_1000', 'baseline', 'baseline_write_samples'),
                                   ('insert_1000', 'candidate', 'indexed_write_samples')]:
                for sample in d[key]:
                    writer.writerow([name, kind, cfg, sample['sample'], sample['total_ms'], sample['rows']])
    paths = [OUT / 'index-trial-literal-results.json', OUT / 'maria/mariadb-force-index-trial-results.json',
             OUT / 'maria/mariadb-index-trial-results.json',
             OUT / 'maria/mariadb-representative-feed-controls.json', OUT / 'dual-backend-proposal.patch',
             OUT / 'maria/mariadb-index-trial-provenance.json',
             OUT / 'maria/mariadb-force-index-trial-provenance.json',
             OUT / 'maria/mariadb-representative-feed-controls-provenance.json',
             OUT / 'maria/unforced-harness-source.cpp', OUT / 'maria/forced-harness-source.cpp',
             OUT / 'maria/representative-controls-harness-source.cpp', OUT / 'maria/qt_benchmark.cpp',
             OUT / 'dual-backend-proposal-integration-checks.json', OUT / 'cpp-proposal-checks.json',
             OUT / 'cpp-compilation-checks.json',
             HERE / 'qt_index_trial.cpp', HERE / 'qt_benchmark.cpp']
    hashes = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    (OUT / 'dual-backend-evidence-hashes.json').write_text(json.dumps(hashes, indent=2) + '\n', encoding='utf-8')
    print(str(OUT / 'dual-backend-finding.txt'))


if __name__ == '__main__':
    main()
