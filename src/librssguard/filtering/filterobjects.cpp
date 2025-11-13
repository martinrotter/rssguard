// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/filterobjects.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "exceptions/applicationexception.h"
#include "filtering/filteringsystem.h"
#include "miscellaneous/domdocument.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/labelsnode.h"

#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>

FilterMessage::FilterMessage(QObject* parent) : QObject(parent), m_message(nullptr) {}

void FilterMessage::setMessage(Message* message) {
  m_message = message;
}

bool FilterMessage::assignLabel(const QString& label_custom_id) const {
  Label* lbl = boolinq::from(m_system->availableLabels()).firstOrDefault([label_custom_id](Label* lbl) {
    return lbl->customId() == label_custom_id;
  });

  if (lbl != nullptr) {
    if (!m_message->m_assignedLabels.contains(lbl)) {
      m_message->m_assignedLabels.append(lbl);
    }

    return true;
  }
  else {
    return false;
  }
}

bool FilterMessage::deassignLabel(const QString& label_custom_id) const {
  Label* lbl = boolinq::from(m_message->m_assignedLabels).firstOrDefault([label_custom_id](Label* lbl) {
    return lbl->customId() == label_custom_id;
  });

  if (lbl != nullptr) {
    m_message->m_assignedLabels.removeAll(lbl);
    return true;
  }
  else {
    return false;
  }
}

void FilterMessage::deassignAllLabels() const {
  m_message->m_assignedLabels.clear();
}

void FilterMessage::exportCategoriesToLabels(bool assign_to_message) const {
  if (m_system->mode() == FilteringSystem::FiteringUseCase::ExistingArticles) {
    return;
  }

  for (QSharedPointer<MessageCategory>& cate : m_message->m_categories) {
    auto lbl = m_system->filterAccount().createLabel(cate->title());

    if (assign_to_message) {
      assignLabel(lbl);
    }
  }
}

void FilterMessage::addEnclosure(const QString& url, const QString& mime_type) const {
  m_message->m_enclosures.append(QSharedPointer<MessageEnclosure>(new MessageEnclosure(url, mime_type)));
}

bool FilterMessage::removeEnclosure(int index) const {
  m_message->m_enclosures.removeAt(index);

  return index <= m_message->m_enclosures.size();
}

void FilterMessage::removeAllEnclosures() const {
  m_message->m_enclosures.clear();
}

QString FilterMessage::title() const {
  return m_message->m_title;
}

void FilterMessage::setTitle(const QString& title) {
  m_message->m_title = title;
}

QString FilterMessage::url() const {
  return m_message->m_url;
}

void FilterMessage::setUrl(const QString& url) {
  m_message->m_url = url;
}

QString FilterMessage::author() const {
  return m_message->m_author;
}

void FilterMessage::setAuthor(const QString& author) {
  m_message->m_author = author;
}

QString FilterMessage::contents() const {
  return m_message->m_contents;
}

void FilterMessage::setContents(const QString& contents) {
  m_message->m_contents = contents;
}

QString FilterMessage::rawContents() const {
  return m_message->m_rawContents;
}

void FilterMessage::setRawContents(const QString& raw_contents) {
  m_message->m_rawContents = raw_contents;
}

QDateTime FilterMessage::created() const {
  return m_message->m_created;
}

void FilterMessage::setCreated(const QDateTime& created) {
  m_message->m_created = created;
}

bool FilterMessage::createdIsMadeup() const {
  return !m_message->m_createdFromFeed;
}

void FilterMessage::setCreatedIsMadeup(bool madeup) {
  m_message->m_createdFromFeed = !madeup;
}

bool FilterMessage::isRead() const {
  return m_message->m_isRead;
}

void FilterMessage::setIsRead(bool is_read) {
  m_message->m_isRead = is_read;
}

bool FilterMessage::isImportant() const {
  return m_message->m_isImportant;
}

void FilterMessage::setIsImportant(bool is_important) {
  m_message->m_isImportant = is_important;
}

bool FilterMessage::isDeleted() const {
  return m_message->m_isDeleted;
}

void FilterMessage::setIsDeleted(bool is_deleted) {
  m_message->m_isDeleted = is_deleted;
}

double FilterMessage::score() const {
  return m_message->m_score;
}

void FilterMessage::setScore(double score) {
  m_message->m_score = score;
}

void FilterMessage::setSystem(FilteringSystem* sys) {
  m_system = sys;
}

int FilterMessage::feedId() const {
  if (m_system->feed() == nullptr || m_system->feed()->customId() == QString::number(NO_PARENT_CATEGORY)) {
    return m_message->m_feedId;
  }
  else {
    return m_system->feed()->id();
  }
}

QString FilterMessage::customId() const {
  return m_message->m_customId;
}

void FilterMessage::setCustomId(const QString& custom_id) {
  m_message->m_customId = custom_id;
}

int FilterMessage::id() const {
  return m_message->m_id;
}

double jaro_winkler_distance(QString str1, QString str2) {
  qsizetype len1 = str1.size();
  qsizetype len2 = str2.size();

  if (len1 < len2) {
    std::swap(str1, str2);
    std::swap(len1, len2);
  }

  if (len2 == 0) {
    return len1 == 0 ? 0.0 : 1.0;
  }

  qsizetype delta = std::max(qsizetype(1), len1 / 2) - 1;
  std::vector<bool> flag(len2, false);
  std::vector<QChar> ch1_match;
  ch1_match.reserve(len1);

  for (uint idx1 = 0; idx1 < len1; ++idx1) {
    QChar ch1 = str1[idx1];

    for (uint idx2 = 0; idx2 < len2; ++idx2) {
      QChar ch2 = str2[idx2];

      if (idx2 <= idx1 + delta && idx2 + delta >= idx1 && ch1 == ch2 && !flag[idx2]) {
        flag[idx2] = true;
        ch1_match.push_back(ch1);
        break;
      }
    }
  }

  size_t matches = ch1_match.size();

  if (matches == 0) {
    return 1.0;
  }

  size_t transpositions = 0;

  for (uint idx1 = 0, idx2 = 0; idx2 < len2; ++idx2) {
    if (flag[idx2]) {
      if (str2[idx2] != ch1_match[idx1]) {
        ++transpositions;
      }

      ++idx1;
    }
  }

  double m = matches;
  double jaro = (m / len1 + m / len2 + (m - transpositions / 2.0) / m) / 3.0;
  size_t common_prefix = 0;
  len2 = std::min(qsizetype(4), len2);

  for (uint i = 0; i < len2; ++i) {
    if (str1[i] == str2[i]) {
      ++common_prefix;
    }
  }

  return 1.0 - (jaro + common_prefix * 0.1 * (1.0 - jaro));
}

#define JARO_WINKLER_DECIDE(attr_check, dupl_check, thres, my_msg_prop, other_msg_prop) \
  if (Globals::hasFlag(attr_check, dupl_check)) {                                       \
    double dst = jaro_winkler_distance(my_msg_prop, other_msg_prop);                    \
    if (dst > thres) {                                                                  \
      continue;                                                                         \
    }                                                                                   \
  }

bool FilterMessage::isAlreadyInDatabaseWinkler(DuplicityCheck criteria, double threshold) const {
  QList<Message> msgs;

  try {
    if (Globals::hasFlag(criteria, DuplicityCheck::AllFeedsSameAccount)) {
      msgs = DatabaseQueries::getUndeletedMessagesForAccount(m_system->database(), m_system->filterAccount().id(), {});
    }
    else {
      msgs = DatabaseQueries::getUndeletedMessagesForFeed(m_system->database(), feedId(), {});
    }
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_ARTICLEFILTER << "Query for undeleted articles failed:" << QUOTE_W_SPACE_DOT(ex.message());
    return false;
  }

  foreach (const Message& msg, msgs) {
    // We check similarity of each article.
    if (m_system->mode() == FilteringSystem::FiteringUseCase::ExistingArticles && id() > 0 && msg.m_id == id()) {
      // NOTE: We skip this message because it is the same one.
      return false;
    }

    JARO_WINKLER_DECIDE(criteria, DuplicityCheck::SameTitle, threshold, title(), msg.m_title)
    JARO_WINKLER_DECIDE(criteria, DuplicityCheck::SameUrl, threshold, url(), msg.m_url)
    JARO_WINKLER_DECIDE(criteria, DuplicityCheck::SameAuthor, threshold, author(), msg.m_author)
    JARO_WINKLER_DECIDE(criteria,
                        DuplicityCheck::SameDateCreated,
                        threshold,
                        created().toString(),
                        msg.m_created.toString())
    JARO_WINKLER_DECIDE(criteria, DuplicityCheck::SameCustomId, threshold, customId(), msg.m_customId)

    return true;
  }

  return false;
}

bool FilterMessage::isAlreadyInDatabase(DuplicityCheck criteria) const {
  // Check database according to duplication attribute_check.
  SqlQuery q(m_system->database());
  QStringList where_clauses;
  QVector<QPair<QString, QVariant>> bind_values;

  // Now we construct the query according to parameter.
  if (Globals::hasFlag(criteria, DuplicityCheck::SameTitle)) {
    where_clauses.append(QSL("title = :title"));
    bind_values.append({QSL(":title"), title()});
  }

  if (Globals::hasFlag(criteria, DuplicityCheck::SameUrl)) {
    where_clauses.append(QSL("url = :url"));
    bind_values.append({QSL(":url"), url()});
  }

  if (Globals::hasFlag(criteria, DuplicityCheck::SameAuthor)) {
    where_clauses.append(QSL("author = :author"));
    bind_values.append({QSL(":author"), author()});
  }

  if (Globals::hasFlag(criteria, DuplicityCheck::SameDateCreated)) {
    where_clauses.append(QSL("date_created = :date_created"));
    bind_values.append({QSL(":date_created"), created().toMSecsSinceEpoch()});
  }

  if (Globals::hasFlag(criteria, DuplicityCheck::SameCustomId)) {
    where_clauses.append(QSL("custom_id = :custom_id"));
    bind_values.append({QSL(":custom_id"), customId()});
  }

  where_clauses.append(QSL("account_id = :account_id"));
  bind_values.append({QSL(":account_id"), m_system->filterAccount().id()});

  // If we have already message stored in DB, then we also must
  // make sure that we do not match the message against itself.
  if (m_system->mode() == FilteringSystem::FiteringUseCase::ExistingArticles && id() > 0) {
    where_clauses.append(QSL("id != :id"));
    bind_values.append({QSL(":id"), QString::number(id())});
  }

  if (!Globals::hasFlag(criteria, DuplicityCheck::AllFeedsSameAccount)) {
    // Limit to current feed.
    where_clauses.append(QSL("feed = :feed"));
    bind_values.append({QSL(":feed"), feedId()});
  }

  QString full_query = QSL("SELECT COUNT(*) FROM Messages WHERE ") + where_clauses.join(QSL(" AND ")) + QSL(";");
  q.prepare(full_query);

  for (const auto& bind : bind_values) {
    q.bindValue(bind.first, bind.second);
  }

  if (q.exec(false) && q.next()) {
    if (q.value(0).toInt() > 0) {
      // Whoops, we have the "same" message in database.
      qDebugNN << LOGSEC_CORE << "Message" << QUOTE_W_SPACE(title()) << "was identified as duplicate by filter script.";
      return true;
    }
  }
  else if (q.lastError().isValid()) {
    qWarningNN << LOGSEC_CORE << "Error when checking for duplicate messages via filtering system, error:"
               << QUOTE_W_SPACE_DOT(q.lastError().text());
  }

  return false;
}

bool FilterMessage::fetchFullContents(bool plain_text_only) {
  QUrl url = m_message->m_url;

  if (!url.isValid() || url.isEmpty()) {
    qWarningNN << LOGSEC_CORE << "Cannot call article extractor with empty or invalid URL.";
    return false;
  }

  try {
    QString full_contents = qApp->feedReader()->getFullArticle(url, plain_text_only);

    if (full_contents.simplified().isEmpty()) {
      qWarningNN << LOGSEC_CORE << "Empty contents returned from article extractor for URL"
                 << QUOTE_W_SPACE_DOT(m_message->m_url);
      return false;
    }

    setContents(full_contents);
    return true;
  }
  catch (const ApplicationException& ex) {
    qWarningNN << LOGSEC_CORE << "Error" << QUOTE_W_SPACE(ex.message()) << "when calling article extractor for URL"
               << QUOTE_W_SPACE_DOT(m_message->m_url);
    return false;
  }
}

QList<Label*> FilterMessage::assignedLabels() const {
  return m_message->m_assignedLabels;
}

QList<MessageCategory*> FilterMessage::categories() const {
  auto cats = m_message->m_categories;
  auto std_cats = boolinq::from(cats)
                    .select([](const QSharedPointer<MessageCategory>& cat) {
                      return cat.data();
                    })
                    .toStdList();

  return FROM_STD_LIST(QList<MessageCategory*>, std_cats);
}

QList<MessageEnclosure*> FilterMessage::enclosures() const {
  auto cats = m_message->m_enclosures;
  auto std_cats = boolinq::from(cats)
                    .select([](const QSharedPointer<MessageEnclosure>& cat) {
                      return cat.data();
                    })
                    .toStdList();

  return FROM_STD_LIST(QList<MessageEnclosure*>, std_cats);
}

bool FilterMessage::hasEnclosures() const {
  return !m_message->m_enclosures.isEmpty();
}

FilterUtils::FilterUtils(QObject* parent) : QObject(parent) {}

FilterUtils::~FilterUtils() {
  qDebugNN << "Destroying FilterUtils instance.";
}

QString FilterUtils::hostname() const {
  return QHostInfo::localHostName();
}

QString jsonEscapeString(const QString& s) {
  return QString(QJsonDocument(QJsonArray() << s).toJson(QJsonDocument::JsonFormat::Compact)).mid(2).chopped(2);
}

QString jsonProcessXmlElement(const QDomElement& elem) {
  QStringList attrs;

  for (int i = 0; i < elem.attributes().size(); i++) {
    attrs << QSL("\"%1\": \"%2\"")
               .arg(jsonEscapeString(elem.attributes().item(i).toAttr().name()),
                    jsonEscapeString(elem.attributes().item(i).toAttr().value()));
  }

  QStringList elems;
  QString elem_text;

  for (int i = 0; i < elem.childNodes().size(); i++) {
    QDomNode el = elem.childNodes().at(i);

    if (el.isText()) {
      elem_text = jsonEscapeString(el.nodeValue());
    }

    if (!el.isElement()) {
      continue;
    }

    elems << QSL("\"%1\": %2").arg(el.toElement().tagName(), jsonProcessXmlElement(el.toElement()));
  }

  QString str;

  if (!elems.isEmpty() && !attrs.isEmpty()) {
    str =
      QSL("{%1, %2, %3}").arg(attrs.join(QSL(",\n")), elems.join(QSL(",\n")), QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else if (!elems.isEmpty()) {
    str = QSL("{%1, %2}").arg(elems.join(QSL(",\n")), QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else if (!attrs.isEmpty()) {
    str = QSL("{%1, %2}").arg(attrs.join(QSL(",\n")), QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else {
    str = QSL("{%1}").arg(QSL("\"__text\": \"%1\"").arg(elem_text));
  }

  return str;
}

QString FilterUtils::fromXmlToJson(const QString& xml) const {
  DomDocument xml_doc;

  xml_doc.setContent(xml, true);

  QString json = QSL("%1").arg(jsonProcessXmlElement(xml_doc.documentElement()));

  return QSL("{\"%1\": %2}").arg(xml_doc.documentElement().tagName(), json);
}

QDateTime FilterUtils::parseDateTime(const QString& dat) const {
  return TextFactory::parseDateTime(dat);
}

QByteArray FilterUtils::readFile(const QString& filename) const {
  return IOFactory::readFile(filename);
}

void FilterUtils::writeFile(const QString& filename, const QByteArray& data) const {
  IOFactory::writeFile(filename, data);
}

QString FilterUtils::readTextFile(const QString& filename) const {
  return QString::fromUtf8(IOFactory::readFile(filename));
}

void FilterUtils::writeTextFile(const QString& filename, const QString& data) const {
  IOFactory::writeFile(filename, data.toUtf8());
}

QString FilterFs::runExecutableGetOutput(const QString& executable,
                                         const QStringList& arguments,
                                         const QString& stdin_data,
                                         const QString& working_directory) const {
  try {
    auto res =
      IOFactory::startProcessGetOutput(executable,
                                       arguments,
                                       stdin_data,
                                       working_directory.isEmpty() ? qApp->userDataFolder() : working_directory);

    return res;
  }
  catch (const ApplicationException& ex) {
    return ex.message();
  }
}

void FilterFs::runExecutable(const QString& executable,
                             const QStringList& arguments,
                             const QString& stdin_data,
                             const QString& working_directory) const {
  try {
    IOFactory::startProcessDetached(executable,
                                    arguments,
                                    stdin_data,
                                    working_directory.isEmpty() ? qApp->userDataFolder() : working_directory);
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_JS << "Error when running executable:" << QUOTE_W_SPACE_DOT(ex.message());
  }
}

void FilterFs::setSystem(FilteringSystem* sys) {
  m_system = sys;
}

void FilterUtils::setSystem(FilteringSystem* sys) {
  m_system = sys;
}

QString FilterAccount::findLabel(const QString& label_title) const {
  Label* found_lbl = boolinq::from(m_system->availableLabels()).firstOrDefault([label_title](Label* lbl) {
    return lbl->title().toLower() == label_title.toLower();
  });

  if (found_lbl == nullptr) {
    qWarningNN << LOGSEC_CORE << "Label with title" << QUOTE_W_SPACE(label_title) << "not found.";
  }

  return found_lbl != nullptr ? found_lbl->customId() : QString();
}

QString FilterAccount::createLabel(const QString& label_title, const QString& hex_color) {
  QString lbl_id = findLabel(label_title);

  if (!lbl_id.isEmpty()) {
    // Label exists.
    return lbl_id;
  }

  if (!Globals::hasFlag(m_system->account()->supportedLabelOperations(), ServiceRoot::LabelOperation::Adding)) {
    qWarningNN << LOGSEC_CORE << "This account does not support creating labels.";
    return nullptr;
  }

  Label* new_lbl = nullptr;

  try {
    auto rnd_color = TextFactory::generateRandomColor();

    new_lbl = new Label(label_title, hex_color.isEmpty() ? rnd_color : hex_color);
    QSqlDatabase db = m_system->database();

    DatabaseQueries::createLabel(db, new_lbl, m_system->account()->accountId());
    m_system->account()->requestItemReassignment(new_lbl, m_system->account()->labelsNode(), true);
    m_system->availableLabels().append(new_lbl);

    return new_lbl->customId();
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_CORE << "Cannot create label:" << QUOTE_W_SPACE_DOT(ex.message());

    if (new_lbl != nullptr) {
      new_lbl->deleteLater();
    }
  }

  return {};
}

void FilterApp::showNotification(const QString& title, const QString& text) {
  qApp->showGuiMessage(Notification::Event::GeneralEvent, GuiMessage(title, text));
}

void FilterApp::log(const QString& message) {
  qWarningNN << LOGSEC_ARTICLEFILTER << message;
  emit logged(message);
}

QList<Label*> FilterAccount::availableLabels() const {
  return m_system->availableLabels();
}

void FilterApp::setSystem(FilteringSystem* sys) {
  m_system = sys;
}

QString FilterFeed::title() const {
  return m_system->feed()->title();
}

QString FilterFeed::customId() const {
  return m_system->feed()->customId();
}

void FilterFeed::setSystem(FilteringSystem* sys) {
  m_system = sys;
}

FilterRun::FilterRun(QObject* parent) : QObject(parent), m_numberOfAcceptedMessages(0) {}

int FilterRun::numberOfAcceptedMessages() const {
  return m_numberOfAcceptedMessages;
}

void FilterRun::incrementNumberOfAcceptedMessages() {
  m_numberOfAcceptedMessages++;
}

int FilterRun::indexOfCurrentFilter() const {
  return m_indexOfCurrentFilter;
}

void FilterRun::setIndexOfCurrentFilter(int idx) {
  m_indexOfCurrentFilter = idx;
}

int FilterRun::totalCountOfFilters() const {
  return m_totalCountOfFilters;
}

void FilterRun::setTotalCountOfFilters(int total) {
  m_totalCountOfFilters = total;
}

QString FilterAccount::title() const {
  return m_system->account()->title();
}

int FilterAccount::id() const {
  return m_system->account() != nullptr ? m_system->account()->accountId() : NO_PARENT_CATEGORY;
}

void FilterAccount::setSystem(FilteringSystem* sys) {
  m_system = sys;
}
