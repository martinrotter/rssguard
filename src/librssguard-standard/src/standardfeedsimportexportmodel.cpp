// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/standardfeedsimportexportmodel.h"

#include "src/definitions.h"
#include "src/standardcategory.h"
#include "src/standardfeed.h"
#include "src/standardserviceroot.h"

#include <librssguard/3rd-party/boolinq/boolinq.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/domdocument.h>
#include <librssguard/miscellaneous/iconfactory.h>

#include <QDomAttr>
#include <QDomElement>
#include <QLocale>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStack>
#include <QtConcurrentMap>

FeedsImportExportModel::FeedsImportExportModel(StandardServiceRoot* account, QObject* parent)
  : AccountCheckSortedModel(parent), m_account(account), m_mode(Mode::Import), m_newRoot(nullptr) {
  connect(&m_watcherLookup, &QFutureWatcher<bool>::progressValueChanged, this, [=](int prog) {
    emit parsingProgress(prog, m_lookup.size());
  });

  connect(&m_watcherLookup, &QFutureWatcher<bool>::finished, this, [=]() {
    auto res = m_watcherLookup.future().results();
    int number_error = boolinq::from(res).count(false);

    emit layoutAboutToBeChanged();
    setRootItem(m_newRoot);
    emit layoutChanged();

    m_newRoot = nullptr;

    emit parsingFinished(number_error, res.size() - number_error);

    // Done, remove lookups.
    m_lookup.clear();
  });
}

FeedsImportExportModel::~FeedsImportExportModel() {
  if (m_watcherLookup.isRunning()) {
    m_watcherLookup.cancel();
    m_watcherLookup.waitForFinished();
    QCoreApplication::processEvents();
  }

  if (sourceModel() != nullptr && sourceModel()->rootItem() != nullptr && m_mode == Mode::Import) {
    // Delete all model items, but only if we are in import mode. Export mode shares
    // root item with main feed model, thus cannot be deleted from memory now.
    delete sourceModel()->rootItem();
  }
}

bool FeedsImportExportModel::exportToOMPL20(QByteArray& result, bool export_icons) {
  DomDocument opml_document;
  QDomProcessingInstruction xml_declaration =
    opml_document.createProcessingInstruction(QSL("xml"), QSL("version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\""));

  opml_document.appendChild(xml_declaration);

  // Added OPML 2.0 metadata.
  opml_document.appendChild(opml_document.createElement(QSL("opml")));
  opml_document.documentElement().setAttribute(QSL("version"), QSL("2.0"));

#if QT_VERSION_MAJOR == 5
  opml_document.documentElement().setAttribute(QSL("xmlns:rssguard"), QSL(APP_URL));
#endif

  QDomElement elem_opml_head = opml_document.createElement(QSL("head"));
  QDomElement elem_opml_title = opml_document.createElement(QSL("title"));
  QDomText text_opml_title = opml_document.createTextNode(QSL(APP_NAME));

  elem_opml_title.appendChild(text_opml_title);
  elem_opml_head.appendChild(elem_opml_title);
  QDomElement elem_opml_created = opml_document.createElement(QSL("dateCreated"));
  QDomText text_opml_created = opml_document.createTextNode(QLocale::c().toString(QDateTime::currentDateTimeUtc(),
                                                                                  QSL("ddd, dd MMM yyyy hh:mm:ss")) +
                                                            QSL(" GMT"));

  elem_opml_created.appendChild(text_opml_created);
  elem_opml_head.appendChild(elem_opml_created);
  opml_document.documentElement().appendChild(elem_opml_head);
  QDomElement elem_opml_body = opml_document.createElement(QSL("body"));
  QStack<RootItem*> items_to_process;

  items_to_process.push(sourceModel()->rootItem());
  QStack<QDomElement> elements_to_use;

  elements_to_use.push(elem_opml_body);

  // Process all unprocessed nodes.
  while (!items_to_process.isEmpty()) {
    QDomElement active_element = elements_to_use.pop();
    RootItem* active_item = items_to_process.pop();
    auto chi = active_item->childItems();

    for (RootItem* child_item : std::as_const(chi)) {
      if (!sourceModel()->isItemChecked(child_item)) {
        continue;
      }

      switch (child_item->kind()) {
        case RootItem::Kind::Category: {
          QDomElement outline_category = opml_document.createElement(QSL("outline"));

          outline_category.setAttribute(QSL("text"), child_item->title());
          outline_category.setAttribute(QSL("description"), child_item->description());

          if (export_icons && !child_item->icon().isNull()) {
            outline_category.setAttributeNS(QSL(APP_URL),
                                            QSL("rssguard:icon"),
                                            QString(qApp->icons()->toByteArray(child_item->icon())));
          }

          active_element.appendChild(outline_category);
          items_to_process.push(child_item);
          elements_to_use.push(outline_category);
          break;
        }

        case RootItem::Kind::Feed: {
          auto* child_feed = qobject_cast<StandardFeed*>(child_item);
          QDomElement outline_feed = opml_document.createElement(QSL("outline"));

          outline_feed.setAttribute(QSL("type"), QSL("rss"));
          outline_feed.setAttribute(QSL("text"), child_feed->title());
          outline_feed.setAttribute(QSL("xmlUrl"), child_feed->source());
          outline_feed.setAttribute(QSL("description"), child_feed->description());
          outline_feed.setAttribute(QSL("encoding"), child_feed->encoding());
          outline_feed.setAttribute(QSL("title"), child_feed->title());

          outline_feed.setAttributeNS(QSL(APP_URL),
                                      QSL("rssguard:xmlUrlType"),
                                      QString::number(int(child_feed->sourceType())));
          outline_feed.setAttributeNS(QSL(APP_URL), QSL("rssguard:postProcess"), child_feed->postProcessScript());

          if (export_icons && !child_feed->icon().isNull()) {
            outline_feed.setAttributeNS(QSL(APP_URL),
                                        QSL("rssguard:icon"),
                                        QString(qApp->icons()->toByteArray(child_feed->icon())));
          }

          switch (child_feed->type()) {
            case StandardFeed::Type::Rss0X:
            case StandardFeed::Type::Rss2X:
              outline_feed.setAttribute(QSL("version"), QSL("RSS"));
              break;

            case StandardFeed::Type::Rdf:
              outline_feed.setAttribute(QSL("version"), QSL("RSS1"));
              break;

            case StandardFeed::Type::Atom10:
              outline_feed.setAttribute(QSL("version"), QSL("ATOM"));
              break;

            case StandardFeed::Type::Json:
              outline_feed.setAttribute(QSL("version"), QSL("JSON"));
              break;

            case StandardFeed::Type::Sitemap:
              outline_feed.setAttribute(QSL("version"), QSL("Sitemap"));
              break;

            default:
              break;
          }

          active_element.appendChild(outline_feed);
          break;
        }

        default:
          break;
      }
    }
  }

  opml_document.documentElement().appendChild(elem_opml_body);
  result = opml_document.toByteArray(2);
  return true;
}

bool FeedsImportExportModel::produceFeed(const FeedLookup& feed_lookup) {
  StandardFeed* new_feed = nullptr;

  try {
    if (feed_lookup.fetch_metadata_online) {
      StandardFeed::SourceType source_type =
        feed_lookup.custom_data.contains(QSL("sourceType"))
          ? feed_lookup.custom_data[QSL("sourceType")].value<StandardFeed::SourceType>()
          : StandardFeed::SourceType::Url;

      QString pp_script = !feed_lookup.custom_data[QSL("postProcessScript")].toString().isEmpty()
                            ? feed_lookup.custom_data[QSL("postProcessScript")].toString()
                            : feed_lookup.post_process_script;

      try {
        auto new_feed_data = StandardFeed::guessFeed(source_type,
                                                     feed_lookup.url,
                                                     pp_script,
                                                     m_account,
                                                     NetworkFactory::NetworkAuthentication::NoAuthentication,
                                                     !feed_lookup.do_not_fetch_icons,
                                                     {},
                                                     {},
                                                     {},
                                                     feed_lookup.custom_proxy);

        new_feed_data.first->setSourceType(source_type);
        new_feed_data.first->setPostProcessScript(pp_script);

        new_feed = new_feed_data.first;

        if (feed_lookup.do_not_fetch_titles) {
          QString old_title = feed_lookup.custom_data[QSL("title")].toString();

          if (!old_title.simplified().isEmpty()) {
            new_feed->setTitle(old_title);
          }
        }

        if (feed_lookup.do_not_fetch_icons) {
          QIcon old_icon = feed_lookup.custom_data[QSL("icon")].value<QIcon>();

          if (old_icon.isNull()) {
            new_feed->setIcon(qApp->icons()->fromTheme(QSL("application-rss+xml")));
          }
          else {
            new_feed->setIcon(old_icon);
          }
        }
      }
      catch (...) {
        if (feed_lookup.add_errored_feeds) {
          // Feed guessing failed, add like regular feed anyway.
          new_feed = new StandardFeed();
          new_feed->setStatus(Feed::Status::OtherError);

          fillFeedFromFeedLookupData(new_feed, feed_lookup);
        }
        else {
          throw;
        }
      }
    }
    else {
      new_feed = new StandardFeed();
      fillFeedFromFeedLookupData(new_feed, feed_lookup);
    }

    QMutexLocker mtx(&m_mtxLookup);
    feed_lookup.parent->appendChild(new_feed);

    return true;
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_STANDARD << "Cannot fetch medatada for feed:" << QUOTE_W_SPACE(feed_lookup.url)
                << "with error:" << QUOTE_W_SPACE_DOT(ex.message());

    if (new_feed != nullptr) {
      new_feed->deleteLater();
    }

    return false;
  }
}

void FeedsImportExportModel::fillFeedFromFeedLookupData(StandardFeed* feed, const FeedLookup& feed_lookup) {
  if (feed_lookup.custom_data.isEmpty()) {
    // We assume these are "best-guess" defaults.
    feed->setSourceType(StandardFeed::SourceType::Url);
    feed->setType(StandardFeed::Type::Rss2X);
    feed->setSource(feed_lookup.url);
    feed->setTitle(feed_lookup.url);
    feed->setIcon(qApp->icons()->fromTheme(QSL("application-rss+xml")));
    feed->setEncoding(QSL(DEFAULT_FEED_ENCODING));
    feed->setPostProcessScript(feed_lookup.post_process_script);
  }
  else {
    QString feed_title = feed_lookup.custom_data[QSL("title")].toString();
    QString feed_encoding = feed_lookup.custom_data.value(QSL("encoding"), QSL(DEFAULT_FEED_ENCODING)).toString();
    QString feed_type = feed_lookup.custom_data.value(QSL("type"), QSL(DEFAULT_FEED_TYPE)).toString().toUpper();
    QString feed_description = feed_lookup.custom_data[QSL("description")].toString();
    QIcon feed_icon = feed_lookup.custom_data[QSL("icon")].value<QIcon>();
    StandardFeed::SourceType source_type = feed_lookup.custom_data[QSL("sourceType")].value<StandardFeed::SourceType>();
    QString post_process = feed_lookup.custom_data[QSL("postProcessScript")].toString();

    feed->setTitle(feed_title);
    feed->setDescription(feed_description);
    feed->setEncoding(feed_encoding);
    feed->setSource(feed_lookup.url);
    feed->setSourceType(source_type);
    feed->setPostProcessScript(feed_lookup.post_process_script.isEmpty() ? post_process
                                                                         : feed_lookup.post_process_script);

    if (!feed_icon.isNull()) {
      feed->setIcon(feed_icon);
    }

    if (feed_type == QL1S("RSS1")) {
      feed->setType(StandardFeed::Type::Rdf);
    }
    else if (feed_type == QL1S("JSON")) {
      feed->setType(StandardFeed::Type::Json);
    }
    else if (feed_type == QL1S("ATOM")) {
      feed->setType(StandardFeed::Type::Atom10);
    }
    else {
      feed->setType(StandardFeed::Type::Rss2X);
    }
  }
}

void FeedsImportExportModel::importAsOPML20(const QByteArray& data,
                                            bool fetch_metadata_online,
                                            bool do_not_fetch_titles,
                                            bool do_not_fetch_icons,
                                            const QString& post_process_script) {
  emit parsingStarted();
  emit layoutAboutToBeChanged();
  setRootItem(nullptr);
  emit layoutChanged();

  DomDocument opml_document;

  if (!opml_document.setContent(data, true)) {
    throw ApplicationException(tr("OPML document contains errors"));
  }

  if (opml_document.documentElement().isNull() || opml_document.documentElement().tagName() != QSL("opml") ||
      opml_document.documentElement().elementsByTagName(QSL("body")).size() != 1) {
    throw ApplicationException(tr("this is likely not OPML document"));
  }

  int completed = 0, total = 0;
  m_newRoot = new StandardServiceRoot();
  QStack<RootItem*> model_items;
  QNetworkProxy custom_proxy;

  if (m_account != nullptr) {
    custom_proxy = m_account->networkProxy();
  }

  model_items.push(m_newRoot);
  QStack<QDomElement> elements_to_process;

  elements_to_process.push(opml_document.documentElement().elementsByTagName(QSL("body")).at(0).toElement());
  total = opml_document.elementsByTagName(QSL("outline")).size();

  QList<FeedLookup> lookup;

  while (!elements_to_process.isEmpty()) {
    RootItem* active_model_item = model_items.pop();
    QDomElement active_element = elements_to_process.pop();
    int current_count = active_element.childNodes().size();

    for (int i = 0; i < current_count; i++) {
      QDomNode child = active_element.childNodes().at(i);

      if (child.isElement()) {
        QDomElement child_element = child.toElement();

        // Now analyze if this element is category or feed.
        // NOTE: All feeds must include xmlUrl attribute and text attribute.
        if (child_element.hasAttribute(QSL("xmlUrl")) &&
            (child_element.hasAttribute(QSL("text")) || child_element.hasAttribute(QSL("title")))) {
          // This is FEED.
          // Add feed and end this iteration.
          QString feed_url = child_element.attribute(QSL("xmlUrl"));

          if (!feed_url.isEmpty()) {
            FeedLookup f;
            QVariantMap feed_data;

            feed_data[QSL("title")] = child_element.attribute(QSL("text"));

            if (feed_data[QSL("title")].toString().isEmpty()) {
              feed_data[QSL("title")] = child_element.attribute(QSL("title"));
            }

            feed_data[QSL("encoding")] = child_element.attribute(QSL("encoding"), QSL(DEFAULT_FEED_ENCODING));
            feed_data[QSL("type")] = child_element.attribute(QSL("version"), QSL(DEFAULT_FEED_TYPE)).toUpper();
            feed_data[QSL("description")] = child_element.attribute(QSL("description"));
            feed_data[QSL("icon")] =
              qApp->icons()->fromByteArray(child_element.attributeNS(QSL(APP_URL), QSL("icon")).toLocal8Bit());
            feed_data[QSL("sourceType")] =
              QVariant::fromValue(StandardFeed::SourceType(child_element.attributeNS(QSL(APP_URL), QSL("xmlUrlType"))
                                                             .toInt()));
            feed_data[QSL("postProcessScript")] = child_element.attributeNS(QSL(APP_URL), QSL("postProcess"));

            f.custom_proxy = custom_proxy;
            f.fetch_metadata_online = fetch_metadata_online;
            f.do_not_fetch_titles = do_not_fetch_titles;
            f.do_not_fetch_icons = do_not_fetch_icons;
            f.custom_data = feed_data;
            f.parent = active_model_item;
            f.post_process_script = post_process_script;
            f.url = feed_url;

            lookup.append(f);
          }
        }
        else {
          // This must be CATEGORY.
          // Add category and continue.
          QString category_title = child_element.attribute(QSL("text"));
          QString category_description = child_element.attribute(QSL("description"));
          QIcon category_icon =
            qApp->icons()->fromByteArray(child_element.attributeNS(QSL(APP_URL), QSL("icon")).toLocal8Bit());

          if (category_title.isEmpty()) {
            qWarningNN << LOGSEC_STANDARD
                       << "Given OMPL file provided category without valid text attribute. Using fallback name.";
            category_title = child_element.attribute(QSL("title"));

            if (category_title.isEmpty()) {
              category_title = tr("Category ") + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
            }
          }

          auto* new_category = new StandardCategory(active_model_item);

          new_category->setTitle(category_title);

          if (!category_icon.isNull()) {
            new_category->setIcon(category_icon);
          }

          new_category->setDescription(category_description);
          active_model_item->appendChild(new_category);

          // Children of this node must be processed later.
          elements_to_process.push(child_element);
          model_items.push(new_category);
        }

        emit parsingProgress(++completed, total);
      }
    }
  }

  m_lookup.clear();
  m_lookup.append(lookup);

  std::function<bool(const FeedLookup&)> func = [=](const FeedLookup& lookup) -> bool {
    return produceFeed(lookup);
  };

#if QT_VERSION_MAJOR == 5
  QFuture<bool> fut = QtConcurrent::mapped(m_lookup, func);
#else
  QFuture<bool> fut = QtConcurrent::mapped(qApp->workHorsePool(), m_lookup, func);
#endif

  m_watcherLookup.setFuture(fut);

  if (!fetch_metadata_online) {
    m_watcherLookup.waitForFinished();
    QCoreApplication::processEvents();
  }
}

bool FeedsImportExportModel::exportToTxtURLPerLine(QByteArray& result) {
  auto stf = sourceModel()->rootItem()->getSubTreeFeeds();

  for (const Feed* const feed : std::as_const(stf)) {
    result += feed->source() + QL1S("\n");
  }

  return true;
}

void FeedsImportExportModel::importAsTxtURLPerLine(const QByteArray& data,
                                                   bool fetch_metadata_online,
                                                   const QString& post_process_script) {
  emit parsingStarted();
  emit layoutAboutToBeChanged();
  setRootItem(nullptr);
  emit layoutChanged();

  int completed = 0;
  m_newRoot = new StandardServiceRoot();
  QNetworkProxy custom_proxy;

  if (m_account != nullptr) {
    custom_proxy = m_account->networkProxy();
  }

  QList<QByteArray> urls = data.split('\n');
  QList<FeedLookup> lookup;

  for (const QByteArray& url : urls) {
    if (!url.isEmpty()) {
      FeedLookup f;

      f.custom_proxy = custom_proxy;
      f.fetch_metadata_online = fetch_metadata_online;
      f.parent = m_newRoot;
      f.post_process_script = post_process_script;
      f.url = url;

      lookup.append(f);
    }
    else {
      qWarningNN << LOGSEC_STANDARD << "Detected empty URL when parsing input TXT [one URL per line] data.";
    }

    emit parsingProgress(++completed, urls.size());
  }

  m_lookup.clear();
  m_lookup.append(lookup);

  std::function<bool(const FeedLookup&)> func = [=](const FeedLookup& lookup) -> bool {
    return produceFeed(lookup);
  };

#if QT_VERSION_MAJOR == 5
  QFuture<bool> fut = QtConcurrent::mapped(m_lookup, func);
#else
  QFuture<bool> fut = QtConcurrent::mapped(qApp->workHorsePool(), m_lookup, func);
#endif

  m_watcherLookup.setFuture(fut);

  if (!fetch_metadata_online) {
    m_watcherLookup.waitForFinished();
    QCoreApplication::processEvents();
  }
}

FeedsImportExportModel::Mode FeedsImportExportModel::mode() const {
  return m_mode;
}

void FeedsImportExportModel::setMode(FeedsImportExportModel::Mode mode) {
  m_mode = mode;
}
