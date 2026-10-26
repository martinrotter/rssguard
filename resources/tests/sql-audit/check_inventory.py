"""Cross-check the manual inventory against SQL input operations in first-party C++."""
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
inventory = json.loads((HERE / 'query-inventory.json').read_text(encoding='utf-8'))
entries = inventory['queries'] + inventory['deferred_mariadb_only']
covered = set()
for entry in entries:
    for line in [entry['line'], entry.get('prepare_line'), entry.get('variable_expression_line'),
                 *entry.get('execution_lines', [])]:
        if line: covered.add((entry['file'], line))
    if entry['file'].endswith('.sql'):
        for caller in entry['callers']:
            covered.add((caller['file'], caller['line']))
unmatched, counts = [], {}
for file in sorted(ROOT.joinpath('src').rglob('*')):
    if file.suffix not in {'.cpp','.h'} or '3rd-party' in file.parts: continue
    name = file.relative_to(ROOT).as_posix()
    if name.endswith('/sqlquery.cpp'): continue  # Generic wrapper, not a SQL generator.
    source = file.read_text(encoding='utf-8-sig')
    # Preserve offsets/newlines while ignoring comments; SQL strings remain intact.
    clean = re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*',
                   lambda m: m[0] if m[0].startswith('"') else re.sub(r'[^\n]', ' ', m[0]),
                   source, flags=re.S)
    variables = set(re.findall(r'\b(?:SqlQuery|QSqlQuery)\s*&?\s*(\w+)',clean))
    count = 0
    for match in re.finditer(r'(?P<v>\w+)\s*\.\s*(?P<op>prepare|exec)\s*\(',clean):
        if match['v'] not in variables: continue
        tail = clean[match.end():].lstrip()
        if match['op'] == 'exec' and re.match(r'(?:true|false)?\s*\)',tail): continue
        line = clean.count('\n',0,match.start())+1
        count += 1
        if (name,line) not in covered: unmatched.append(dict(file=name,line=line,variable=match['v'],operation=match['op']))
    if count: counts[name] = count
result = dict(input_sql_operations=sum(counts.values()),per_file=counts,unmatched=unmatched,
              limitations='Static input-operation cross-check complements manual whole-file review. It proves coverage of these execution/generation sites, not every possible user SQL text or driver internal SQL.')
(HERE / 'inventory-coverage.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result,indent=2))
if unmatched: raise SystemExit(1)
