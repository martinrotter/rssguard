"""Combine the reviewed source inventories and freeze the audit order (no SQL runs)."""
from __future__ import annotations
import csv
import hashlib
import json
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RESULTS = ROOT / 'benchmark-results/sql-audit-20261014'


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def source_order(entry):
    # Common library first, then plugins; stable source path and line ordering.
    return (0 if entry['file'].startswith('src/librssguard/') else 1,
            entry['file'], entry['line'], entry['id'])


def main():
    entries = []
    for name in ('core-inventory.json', 'outer-inventory.json'):
        entries.extend(json.loads((HERE / name).read_text(encoding='utf-8')))
    # Definitions/builders and execution sinks can be on different source lines.
    for e in entries:
        if e['id'] in {'O044','O045'}: e['prepare_line'] = 202; e['execution_lines'] = [209]
        if e['id'] == 'O051': e['execution_lines'] = [209]
        if e['id'] == 'O053': e['execution_lines'] = [497]
        if e['id'] == 'O054': e['prepare_line'] = 546; e['execution_lines'] = [552]
        if e['id'] == 'CORE-039': e['variable_expression_line'] = 1169
    sqlite = sorted([e for e in entries if 'sqlite' in e['dialects']], key=source_order)
    maria_only = [e for e in entries if 'sqlite' not in e['dialects']]
    fragments = json.loads((HERE / 'core-fragments.json').read_text(encoding='utf-8'))
    sources = {e['file'] for e in entries + fragments}
    hashes = {s: hashlib.sha256((ROOT / s).read_bytes()).hexdigest() for s in sorted(sources)}
    data = {
        'scope': 'Application-authored SQLite SQL, including init/migrations, legacy import source SQL, dynamic generators and Qt transaction sites.',
        'schema_version': 105,
        'completeness': 'Every identified application execution site and structural generator is listed. Arbitrary user WHERE clauses and variable values/lists are infinite families, not enumerable concrete texts. Vendored libraries and developer test SQL excluded. Qt driver internal SQL not claimed to be exhaustively traced.',
        'counts': {'sqlite_query_site_entries': len(sqlite), 'sql_fragment_builders': len(fragments),
                   'mariadb_only_deferred_entries': len(maria_only)},
        'source_sha256': hashes, 'queries': sqlite, 'fragments': fragments,
        'deferred_mariadb_only': maria_only,
    }
    write_json(HERE / 'query-inventory.json', data)
    inventory_hash = hashlib.sha256((HERE / 'query-inventory.json').read_bytes()).hexdigest()
    lines = ['RSS Guard SQLite SQL inventory', '=' * 31, '',
             f"{len(sqlite)} query/site entries; {len(fragments)} fragment builders; {len(maria_only)} MariaDB-only entries deferred.",
             data['completeness'], '', 'SQL, execution site, bindings, dynamic branches and application callers follow.', '']
    for e in sqlite + fragments:
        lines += [f"[{e['id']}] {e['function']}", f"Source: {e['file']}:{e['line']}",
                  'Scope: ' + e.get('database_scope', 'current-schema-runtime'),
                  'SQL:', e.get('dialect_sql', {}).get('sqlite', e['sql_template']),
                  'Dynamic construction: ' + e['dynamic_details']]
        for key in ('execution_lines','bindings','dynamic_variants','filter_variants','sort_fields',
                    'selection_callsites','default_order_by','default_limit','source_expression'):
            if key in e:
                lines += [key + ':', json.dumps(e[key], ensure_ascii=False, indent=2)]
        lines.append('Application callers:')
        for caller in e['callers']:
            lines.append(f"  {caller['file']}:{caller['line']} " + (caller.get('function') or caller.get('source') or ''))
        lines += ['', '-' * 80, '']
    lines += ['Completeness notes', '=' * 18, '', (HERE / 'core-notes.txt').read_text(encoding='utf-8'),
              (HERE / 'outer-notes.txt').read_text(encoding='utf-8')]
    (HERE / 'query-inventory.txt').write_text('\n'.join(lines), encoding='utf-8')
    with (HERE / 'query-inventory.csv').open('w', encoding='utf-8-sig', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['id','source','function','scope','sqlite_sql_template','dynamic_details','call_sites'])
        for e in sqlite:
            writer.writerow([e['id'],f"{e['file']}:{e['line']}",e['function'],e.get('database_scope','current-schema-runtime'),
                             e.get('dialect_sql',{}).get('sqlite',e['sql_template']),e['dynamic_details'],
                             '\n'.join(f"{c['file']}:{c['line']}" for c in e['callers'])])

    model = next(e for e in sqlite if e['function'] == 'MessagesModelSqlLayer::fetchMessages')
    cases = []
    def model_case(name, predicate, order=None, suffix='', **extra):
        cases.append(dict(id=model['id'] + '/' + name, inventory_id=model['id'],
                          source=f"{model['file']}:{model['line']}", scenario=name,
                          sql=f"SELECT {model['projection']} FROM Messages WHERE {predicate} {order or model['default_order_by']} {suffix};",
                          direct_exec=True, bindings={}, threshold_ms=250, mutation=False, **extra))
    model_case('empty-selection','0 > 1')
    model_case('ordinary-feed-default-all','Messages.feed = 6000 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0')
    model_case('hot-feed-default-all','Messages.feed = 1 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0')
    model_case('hot-feed-lazy-first-page','Messages.feed = 1 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0',suffix='LIMIT 10000')
    model_case('hot-feed-lazy-deep-page','Messages.feed = 1 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0',suffix='LIMIT 10000 OFFSET 100000')
    substitutions = dict(account_id=1,label_id=1,escaped_regex='security|science',
                         user_authored_where="Messages.title LIKE '%security%'",feed_id=1,
                         comma_separated_feed_ids='1,2,3,4,5')
    for name,predicate in model['filter_variants'].items():
        if name in {'empty','feed'}: continue
        model_case('filter-' + name,predicate.format(**substitutions))
    for field in model['sort_fields']:
        for direction in ('ASC','DESC'):
            order = f'ORDER BY {field} {direction}'
            if field != 'Messages.id': order += f', Messages.id {direction}'
            model_case('sort-' + field.replace('Messages.','') + '-' + direction,
                       'Messages.feed = 1 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0',
                       order=order,suffix='LIMIT 10000')
    # Remaining query families remain explicitly in the queue. Their fixture-bound
    # scenario expansion is a separate step if the first family passes/gets approved.
    queue = [dict(id=e['id'],source=f"{e['file']}:{e['line']}",function=e['function'],
                  status='represented_by_cases_above' if e['id']==model['id'] else 'not_reached',
                  scope=e.get('database_scope','current-schema-runtime')) for e in sqlite]
    manifest = dict(order='Common-library runtime queries by source path/line, then plugins. For each family: empty -> ordinary -> hot-volume -> structural variants. Model first; no later families execute after a slow case.',
                    stop_rule='Stop at first case whose median of three repeated full-result samples >=250ms. The first sample is additional context; OS cache is not flushed. Repetition is confined to that case.',
                    inventory_sha256=inventory_hash, cases=cases, inventory_queue=queue,
                    limitations='Legacy importer reads and historical migrations require separate matching source/pre-migration schemas, so they cannot be run verbatim on schema105. Init SQL is exercised by fixture creation. Arbitrary user predicates are representative samples only. Later scenario materialization is pending because this pass stops at the first finding.')
    write_json(RESULTS / 'benchmark-plan.json', manifest)
    print(json.dumps({'counts':data['counts'],'scopes':dict(Counter(e.get('database_scope','current-schema-runtime') for e in sqlite)),
                      'first_family_cases':len(cases),'inventory_sha256':inventory_hash},indent=2))


if __name__ == '__main__': main()
