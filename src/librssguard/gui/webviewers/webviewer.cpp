// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/webviewer.h"

#include "gui/dialogs/filedialog.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"
#include "qtlinq/qtlinq.h"

#include <QClipboard>
#include <QFileIconProvider>
#include <QImageWriter>
#include <QTimer>

WebViewer::WebViewer() {
  reloadSettings();
}

WebViewer::~WebViewer() {}

void WebViewer::reloadSettings() {
  setLoadExternalResources(qApp->settings()->value(GROUP(Browser), SETTING(Browser::LoadExternalResources)).toBool());
}

QUrl WebViewer::urlForMessage(const Message& message, RootItem* root) const {
  if (!message.m_url.isEmpty()) {
    return message.m_url;
  }

  auto* feed = root != nullptr ? root->account()
                                   ->getItemFromSubTree([message](const RootItem* it) {
                                     return it->kind() == RootItem::Kind::Feed && it->id() == message.m_feedId;
                                   })
                                   ->toFeed()
                               : nullptr;

  if (feed != nullptr) {
    QUrl url(NetworkFactory::sanitizeUrl(feed->source()));

    if (url.isValid()) {
      QString deducted_url = url.scheme() + QSL("://") + (url.isLocalFile() ? url.toLocalFile() : url.host());
      return deducted_url;
    }
  }

  return QString();
}

void WebViewer::processContextMenu(QMenu* specific_menu, QContextMenuEvent* event) {
  // Setup the menu.
  m_contextMenuData = provideContextMenuData(event);
  initializeCommonMenuItems();

  // Add common items.
  auto* act_copy_text =
    new QAction(qApp->icons()->fromTheme(QSL("edit-copy")), QObject::tr("Copy text"), specific_menu);

  act_copy_text->setShortcut(QKeySequence(QKeySequence::StandardKey::Copy));
  act_copy_text->setEnabled(!m_contextMenuData.m_selectedText.isEmpty());

  QObject::connect(act_copy_text, &QAction::triggered, specific_menu, [this]() {
    auto* clip = QGuiApplication::clipboard();

    if (clip != nullptr) {
      clip->setText(m_contextMenuData.m_selectedText);
    }
  });

  specific_menu->addAction(act_copy_text);

  auto* act_copy_link =
    new QAction(qApp->icons()->fromTheme(QSL("edit-copy")), QObject::tr("Copy link"), specific_menu);
  act_copy_link->setEnabled(m_contextMenuData.m_linkUrl.isValid());

  QObject::connect(act_copy_link, &QAction::triggered, specific_menu, [this]() {
    auto* clip = QGuiApplication::clipboard();

    if (clip != nullptr) {
      clip->setText(m_contextMenuData.m_linkUrl.toString());
    }
  });

  specific_menu->addAction(act_copy_link);
  specific_menu->addSeparator();

  auto* act_copy_img_link = new QAction(qApp->icons()->fromTheme(QSL("viewimage"), QSL("image-x-generic")),
                                        QObject::tr("Copy image link"),
                                        specific_menu);
  act_copy_img_link->setEnabled(m_contextMenuData.m_imgLinkUrl.isValid());

  QObject::connect(act_copy_img_link, &QAction::triggered, specific_menu, [this]() {
    auto* clip = QGuiApplication::clipboard();

    if (clip != nullptr) {
      clip->setText(m_contextMenuData.m_imgLinkUrl.toString());
    }
  });

  specific_menu->addAction(act_copy_img_link);

  auto* act_copy_img = new QAction(qApp->icons()->fromTheme(QSL("viewimage"), QSL("image-x-generic")),
                                   QObject::tr("Copy image"),
                                   specific_menu);
  act_copy_img->setEnabled(!m_contextMenuData.m_img.isNull());

  QObject::connect(act_copy_img, &QAction::triggered, specific_menu, [this]() {
    auto* clip = QGuiApplication::clipboard();

    if (clip != nullptr) {
      clip->setPixmap(m_contextMenuData.m_img);
    }
  });

  specific_menu->addAction(act_copy_img);

  auto* act_save_img = new QAction(qApp->icons()->fromTheme(QSL("download"), QSL("document-save-as")),
                                   QObject::tr("Save image"),
                                   specific_menu);
  act_save_img->setEnabled(!m_contextMenuData.m_img.isNull());

  QObject::connect(act_save_img, &QAction::triggered, specific_menu, [this]() {
    auto supported_formats = QImageWriter::supportedImageFormats();
    auto list_formats = qlinq::from(supported_formats)
                          .select([](const QByteArray& frmt) {
                            return QSL("*.%1").arg(QString::fromLocal8Bit(frmt));
                          })
                          .toList();

    QString selected_filter;
    auto filename = FileDialog::saveFileName(qApp->mainFormWidget(),
                                             QObject::tr("Save image"),
                                             qApp->documentsFolder(),
                                             QObject::tr("image.%1").arg(QSL("png")),
                                             QObject::tr("Images (%1)").arg(list_formats.join(QL1C(' '))),
                                             &selected_filter,
                                             GENERAL_REMEMBERED_PATH);

    if (!filename.isEmpty()) {
      m_contextMenuData.m_img.save(filename);
    }
  });

  specific_menu->addAction(act_save_img);

  specific_menu->addSeparator();
  specific_menu->addAction(m_actionSaveHtml.data());
  specific_menu->addSeparator();
  specific_menu->addAction(m_actionExternalResources.data());
  specific_menu->addAction(m_actionOpenExternalBrowser.data());
  specific_menu->addAction(m_actionPlayLink.data());

  m_actionSaveHtml.data()->setEnabled(!html().simplified().isEmpty());
  m_actionOpenExternalBrowser.data()->setEnabled(m_contextMenuData.m_linkUrl.isValid());

#if defined(ENABLE_MEDIAPLAYER)
  m_actionPlayLink.data()->setEnabled(m_contextMenuData.m_linkUrl.isValid());
#endif

  if (m_contextMenuData.m_linkUrl.isValid()) {
    QFileIconProvider icon_provider;
    QMenu* menu_ext_tools = new QMenu(QObject::tr("Open with external tool"), specific_menu);
    auto tools = ExternalTool::toolsFromSettings();

    menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

    for (const ExternalTool& tool : std::as_const(tools)) {
      QAction* act_tool =
        new QAction(QFileInfo(tool.name().simplified().isEmpty() ? tool.executable() : tool.name().simplified())
                      .fileName(),
                    menu_ext_tools);

      act_tool->setIcon(icon_provider.icon(QFileInfo(tool.executable())));
      act_tool->setToolTip(tool.executable());
      act_tool->setData(QVariant::fromValue(tool));
      menu_ext_tools->addAction(act_tool);

      QObject::connect(act_tool, &QAction::triggered, act_tool, [this, act_tool]() {
        act_tool->data().value<ExternalTool>().run(m_contextMenuData.m_linkUrl.toString());
      });
    }

    if (menu_ext_tools->actions().isEmpty()) {
      QAction* act_not_tools = new QAction("No external tools activated");

      act_not_tools->setEnabled(false);
      menu_ext_tools->addAction(act_not_tools);
    }

    specific_menu->addMenu(menu_ext_tools);
  }
}

void WebViewer::saveHtmlAs() {
  QString selected_file = FileDialog::saveFileName(nullptr,
                                                   QObject::tr("Save article in HTML format"),
                                                   qApp->documentsFolder(),
                                                   QSL("content.html"),
                                                   QObject::tr("HTML files (*.htm *.html)"),
                                                   nullptr,
                                                   GENERAL_REMEMBERED_PATH);

  if (selected_file.isEmpty()) {
    return;
  }

  IOFactory::writeFile(selected_file, html().toUtf8());
}

void WebViewer::playClickedLinkAsMedia() {
#if defined(ENABLE_MEDIAPLAYER)
  auto context_url = m_contextMenuData.m_linkUrl;

  if (context_url.isValid()) {
    qApp->mainForm()->tabWidget()->addMediaPlayer(context_url.toString(), true);
  }
#endif
}

void WebViewer::openClickedLinkInExternalBrowser() {
  auto context_url = m_contextMenuData.m_linkUrl;

  if (context_url.isValid()) {
    const QUrl resolved_url = (url().isValid() && context_url.isRelative()) ? url().resolved(context_url) : context_url;

    qApp->web()->openUrlInExternalBrowser(resolved_url.toString());

    if (qApp->settings()
          ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
          .toBool()) {
      QTimer::singleShot(1000, qApp, []() {
        qApp->mainForm()->display();
      });
    }
  }
}

void WebViewer::initializeCommonMenuItems() {
  if (!m_actionOpenExternalBrowser.isNull()) {
    return;
  }

  m_actionExternalResources.reset(new QAction(qApp->icons()->fromTheme(QSL("applications-internet")),
                                              QObject::tr("Load external resources")));
  m_actionSaveHtml.reset(new QAction(qApp->icons()->fromTheme(QSL("document-save-as")),
                                     QObject::tr("Save article as...")));
  m_actionOpenExternalBrowser.reset(new QAction(qApp->icons()->fromTheme(QSL("document-open")),
                                                QObject::tr("Open in external browser")));

  m_actionPlayLink.reset(new QAction(qApp->icons()->fromTheme(QSL("player_play"), QSL("media-playback-start")),
                                     QObject::tr("Play in media player")));

#if !defined(ENABLE_MEDIAPLAYER)
  m_actionPlayLink->setText(m_actionPlayLink->text() + QSL(" ") + QObject::tr("(not supported)"));
  m_actionPlayLink->setEnabled(false);
#endif

  m_actionExternalResources->setCheckable(true);
  m_actionExternalResources->setChecked(loadExternalResources());

  QObject::connect(m_actionOpenExternalBrowser.data(),
                   &QAction::triggered,
                   m_actionOpenExternalBrowser.data(),
                   [this]() {
                     openClickedLinkInExternalBrowser();
                   });

  QObject::connect(m_actionExternalResources.data(),
                   &QAction::triggered,
                   m_actionExternalResources.data(),
                   [this](bool checked) {
                     setLoadExternalResources(checked);
                   });

  QObject::connect(m_actionSaveHtml.data(), &QAction::triggered, m_actionOpenExternalBrowser.data(), [this]() {
    saveHtmlAs();
  });

  QObject::connect(m_actionPlayLink.data(), &QAction::triggered, m_actionPlayLink.data(), [this]() {
    playClickedLinkAsMedia();
  });
}

bool WebViewer::loadExternalResources() const {
  return m_loadExternalResources;
}

void WebViewer::setLoadExternalResources(bool load_resources) {
  m_loadExternalResources = load_resources;
  qApp->settings()->setValue(GROUP(Browser), Browser::LoadExternalResources, load_resources);
}
