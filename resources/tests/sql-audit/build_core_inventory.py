"""Inventory DatabaseQueries SQL without importing or modifying the application.

The literal scanner checks every prepare()/exec(SQL) source occurrence, then
hand-reviewed generator metadata resolves nonliteral expressions and branches.
This is an inventory helper, not the benchmark runner.
"""
import json
import re
from functools import lru_cache
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
FILES = [ROOT / 'src/librssguard/database/databasequeries.cpp',
         ROOT / 'src/librssguard/database/databasequeries.h']
STRING = re.compile(r'"(?:\\.|[^"\\])*"')


def scan_end(text, start, opening='(', closing=')'):
    """Balanced C++ delimiter scan, ignoring strings and comments."""
    depth = 0
    i = start
    while i < len(text):
        if text.startswith('//', i):
            end = text.find('\n', i)
            i = len(text) if end < 0 else end
        elif text.startswith('/*', i):
            end = text.find('*/', i + 2)
            i = len(text) if end < 0 else end + 2
        elif text[i] in ('"', "'"):
            quote = text[i]
            i += 1
            while i < len(text):
                if text[i] == '\\':
                    i += 2
                elif text[i] == quote:
                    i += 1
                    break
                else:
                    i += 1
        else:
            if text[i] == opening:
                depth += 1
            elif text[i] == closing:
                depth -= 1
                if depth == 0:
                    return i
            i += 1
    raise ValueError('Unbalanced source delimiter at ' + str(start))


def literal(text):
    # JSON string decoding agrees with the escape sequences present in these files.
    return ''.join(json.loads(match.group()) for match in STRING.finditer(text))


def leading_literal(expression):
    expression = expression.strip()
    if expression.startswith(('QSL(', 'QL1S(')):
        begin = expression.index('(')
        return literal(expression[begin + 1:scan_end(expression, begin)])
    if expression.startswith('"'):
        # Plain adjacent C++ string literals (the two wide UPDATE statements).
        return literal(expression)
    return None


texts = {path: path.read_text(encoding='utf-8') for path in FILES}
functions = []
for path, source in texts.items():
    for match in re.finditer(r'\bDatabaseQueries::(\w+)\s*\(', source):
        open_paren = source.index('(', match.start())
        end_paren = scan_end(source, open_paren)
        brace = source.find('{', end_paren)
        if brace < 0 or source[end_paren + 1:brace].strip():
            continue
        end_brace = scan_end(source, brace, '{', '}')
        functions.append({'path': path, 'name': match.group(1), 'start': match.start(),
                          'brace': brace, 'end': end_brace,
                          'line': source.count('\n', 0, match.start()) + 1,
                          'body': source[brace + 1:end_brace]})

by_name = {item['name']: item for item in functions}


APPLICATION_SOURCES = {}
for path in sorted((ROOT / 'src').rglob('*')):
    if path.suffix in ('.cpp', '.h', '.cc', '.hpp') and not any(
            p in ('3rdparty', '3rd-party', 'thirdparty', 'third-party') for p in path.parts):
        APPLICATION_SOURCES[path] = path.read_text(encoding='utf-8', errors='replace')


@lru_cache(maxsize=None)
def callsites(name):
    result = []
    # Application code and plugins only. Tests and third-party SQL are not call sites.
    for path, source in APPLICATION_SOURCES.items():
        pattern = r'\bDatabaseQueries::' + re.escape(name) + r'(?:<[^;\n]*>)?\s*\('
        matches = list(re.finditer(pattern, source))
        if path in texts:
            matches += list(re.finditer(r'(?<!::)\b' + re.escape(name) + r'\s*\(', source))
        for match in sorted(matches, key=lambda m: m.start()):
            enclosing = next((f for f in functions if f['path'] == path and f['brace'] < match.start() < f['end']), None)
            if path in texts and enclosing is None:
                continue  # Declarations and definitions, not calls.
            start_line = source.rfind('\n', 0, match.start()) + 1
            if source[start_line:match.start()].lstrip().startswith('//'):
                continue
            paren = source.index('(', match.start())
            end = scan_end(source, paren)
            result.append({'file': path.relative_to(ROOT).as_posix(),
                           'line': source.count('\n', 0, match.start()) + 1,
                           'function': 'DatabaseQueries::' + enclosing['name'] if enclosing else None,
                           'source': source[match.start():end + 1]})
    return result


FIELDS = ('Messages.id, Messages.is_read, Messages.is_important, Messages.is_deleted, '
          'Messages.is_pdeleted, Messages.feed, Messages.title, Messages.url, Messages.author, '
          'Messages.date_created, Messages.date_retrieved, Messages.contents, Messages.enclosures, '
          'Messages.score, Messages.account_id, Messages.custom_id, Messages.custom_data, '
          '(SELECT GROUP_CONCAT(Labels.custom_id) FROM LabelsInMessages lim JOIN Labels ON '
          'lim.label = Labels.id WHERE lim.message = Messages.id) AS msg_labels')
BIN = 'Messages.account_id = {account_id} AND Messages.is_deleted = 1 AND Messages.is_pdeleted = 0'
IMPORTANT = ('Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND '
             'Messages.is_pdeleted = 0 AND Messages.is_important = 1')
UNREAD = ('Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND '
          'Messages.is_pdeleted = 0 AND Messages.is_read = 0')
ACCOUNT = 'Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0'
ACCOUNT_ALL = 'Messages.account_id = {account_id} AND Messages.is_pdeleted = 0'
FEEDS = '{feed_selector} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0'
LABEL = ('Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND '
         'EXISTS (SELECT 1 FROM LabelsInMessages WHERE LabelsInMessages.label = {label_id} AND '
         'LabelsInMessages.message = Messages.id)')
LABELS = LABEL.replace('LabelsInMessages.label = {label_id} AND ', '')
PROBE_REGEX = ('Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND '
               "(Messages.title REGEXP '{escaped_regex}' OR Messages.contents REGEXP '{escaped_regex}')")
PROBE_SQL = ('Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND '
             '({user_sql_predicate})')
LOOKUP = ('SELECT requested.message_index, Messages.id, Messages.date_created, Messages.is_read, '
          'Messages.is_important, {contents_column}, Messages.feed, Messages.title, Messages.author '
          'FROM ({requested_rows}) AS requested JOIN Messages ON Messages.id = ('
          '  SELECT candidate.id FROM Messages AS candidate WHERE {join_condition} LIMIT 1) '
          'ORDER BY requested.message_index;')
LOOKUP_ID_ROW = 'SELECT CAST(:message_index_{i} AS SIGNED INTEGER) AS message_index, :lookup_id_{i} AS lookup_id'
LOOKUP_CUSTOM_ROW = 'SELECT CAST(:message_index_{i} AS SIGNED INTEGER) AS message_index, :custom_id_{i} AS custom_id'
LOOKUP_ANON_ROW = ('SELECT CAST(:message_index_{i} AS SIGNED INTEGER) AS message_index, '
                   ':title_{i} AS title, :url_{i} AS url, :author_{i} AS author')
BULK = ('INSERT INTO Messages (feed, title, is_read, is_important, is_deleted, url, author, score, '
        'date_created, date_retrieved, contents, enclosures, custom_id, custom_data, account_id) VALUES {rows};')
BULK_ROW = ("\n({feed_id}, '{escaped_title}', {is_read}, {is_important}, {is_deleted}, "
            "'{escaped_url}', '{escaped_author}', {score}, {date_created_ms}, {date_retrieved_ms}, "
            "'{escaped_contents}', '{escaped_enclosures}', '{escaped_custom_id}', '{escaped_custom_data}', {account_id})")


def variant(origin, predicate, template, detail=''):
    f = by_name[origin]
    return {'origin_function': 'DatabaseQueries::' + origin,
            'origin_file': f['path'].relative_to(ROOT).as_posix(), 'origin_line': f['line'],
            'sql_template': template.replace('{where_clause}', predicate), 'details': detail,
            'callers': callsites(origin)}


variants = {}
template = 'UPDATE Messages SET is_read = :read WHERE {where_clause};'
variants['markMessagesByCondition'] = [
    variant('markProbeReadUnread', PROBE_REGEX, template, 'Search::Type::Regex = 0'),
    variant('markProbeReadUnread', PROBE_SQL, template, 'Search::Type::SqlWhereClause = 1; arbitrary user SQL predicate'),
    variant('markAllLabelledMessagesReadUnread', LABELS, template),
    variant('markLabelledMessagesReadUnread', LABEL, template),
    variant('markImportantMessagesReadUnread', IMPORTANT, template),
    variant('markUnreadMessagesRead', UNREAD, template, ':read = 1'),
    variant('markMessagesReadUnread', 'id IN ({comma_separated_message_ids})', template),
    variant('markFeedsReadUnread', FEEDS, template),
    variant('markBinReadUnread', BIN, template),
    variant('markAccountReadUnread', ACCOUNT_ALL, template)]
template = 'DELETE FROM Messages WHERE {where_clause};'
variants['purgeMessagesByCondition'] = [
    variant('purgeMessage', 'id = {message_id}', template),
    variant('purgeImportantMessages', 'is_important = 1 AND is_deleted = 0', template),
    variant('purgeReadMessages', 'is_important = 0 AND is_deleted = 0 AND is_read = 1', template),
    variant('purgeOldMessages', 'is_important = 0 AND date_created < {since_epoch_ms}', template,
            'since_epoch_ms = UTC now + 10 years if older_than_days == 0, else UTC now - older_than_days'),
    variant('purgeRecycleBin', 'is_important = 0 AND is_deleted = 1', template),
    variant('purgeLeftoverMessages', 'feed NOT IN (SELECT id FROM Feeds WHERE account_id = {account_id}) AND account_id = {account_id}', template)]
template = 'SELECT feed, COUNT(*), SUM(NOT is_read) FROM Messages WHERE {where_clause}{group_by};'
variants['messageCountsByCondition'] = [
    variant('getMessageCountsForFeeds', FEEDS, template.replace('{group_by}', ' GROUP BY feed')),
    variant('getMessageCountsForFeed', FEEDS.replace('{feed_selector}', 'Messages.feed = {feed_id}'), template.replace('{group_by}', '')),
    variant('getImportantMessageCounts', IMPORTANT, template.replace('{group_by}', '')),
    variant('getUnreadMessageCounts', UNREAD, template.replace('{group_by}', '')),
    variant('getMessageCountsForBin', BIN, template.replace('{group_by}', '')),
    variant('getMessageCountsForLabel', LABEL, template.replace('{group_by}', ''))]
template = 'UPDATE Messages SET is_deleted = 1 WHERE ({where_clause}){read_only};'
variants['cleanMessagesByCondition'] = [
    variant('cleanImportantMessages', IMPORTANT, template),
    variant('cleanUnreadMessages', UNREAD, template.replace('{read_only}', '')),
    variant('cleanFeeds', FEEDS, template, 'No SQL emitted for empty feed_ids'),
    variant('cleanLabelledMessages', LABEL, template),
    variant('cleanProbedMessages', PROBE_REGEX, template, 'Regex'),
    variant('cleanProbedMessages', PROBE_SQL, template, 'Arbitrary user SQL predicate')]
template = 'SELECT custom_id FROM Messages WHERE {where_clause};'
variants['customIdsOfMessagesByCondition'] = [
    variant('customIdsOfMessagesFromLabels', LABELS, template,
            'Unknown omits status predicate; else append AND Messages.is_read = :read. :read = 0 for Read, 1 otherwise.'),
    variant('customIdsOfMessagesFromLabel', LABEL, template,
            'Unknown omits status predicate; else append AND Messages.is_read = :read. :read = 0 for Read, 1 otherwise.'),
    variant('customIdsOfMessagesFromFeeds', FEEDS + ' AND Messages.is_read = :read', template),
    variant('customIdsOfMessagesFromAccount', ACCOUNT + ' AND Messages.is_read = :read', template),
    variant('customIdsOfImportantMessages', IMPORTANT + ' AND Messages.is_read = :read', template),
    variant('customIdsOfUnreadMessages', UNREAD, template),
    variant('customIdsOfMessagesFromBin', BIN + ' AND Messages.is_read = :read', template),
    variant('customIdsOfMessagesFromProbe', PROBE_REGEX + ' AND Messages.is_read = :read', template),
    variant('customIdsOfMessagesFromProbe', PROBE_SQL + ' AND Messages.is_read = :read', template)]

DYNAMIC = {
    'deassignLabelFromMessages': 'message_ids is a numeric comma-separated list, 1..500 per batch; no statement for an empty messages list. :label = label->id().',
    'assignLabelToMessages': 'Finite dialect verb: SQLite INSERT OR IGNORE / MariaDB INSERT IGNORE. Values are 1..500 literal numeric (message_id, label_id) pairs per batch. No statement for empty messages.',
    'setLabelsForMessage': 'Always DELETE bindings :message = msg.m_id; if labels nonempty, generate one INSERT with one numeric (msg.m_id, label->id()) tuple per label, no batch cap.',
    'createLabel': 'INSERT branch with explicit :id iff new_label_id > 0; otherwise auto id. Both followed by whole-Labels custom_id fixup. Fields bound from label and account_id.',
    'customIdsOfMessagesByCondition': 'where_clause composed by 8 public callers; dynamic_variants enumerates each family. Bindings iterated from supplied QVariantMap. Every status binding is 0 for target Read, 1 otherwise (opposite current state). Optional Unknown status only for Labels/Label variants.',
    'markMessagesByCondition': 'where_clause is generated by 9 public callers. dynamic_variants enumerates all families. :read = 1 for Read, 0 otherwise. Raw numeric ID list has no batch cap. Probe predicates can be arbitrary SQL.',
    'switchMessagesImportance': 'ids.join(", ") inserted directly as numeric IN list, no batch cap.',
    'permanentlyDeleteMessages': 'ids.join(", ") inserted directly as numeric IN list, no batch cap.',
    'deleteOrRestoreMessagesToFromBin': 'is_deleted = deleted ? 1 : 0; is_pdeleted always 0. ids.join(", ") is unbounded numeric IN list.',
    'restoreBin': 'whereClauseBin(account_id) expanded using numeric account id.',
    'removeUnwantedArticlesFromFeed': 'recycle_dont_purge selects recycle SELECT/UPDATE vs purge SELECT/DELETE. Controls inherit feed or app limit settings. :feed = feed->id(), :offset = amount_to_keep (>0), :is_important = dont_remove_starred ? 1 : 2, :is_read = dont_remove_unread ? 0 : 2. Returned ID list expanded into a single unbounded UPDATE/DELETE; no mutation emitted if no ids returned.',
    'purgeFeedArticles': 'feeds_ids joined into one unbounded numeric IN list; empty feeds produces no SQL.',
    'purgeMessagesByCondition': 'All 6 public caller predicates appear in dynamic_variants; epoch and IDs interpolated as numeric literals.',
    'messageCountsByCondition': 'where_clause composed by 6 public callers; GROUP BY feed only in getMessageCountsForFeeds. Other variants select feed alongside aggregate without GROUP BY; MariaDB execution depends on sql_mode (ONLY_FULL_GROUP_BY may reject). Optional binding map is supported but current callers supply no bindings.',
    'getUndeletedMessagesForFeed': 'Full 18-attribute projection from messageTableAttributes, including correlated label GROUP_CONCAT subquery. Single-feed predicate expanded. No LIMIT.',
    'getUndeletedMessagesForAccount': 'Full 18-attribute projection from messageTableAttributes, including correlated label GROUP_CONCAT subquery. Account undeleted predicate expanded. No LIMIT.',
    'bagOfMessages': 'bag enum gives is_read = 0 (Unread), is_important = 1 (Starred), is_read = 1 (Read/default). :feed = feed->id(). No deleted/pdeleted predicates and no LIMIT.',
    'updateMessages': 'See generator_details. force_insert skips lookups; updates reexecute one prepared statement per changed row. Insert batches contain at most 1000 valid-title rows. Entire Messages custom_id fixup runs for every nonempty updateMessages invocation.',
    'cleanBin': 'clear_only_read gives extra AND is_read = 1; whereClauseBin(account_id) expanded.',
    'deleteAccount': 'The loop currently has exactly one query: DELETE FROM Accounts WHERE id = :account_id. Foreign-key cascades emit engine work.',
    'deleteAccountData': 'Messages DELETE only if delete_messages_too; Feeds and Categories always deleted by account; Labels DELETE only if delete_labels_too. Includes FK cascades.',
    'cleanMessagesByCondition': 'where_clause composed by 5 public callers; append AND is_read = 1 iff clean_read_only. cleanUnreadMessages always supplies false; cleanFeeds returns early on empty IDs. dynamic_variants enumerates all branches.',
    'createOverwriteCategory': 'MAX sort-order SELECT only for new category/moving category. INSERT only for new category; account_id inserted as numeric literal. UPDATE always. Existing moving category calls moveItem first.',
    'createOverwriteFeed': 'MAX sort-order SELECT only for new/moving feed. INSERT for new feed has explicit numeric id iff new_feed_id > 0; otherwise auto id. account_id numeric literal. UPDATE always. Moving feed also calls moveItem.',
    'storeCustomData': 'Runtime table identifier arg(table); observed callers restrict table to Categories and Feeds, but public method accepts arbitrary QString. :id = item_id; :custom_data = serialized QVariantHash.',
    'createOverwriteAccount': 'SELECT MAX(ordr) only for new account with sortOrder < 0. INSERT only for accountId <= 0, :type = account->code(). UPDATE always.',
    'moveItem': 'Finite table/parent pairs: Accounts (no parent/account scope), Feeds/category, Categories/parent_id. Up/down reordering selects +1 or -1 neighbor UPDATE. Final UPDATE sets moved item order by id. :account_id = account id, :category = parent id, :move_low/:move_high = min/max of old and new order; final :id accountId for root or item id; :ordr = move_index.',
    'moveMessageFilter': 'Neighbor UPDATE uses +1 if old order > target, -1 otherwise. :move_low/:move_high = min/max old/target. Final UPDATE by :id sets :ordr = move_index.',
    'getAccounts': "Plugin code is interpolated as '%1' via arg(code), not bindValue. Observed callers invoke service-entrypoint code() for all compiled services; result selects all account attributes."
}

PREPARED_EXECUTION_LINES = {
    87: [89], 196: [200], 231: [234], 257: [260], 277: [284],
    290: [293], 304: [317], 309: [317], 328: [329], 335: [344],
    350: [358], 368: [370], 390: [394], 403: [409], 427: [429],
    477: [481], 487: [489], 495: [497], 503: [506], 512: [514],
    542: [569], 554: [569], 585: [597], 592: [597], 624: [625],
    670: [679], 728: [737], 760: [764], 780: [784], 832: [839],
    1008: [943], 1039: [943], 1070: [943], 1170: [1199],
    1392: [1398], 1395: [1398], 1412: [1415], 1427: [1430],
    1433: [1436], 1438: [1441], 1444: [1447], 1464: [1465],
    1495: [1497], 1503: [1507], 1645: [1649], 1664: [1669],
    1685: [1712], 1726: [1730], 1746: [1759], 1753: [1759],
    1779: [1843], 1853: [1859], 1877: [1881], 1888: [1906],
    1914: [1917], 1925: [1928], 1992: [2018], 1996: [2018],
    2002: [2018], 2007: [2018], 2020: [2024], 2084: [2095],
    2088: [2095], 2097: [2101], 2139: [2144], 2158: [2160],
    2167: [2168], 2187: [2193], 2204: [2210], 2216: [2225],
    2231: [2237], 2244: [2248], 2260: [2263], 2274: [2278]
}

entries = []
for path in FILES:
    source = texts[path]
    for match in re.finditer(r'\b(\w+)\s*\.\s*(prepare|exec)\s*\(', source):
        variable, method = match.group(1), match.group(2)
        begin = source.index('(', match.start())
        end = scan_end(source, begin)
        expression = source[begin + 1:end].strip()
        if method == 'exec' and expression in ('', 'false', 'true'):
            continue  # Executes previously prepared SQL, recorded against prepare below.
        f = next((f for f in functions if f['path'] == path and f['brace'] < match.start() < f['end']), None)
        if f is None:
            raise ValueError('Unmapped SQL call ' + expression)
        method_offset = match.start() + match.group().index(method)
        line = source.count('\n', 0, method_offset) + 1
        template = leading_literal(expression)
        name = f['name']
        generator_details = None
        if template is None:
            if name == 'setLabelsForMessage':
                template = 'INSERT INTO LabelsInMessages (message, label) VALUES {comma_separated_numeric_pairs};'
            elif name == 'customIdsOfMessagesByCondition':
                template = 'SELECT custom_id FROM Messages WHERE {where_clause};'
            elif name == 'markMessagesByCondition':
                template = 'UPDATE Messages SET is_read = :read WHERE {where_clause};'
            elif name == 'purgeMessagesByCondition':
                template = 'DELETE FROM Messages WHERE {where_clause};'
            elif name == 'cleanMessagesByCondition':
                template = 'UPDATE Messages SET is_deleted = 1 WHERE ({where_clause}){read_only};'
            elif name == 'deleteAccount':
                template = 'DELETE FROM Accounts WHERE id = :account_id;'
            elif name == 'updateMessages':
                if expression.startswith('lookup_query('):
                    template = LOOKUP
                    if line == 1008:
                        join = 'candidate.id = requested.lookup_id'
                        row = LOOKUP_ID_ROW
                        scope = None
                        bind_patterns = {':message_index_{i}': 'direct_id_indices[batch_start+i]',
                                         ':lookup_id_{i}': 'messages[message_index].m_id'}
                    elif line == 1039:
                        join = 'candidate.custom_id = requested.custom_id AND {scope_condition}'
                        row = LOOKUP_CUSTOM_ROW
                        scope = ['candidate.account_id = :scope_id', 'candidate.feed = :scope_id']
                        bind_patterns = {':scope_id': 'is_syncable ? account_id : feed_id',
                                         ':message_index_{i}': 'custom_id_indices[batch_start+i]',
                                         ':custom_id_{i}': 'unnulifyString(messages[message_index].m_customId)'}
                    elif line == 1070:
                        join = ('candidate.feed = :feed AND candidate.title = requested.title AND '
                                'candidate.url = requested.url AND candidate.author = requested.author')
                        row = LOOKUP_ANON_ROW
                        scope = None
                        bind_patterns = {':feed': 'feed_id', ':message_index_{i}': 'anonymous_indices[batch_start+i]',
                                         ':title_{i}': 'unnulifyString(message.m_title)',
                                         ':url_{i}': 'unnulifyString(message.m_url)',
                                         ':author_{i}': 'unnulifyString(message.m_author)'}
                    else:
                        raise ValueError('Unexpected lookup line ' + str(line))
                    template = template.replace('{join_condition}', join)
                    generator_details = {'builder_line': 931, 'execution_line': 943,
                                         'requested_row_template': row, 'join_separator': ' UNION ALL ',
                                         'rows_per_batch': {'min': 1, 'max': 200},
                                         'placeholder_i_range': '0..batch_rows-1',
                                         'contents_column_variants': ['NULL', 'Messages.contents'],
                                         'contents_control': 'IgnoreContentsChanges setting',
                                         'scope_condition_variants': scope, 'binding_patterns': bind_patterns,
                                         'skip_condition': 'force_insert or no messages in that lookup class'}
                elif expression == 'final_bulk':
                    template = BULK
                    generator_details = {'builder_line': 1222, 'row_builder_line': 1247, 'row_template': BULK_ROW,
                                         'join_separator': ', ', 'rows_per_batch': {'min': 1, 'max': 1000},
                                         'escaping': "DatabaseFactory::escapeQuery doubles single quotes; quoted string values are interpolated, not bound. Numeric fields use QString::number.",
                                         'skip_condition': 'no msgs_to_insert or all titles empty',
                                         'column_defaults': 'id auto-generated; is_pdeleted omitted and defaults to 0'}
                else:
                    raise ValueError('Unresolved update expression ' + expression)
            else:
                raise ValueError('Unresolved SQL expression in ' + name + ': ' + expression)
        if name == 'getUndeletedMessagesForFeed':
            template = 'SELECT ' + FIELDS + ' FROM Messages WHERE ' + FEEDS.replace('{feed_selector}', 'Messages.feed = {feed_id}') + ';'
        elif name == 'getUndeletedMessagesForAccount':
            template = 'SELECT ' + FIELDS + ' FROM Messages WHERE ' + ACCOUNT + ';'
        elif name == 'messageCountsByCondition':
            template = template.replace('%1', '{where_clause}').replace('%2', '{group_by}')
        elif name == 'restoreBin':
            template = template.replace('%1', BIN)
        elif name == 'cleanBin':
            template = template.replace('%1', BIN)

        bindings = []
        # Preserve all potential bindValue sources in this enclosing function;
        # filter by SQL name where possible. Generator patterns are separately explicit.
        for bm in re.finditer(r'\b' + re.escape(variable) + r'\s*\.\s*bindValue\s*\(', f['body']):
            bp = f['body'].index('(', bm.start())
            be = scan_end(f['body'], bp)
            raw = f['body'][bp + 1:be]
            tokens = STRING.findall(raw)
            bind_name = json.loads(tokens[0]) if tokens else None
            if bind_name is None or bind_name in template or ('%1' in bind_name and name == 'updateMessages'):
                bind_offset = f['brace'] + 1 + bm.start()
                bindings.append({'line': source.count('\n', 0, bind_offset) + 1, 'source': raw.strip()})

        execution_lines = []
        if method == 'prepare':
            execution_lines = ({281: [284], 325: [328]} if path.suffix == '.h'
                               else PREPARED_EXECUTION_LINES)[line]
        else:
            execution_lines = [line]

        entry = {'id': 'CORE-' + str(len(entries) + 1).zfill(3),
                 'file': path.relative_to(ROOT).as_posix(), 'line': line,
                 'function': 'DatabaseQueries::' + name, 'source_method': method,
                 'query_variable': variable, 'sql_template': template,
                 'source_expression': expression, 'execution_lines': execution_lines,
                 'dynamic_details': DYNAMIC.get(name, 'Static SQL; parameter values supplied by bindValue.'),
                 'dialects': ['sqlite', 'mariadb'], 'bindings': bindings,
                 'callers': callsites(name)}
        if name in variants:
            entry['dynamic_variants'] = variants[name]
        if generator_details:
            entry['generator_details'] = generator_details
        if name == 'assignLabelToMessages':
            entry['dialect_variants'] = {'sqlite': template.replace('%1', 'INSERT OR IGNORE'),
                                         'mariadb': template.replace('%1', 'INSERT IGNORE')}
        if name == 'bagOfMessages':
            entry['dynamic_variants'] = [{'enum_value': bag, 'sql_template': template.replace('%1', pred)}
                                         for bag, pred in [('Unread', 'is_read = 0'), ('Starred', 'is_important = 1'), ('Read/default', 'is_read = 1')]]
        if name == 'storeCustomData':
            entry['dynamic_variants'] = [{'table': table, 'sql_template': template.replace('%1', table)}
                                         for table in ('Categories', 'Feeds')]
        if name == 'moveItem' and '%1' in template:
            valid_tables = [('Feeds', 'category'), ('Categories', 'parent_id')]
            if '%2' not in template:
                valid_tables += [('Accounts', None)]
            entry['dynamic_variants'] = [{'table': table, 'sql_template': template.replace('%1', table).replace('%2', parent or '')}
                                         for table, parent in valid_tables]
        entries.append(entry)

# Fragment builders are listed separately because they are not standalone queries;
# their caller graph exposes SELECTs generated elsewhere in the application.
fragments = []
for name, sql, details in [
    ('messageTableAttributes', FIELDS, 'initMessageTableAttributes() at cpp:21 constructs 18 projections; correlated Labels GROUP_CONCAT expression at cpp:41.'),
    ('whereClauseBin', BIN, 'Numeric account_id interpolation.'),
    ('whereClauseImportantArticles', IMPORTANT, 'Numeric account_id interpolation.'),
    ('whereClauseUnreadArticles', UNREAD, 'Numeric account_id interpolation.'),
    ('whereClauseProbe', PROBE_REGEX + '\nOR branch:\n' + PROBE_SQL,
     'Regex filter is quote escaped by DatabaseFactory::escapeQuery; SQL filter is arbitrary raw SQL in parentheses. Search::Type::Regex=0, SqlWhereClause=1.'),
    ('whereClauseLabel', LABEL, 'label_id > 0 adds label equality; label_id <= 0 omits it, yielding any-label EXISTS predicate.'),
    ('whereClauseLabels', LABELS, 'Delegates whereClauseLabel(0, account_id).'),
    ('whereClauseAccount', ACCOUNT, 'including_deleted true omits is_deleted predicate, giving ' + ACCOUNT_ALL),
    ('whereClauseFeeds', FEEDS, 'feed_selector = Messages.feed IN (NULL) for 0 ids, Messages.feed = numeric id for 1, Messages.feed IN (comma-separated ids) for 2+. IN list has no cap.')]:
    f = by_name[name]
    fragments.append({'id': 'CORE-FRAGMENT-' + str(len(fragments) + 1).zfill(2),
                      'file': f['path'].relative_to(ROOT).as_posix(), 'line': f['line'],
                      'function': 'DatabaseQueries::' + name, 'sql_template': sql,
                      'dynamic_details': details, 'dialects': ['sqlite', 'mariadb'],
                      'callers': callsites(name), 'kind': 'sql_fragment'})

EXAMPLE_VALUES = {
    'account_id': '1', 'feed_id': '1', 'label_id': '1', 'message_id': '1',
    'comma_separated_message_ids': '1, 2, 3', 'comma_separated_numeric_pairs': '(1, 1), (1, 2), (1, 3)',
    'feed_selector': 'Messages.feed IN (1, 2, 3)', 'where_clause': ACCOUNT.replace('{account_id}', '1'),
    'since_epoch_ms': '1750000000000', 'escaped_regex': 'audit', 'user_sql_predicate': "Messages.title LIKE '%audit%'",
    'read_only': '', 'group_by': '', 'contents_column': 'Messages.contents',
    'scope_condition': 'candidate.feed = :scope_id'
}
EXAMPLE_BINDINGS = {
    'id': 1, 'message': 1, 'account_id': 1, 'feed': 1, 'label': 1, 'filter': 1,
    'category': 0, 'parent_id': 0, 'read': 1, 'important': 1, 'is_important': 1,
    'is_read': 1, 'is_deleted': 0, 'is_off': 0, 'is_quiet': 0, 'is_rtl': 0,
    'is_enabled': 1, 'offset': 100, 'scope_id': 1, 'ordr': 1, 'move_low': 0, 'move_high': 10,
    'name': 'Audit item', 'title': 'Audit article', 'description': 'Audit description',
    'script': '', 'fltr': "Messages.title LIKE '%audit%'", 'type': 'std', 'icon': '',
    'custom_data': '{}', 'custom_id': 'audit-item', 'url': 'https://example.invalid/audit',
    'author': 'Audit author', 'contents': '<p>Audit article contents</p>', 'enclosures': '',
    'date_created': 1700000000000, 'date_retrieved': 1700000000000, 'score': 0,
    'source': 'https://example.invalid/feed', 'update_type': 0, 'update_interval': 1,
    'proxy_type': 0, 'proxy_host': '', 'proxy_port': 0, 'proxy_username': '', 'proxy_password': '',
    'add_any_datetime_articles': 1, 'datetime_to_avoid': 0, 'keep_article_customize': 0,
    'keep_article_count': 1000, 'keep_unread_articles': 1, 'keep_starred_articles': 1,
    'recycle_articles': 1, 'open_articles': 0, 'exclude_global_unread': 0
}


def example(sql, entry, label='representative', rows=1, extra=None):
    name = entry['function'].split('::')[1]
    for placeholder, value in EXAMPLE_VALUES.items():
        sql = sql.replace('{' + placeholder + '}', value)
    if '%1' in sql or '%2' in sql or '%3' in sql:
        if name == 'assignLabelToMessages':
            sql = sql.replace('%1', 'INSERT OR IGNORE').replace('%2', ', '.join(f'({i + 1}, 1)' for i in range(rows)))
        elif name == 'bagOfMessages':
            sql = sql.replace('%1', 'is_read = 0')
        elif name == 'getAccounts':
            sql = sql.replace('%1', 'std')
        elif name == 'storeCustomData' or name == 'moveItem':
            sql = sql.replace('%1', 'Feeds').replace('%2', 'category')
        elif name in ('createOverwriteCategory', 'createOverwriteFeed'):
            sql = sql.replace('%1', '1').replace('%2', '9000001')
        elif name == 'deleteOrRestoreMessagesToFromBin':
            sql = sql.replace('%1', '1, 2, 3').replace('%2', '1').replace('%3', '0')
        else:
            sql = sql.replace('%1', ', '.join(str(i + 1) for i in range(rows)))
    if entry.get('generator_details', {}).get('requested_row_template'):
        generator = entry['generator_details']
        sql = sql.replace('{requested_rows}', generator['join_separator'].join(
            generator['requested_row_template'].replace('{i}', str(i)) for i in range(rows)))
    elif '{rows}' in sql:
        row = "(1, 'Audit article', 0, 0, 0, 'https://example.invalid/audit', 'Audit author', 0, 1700000000000, 1700000000000, '<p>Audit content</p>', '', 'audit-article', '{}', 1)"
        sql = sql.replace('{rows}', ', '.join(row for _ in range(rows)))
    bindings = {}
    for binding in set(re.findall(r':([A-Za-z_][A-Za-z_0-9]*)', sql)):
        index = re.search(r'_(\d+)$', binding)
        if index:
            idx = int(index.group(1))
            if binding.startswith('message_index_'):
                value = idx
            elif binding.startswith('lookup_id_'):
                value = idx + 1
            elif binding.startswith('custom_id_'):
                value = 'audit-article-' + str(idx)
            elif binding.startswith('title_'):
                value = 'Audit lookup title ' + str(idx)
            elif binding.startswith('url_'):
                value = 'https://example.invalid/audit/' + str(idx)
            elif binding.startswith('author_'):
                value = 'Audit author'
            else:
                raise ValueError('Unhandled indexed binding ' + binding)
        else:
            if binding not in EXAMPLE_BINDINGS:
                raise ValueError('Unhandled binding ' + binding)
            value = EXAMPLE_BINDINGS[binding]
            if binding == 'type' and 'Probes' in sql:
                value = 1
        bindings[binding] = value
    if extra:
        bindings.update(extra)
    if re.search(r'\{\w+\}|%[123]', sql):
        raise ValueError('Unresolved example SQL for ' + entry['id'] + ': ' + sql)
    return {'label': label, 'concrete_sql': sql, 'bindings': bindings,
            'dialect': 'sqlite', 'example_only': True,
            'notes': 'Adapt identifiers/text to the synthetic dataset; execute mutations only inside rolled-back savepoints or disposable clones.'}


for entry in entries:
    examples = []
    if entry.get('dynamic_variants'):
        for i, item in enumerate(entry['dynamic_variants']):
            label = item.get('origin_function', item.get('enum_value', item.get('table', f'variant-{i + 1}')))
            examples.append(example(item['sql_template'], entry, label))
            if '{read_only}' in item['sql_template']:
                examples.append(example(item['sql_template'].replace('{read_only}', ' AND is_read = 1'), entry, label + '/read-only'))
            if entry['function'].endswith('customIdsOfMessagesByCondition') and 'Unknown omits' in item.get('details', ''):
                examples.append(example(item['sql_template'].replace(';', ' AND Messages.is_read = :read;'), entry, label + '/opposite-state'))
            if '{feed_selector}' in item['sql_template']:
                for selector_label, selector in [('zero-feeds', 'Messages.feed IN (NULL)'), ('one-feed', 'Messages.feed = 1')]:
                    # cleanFeeds returns early for zero feed IDs; other users emit IN(NULL).
                    if selector_label == 'zero-feeds' and item.get('origin_function', '').endswith('cleanFeeds'):
                        continue
                    examples.append(example(item['sql_template'].replace('{feed_selector}', selector), entry, label + '/' + selector_label))
    elif entry.get('generator_details', {}).get('requested_row_template'):
        generator = entry['generator_details']
        for count in (1, 200):
            scopes = generator['scope_condition_variants'] or [None]
            for scope in scopes:
                for contents in generator['contents_column_variants']:
                    sql = entry['sql_template'].replace('{contents_column}', contents)
                    if scope:
                        sql = sql.replace('{scope_condition}', scope)
                    label = f'{count}-row/{scope or "primary-or-anonymous"}/{contents}'
                    examples.append(example(sql, entry, label, rows=count))
    else:
        examples.append(example(entry['sql_template'], entry))
    if entry['function'].endswith('assignLabelToMessages') or entry['function'].endswith('deassignLabelFromMessages'):
        examples.append(example(entry['sql_template'], entry, 'maximum-500-row-batch', rows=500))
    if entry['function'].endswith('deleteOrRestoreMessagesToFromBin'):
        examples.append(example(entry['sql_template'].replace('%2', '0'), entry, 'restore-from-bin'))
    if entry['function'].endswith('removeUnwantedArticlesFromFeed') and entry['sql_template'].startswith('SELECT'):
        for protect_starred in (False, True):
            for protect_unread in (False, True):
                examples.append(example(entry['sql_template'], entry,
                                        f'protect-starred={protect_starred}/protect-unread={protect_unread}',
                                        extra={'is_important': 1 if protect_starred else 2,
                                               'is_read': 0 if protect_unread else 2}))
    if entry.get('generator_details', {}).get('row_template'):
        examples.append(example(entry['sql_template'], entry, 'maximum-1000-row-batch', rows=1000))
    entry['benchmark_examples'] = examples
    entry['concrete_sql'] = examples[0]['concrete_sql']
    entry['concrete_bindings'] = examples[0]['bindings']

OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'core-inventory.json').write_text(json.dumps(entries, indent=2) + '\n', encoding='utf-8')
(OUT / 'core-fragments.json').write_text(json.dumps(fragments, indent=2) + '\n', encoding='utf-8')
notes = f'''DatabaseQueries core SQL inventory
==================================
Scope: databasequeries.cpp (all 2283 lines), databasequeries.h (all 425 lines),
plus source call-site search under src excluding third-party trees.
{len(entries)} prepare/direct-exec SQL sites/templates, {len(fragments)} SQL fragment builders.
Prepared exec() calls do not create additional SQL templates; execution_lines
preserves their exact execution sites, including execute_lookup's lambda at 943.
Every query source expression is preserved for verification.

Dynamic SQL completeness
------------------------
Finite structural families: 0/1/many feed selectors; any/specific label EXISTS;
included/excluded bin articles; Regex/raw SQL probe; read-only cleanup; counts
grouped/ungrouped; Read/Unread/Starred bags; explicit/automatic IDs; category/feed/
account reorder tables and up/down directions; custom-id account/feed scope;
lookup projection NULL/contents; SQLite/MariaDB insert-ignore verbs.

Bounded batch structures: label assignment/deassignment 1..500; existing article
lookup 1..200 UNION ALL SELECT rows with repeated named bindings; article bulk
INSERT 1..1000 valid-title rows per execution. All batches repeat until input ends.

Unbounded families: raw SQL probe filters (including arbitrary SQL subqueries,
operators, functions and expressions); Regex patterns; IN lists used by article
mark/delete/restore, purge/retention, multi-feed selectors; label list insertion;
public storeCustomData table string; textual data and numeric literal values.
It is impossible to list every concrete runtime SQL text or benchmark every raw
user SQL predicate. This inventory lists every application generator/call site,
all identified structural branches, and the exact generator grammar instead.

messageTableAttributes contributes a correlated label GROUP_CONCAT SELECT to
message reads in this file, filterobjects.cpp and MessagesModelSqlLayer. Root
inventory must expand this projection in those consumers. Universal whereClause
builders feed ServiceRoot::updateMessagesModel filters at serviceroot.cpp:993..1022,
which are executed later by MessagesModelSqlLayer; core-fragments.json includes
their call sites so those runtime SQL families are connected explicitly.

Observed bindings and branch caveats
----------------------------------
ReadStatus Unread=0, Read=1, Unknown=256. markMessagesByCondition writes Read=1;
customIdsOfMessagesFrom* select opposite state (:read=0 for requested Read,
otherwise 1). Labels/Label omit status predicate for Unknown; others do not.
Retention binds avoid-starred/read sentinels 1/0 or 2 rather than changing SQL.
force_insert skips all article lookups. IgnoreContentsChanges switches lookup
projection from Messages.contents to NULL. Lookups choose first match LIMIT 1
without ORDER BY candidate. Scalar and bulk writes include FK/index maintenance.
Updates may execute per row and later assign/deassign labels through separate
queries. updateMessages finishes with a whole-table missing-custom_id fixup for
every nonempty updateMessages invocation, even if no row was added/updated.
Most SQL works on both dialects; REGEXP requires app SQLite regexp extension.
Ungrouped messageCountsByCondition selecting feed alongside aggregates may fail
with MariaDB ONLY_FULL_GROUP_BY. Dialect claims describe app paths, not proof
that arbitrary user predicate syntax is supported on both databases.

benchmark_examples provides concrete SQLite SQL and complete named bindings for
each site/family, both lookup contents projections and account/feed scopes, max
lookup/label/insert batches, and optional read-only/opposite-state branches. These
are ready-to-parse representative inputs, not evidence of runtime execution.
Adapt IDs and text to the synthetic DB for hit/miss/skew and selectivity coverage.

No benchmarks were run by this inventory worker. Application files unmodified.
'''
(OUT / 'core-notes.txt').write_text(notes, encoding='utf-8')
print(json.dumps({'sql_sites': len(entries), 'fragment_builders': len(fragments),
                  'nested_dynamic_variants': sum(len(e.get('dynamic_variants', [])) for e in entries)}, indent=2))
