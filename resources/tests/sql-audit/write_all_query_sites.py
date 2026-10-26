"""Add the already-reviewed MariaDB-only sites without altering audit-order hashes."""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
data = json.loads((HERE / 'query-inventory.json').read_text(encoding='utf-8'))
deferred = data['deferred_mariadb_only']
lines = [(HERE / 'query-inventory.txt').read_text(encoding='utf-8'), '',
         'MariaDB-only application SQL sites (inventory only)', '=' * 49, '',
         'The initial audit targets SQLite. These additional backend sites are listed here for',
         'completeness; they are not part of a completed general MariaDB query-performance audit.',
         'Proposed improvements to the first slow family are measured on BOTH backends.', '']
for e in deferred:
    lines += [f'[{e["id"]}] {e["function"]}', f'Source: {e["file"]}:{e["line"]}',
              'SQL:', e.get('dialect_sql', {}).get('mariadb', e['sql_template']),
              'Dynamic construction: ' + e['dynamic_details']]
    for key in ('execution_lines', 'bindings', 'dynamic_variants', 'source_expression'):
        if key in e:
            lines += [key + ':', json.dumps(e[key], ensure_ascii=False, indent=2)]
    lines.append('Application callers:')
    for c in e['callers']:
        lines.append(f'  {c["file"]}:{c["line"]} ' + (c.get('function') or c.get('source') or ''))
    lines += ['', '-' * 80, '']
path = HERE / 'all-query-sites.txt'
path.write_text('\n'.join(lines), encoding='utf-8')
print(json.dumps({'all_query_site_entries': len(data['queries']) + len(deferred),
                  'fragment_builders': len(data['fragments']), 'path': str(path)}, indent=2))
