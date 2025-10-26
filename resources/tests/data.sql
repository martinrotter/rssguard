PRAGMA journal_mode = MEMORY;
PRAGMA synchronous = OFF;
PRAGMA temp_store = MEMORY;
PRAGMA cache_size = -80000;

-- 1️⃣ Account
INSERT INTO Accounts (id, ordr, type, custom_data)
VALUES (1, 0, 'std-rss', '{"title":"Demo RSS Account"}');

-- 2️⃣ Categories (50) with nesting (~20%)
WITH RECURSIVE cats(id) AS (
  SELECT 1
  UNION ALL
  SELECT id + 1 FROM cats WHERE id < 50
)
INSERT INTO Categories (id, ordr, parent_id, title, account_id)
SELECT
  id,
  id - 1,
  CASE WHEN id % 5 = 0 THEN (abs(random()) % (id-1)) + 1 ELSE -1 END,
  'Category ' || id || ' ' || substr('Alpha Bravo Charlie Delta Echo Foxtrot Golf Hotel India Juliet Kilo Lima Mike November Oscar Papa Quebec Romeo Sierra Tango Uniform Victor Whiskey Xray Yankee Zulu', (abs(random()) % 200) + 1, 12),
  1
FROM cats;

-- 3️⃣ Feeds (1000)
WITH RECURSIVE feeds(id) AS (
  SELECT 1
  UNION ALL
  SELECT id + 1 FROM feeds WHERE id < 1000
)
INSERT INTO Feeds (
  id, ordr, title, description, date_created, category, source,
  update_type, update_interval, is_off, is_quiet, account_id, custom_id, custom_data
)
SELECT
  id,
  id - 1,
  'Feed ' || id || ' ' || substr('Quick Brown Fox Jumps Over Lazy Dog Alpha Beta Gamma Delta Epsilon Zeta Eta Theta Iota Kappa Lambda Mu Nu Xi Omicron Pi Rho Sigma Tau Upsilon Phi Chi Psi Omega', (abs(random()) % 300) + 1, 12),
  'Test feed ' || id || ' description',
  (strftime('%s','now') - (abs(random()) % 31536000)),
  (abs(random()) % 50) + 1,
  'https://example.com/feed/' || id,
  0,
  900,
  0,
  0,
  1,
  'feed-' || id,
  NULL
FROM feeds;

-- 4️⃣ Labels (600)
WITH RECURSIVE lbls(id) AS (
  SELECT 1
  UNION ALL
  SELECT id + 1 FROM lbls WHERE id < 600
)
INSERT INTO Labels (id, name, color, custom_id, account_id)
SELECT
  id,
  'Label ' || id || ' ' || substr('Alpha Bravo Charlie Delta Echo Foxtrot Golf Hotel India Juliet Kilo Lima Mike November Oscar Papa Quebec Romeo Sierra Tango Uniform Victor Whiskey Xray Yankee Zulu', (abs(random()) % 200) + 1, 10),
  '#' || printf('%02X', abs(random()) % 256) || printf('%02X', abs(random()) % 256) || printf('%02X', abs(random()) % 256),
  'label-' || id,
  1
FROM lbls;

-- Function: sample HTML content generator
-- (You can inline or refer to an external table; here we inline logic)

-- 5️⃣ Messages (300,000) with long HTML contents (including <img> tags)
WITH RECURSIVE msgs(id) AS (
  SELECT 1
  UNION ALL
  SELECT id + 1 FROM msgs WHERE id < 300000
)
INSERT INTO Messages (
  id,
  is_read,
  is_important,
  is_deleted,
  is_pdeleted,
  feed,
  title,
  url,
  author,
  date_created,
  contents,
  enclosures,
  score,
  account_id,
  custom_id,
  custom_hash
)
SELECT
  id,
  (abs(random()) % 2),
  CASE WHEN abs(random()) % 100 < 5 THEN 1 ELSE 0 END,
  CASE WHEN abs(random()) % 10 = 0 THEN 1 ELSE 0 END,
  0,
  (abs(random()) % 1000) + 1,
  'Article ' || id || ' ' || substr('Lorem Ipsum Dolor Sit Amet Consectetur Adipiscing Elit Sed Do Eiusmod Tempor', (abs(random()) % 200) + 1, 15),
  'https://example.com/article/' || id,
  'Author ' || ((abs(random()) % 500) + 1),
  (strftime('%s','now') - (abs(random()) % 31536000)),
  
  -- contents: multiple HTML paragraphs + image tags + random word "RANDOM"
  (
    '<html><body>' ||
    '<h1>Heading ' || id || '</h1>' ||
    '<p>' || substr('Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer nec odio. Praesent libero. Sed cursus ante dapibus diam.', (abs(random()) % 200) + 1, 120) || '</p>' ||
    '<p>' || substr('Suspendisse potenti. In faucibus massa arcu, vitae cursus mi hendrerit nec. Nulla facilisi.', (abs(random()) % 200) + 1, 120) || '</p>' ||
    '<p><img src="https://via.placeholder.com/150?text=Img' || id || '" alt="img' || id || '"/></p>' ||
    '<p>' || 'Some random text with RANDOM marker and more content. ' || substr('Curabitur suscipit suscipit tellus. Phasellus viverra nulla ut metus varius laoreet.', (abs(random()) % 200) + 1, 120) || '</p>' ||
    '<footer>Footer RANDOM</footer>' ||
    '</body></html>'
  ),
  
  NULL,
  (abs(random()) % 101),
  1,
  'msg-' || id,
  hex(abs(random()) % 9223372036854775807)
FROM msgs;

-- 6️⃣ LabelsInMessages (~30%)
WITH RECURSIVE attempts(n) AS (
  SELECT 1
  UNION ALL
  SELECT n + 1 FROM attempts WHERE n < 600000
)
INSERT OR IGNORE INTO LabelsInMessages (message, label, account_id)
SELECT
  (abs(random()) % 300000) + 1,
  (abs(random()) % 600) + 1,
  1
FROM attempts
WHERE (abs(random()) % 100) < 30;

-- 7️⃣ Probes (now 100) — we ensure some common words like "RANDOM", "Heading", etc.
WITH RECURSIVE probe_ids(id) AS (
  SELECT 1
  UNION ALL
  SELECT id + 1 FROM probe_ids WHERE id < 100
)
INSERT INTO Probes (id, name, color, fltr, account_id)
SELECT
  id,
  'Probe ' || id,
  '#' || printf('%02X', abs(random()) % 256) || printf('%02X', abs(random()) % 256) || printf('%02X', abs(random()) % 256),
  CASE
    WHEN id % 10 = 0 THEN 'RANDOM'            -- some probes look for the word "RANDOM" in contents
    WHEN id % 10 = 1 THEN 'Heading'           -- some look for "Heading"
    WHEN id % 10 = 2 THEN 'img[0-9]+'         -- regex matching img + digits
    WHEN id % 10 = 3 THEN 'Footer'            -- footer text
    WHEN id % 10 = 4 THEN 'Author [0-9]+'     -- match author in title/metadata
    WHEN id % 10 = 5 THEN 'Lorem ipsum'        -- common lorem phrase
    ELSE substr('Consectetur|Adipiscing|Curabitur|Phasellus|Nulla', (abs(random()) % 40) + 1, 10)
  END,
  1
FROM probe_ids;

-- ✅ Restore WAL mode
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;

-- 🧾 Verification
SELECT
  (SELECT COUNT(*) FROM Categories WHERE account_id = 1) AS categories_count,
  (SELECT COUNT(*) FROM Feeds WHERE account_id = 1) AS feeds_count,
  (SELECT COUNT(*) FROM Messages WHERE account_id = 1) AS messages_count,
  (SELECT COUNT(*) FROM Messages WHERE is_deleted = 1 AND account_id = 1) AS deleted_messages,
  (SELECT COUNT(*) FROM Labels WHERE account_id = 1) AS labels_count,
  (SELECT COUNT(*) FROM LabelsInMessages WHERE account_id = 1) AS label_links_count,
  (SELECT COUNT(*) FROM Probes WHERE account_id = 1) AS probes_count;
