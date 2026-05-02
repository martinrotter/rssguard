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
#include <QPrintDialog>
#include <QPrinter>
#include <QTimer>

WebViewer::WebViewer() {
  m_loadExternalResources = qApp->settings()->value(GROUP(Browser), SETTING(Browser::LoadExternalResources)).toBool();
  m_placeholderImage = qApp->icons()->miscPixmap(QSL("image-placeholder"));
  m_placeholderImageError = qApp->icons()->miscPixmap(QSL("image-placeholder-error"));
}

WebViewer::~WebViewer() {}

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

void WebViewer::copySelectedText() {
  auto* clip = QGuiApplication::clipboard();

  if (clip != nullptr && !m_contextMenuData.m_selectedText.trimmed().isEmpty()) {
    clip->setText(m_contextMenuData.m_selectedText);
  }
}

void WebViewer::copySelectedLink() {
  auto* clip = QGuiApplication::clipboard();

  if (clip != nullptr && m_contextMenuData.m_linkUrl.isValid()) {
    clip->setText(m_contextMenuData.m_linkUrl.toString());
  }
}

void WebViewer::copySelectedImage() {
  auto* clip = QGuiApplication::clipboard();

  if (clip != nullptr && m_contextMenuData.m_imgLinkUrl.isValid()) {
    // TODO stahnout img a nastavit
    // clip->setPixmap(m_contextMenuData.m_img);
  }
}

void WebViewer::copySelectedImageLink() {
  auto* clip = QGuiApplication::clipboard();

  if (clip != nullptr && m_contextMenuData.m_imgLinkUrl.isValid()) {
    clip->setText(m_contextMenuData.m_imgLinkUrl.toString());
  }
}

void WebViewer::saveImageAs() {
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
    // TODO: todo
    // m_contextMenuData.m_imgLinkUrl.save(filename);
  }
}

void WebViewer::processContextMenu(QMenu* specific_menu, QContextMenuEvent* event) {
  specific_menu->setTitle(QObject::tr("Context menu for article viewer"));

  // Setup the menu.
  m_contextMenuData = provideContextMenuData(event);
  initializeCommonMenuItems();

  // Open.
  specific_menu->addAction(m_actionOpenNewTab.data());
  specific_menu->addAction(m_actionOpenExternalBrowser.data());
  specific_menu->addAction(m_actionPlayLink.data());

  // Tools.
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

  specific_menu->addSeparator();
  specific_menu->addAction(m_actionCopyText.data());
  specific_menu->addAction(m_actionCopyLink.data());
  specific_menu->addAction(m_actionCopyImage.data());
  specific_menu->addAction(m_actionCopyImageLink.data());
  specific_menu->addAction(m_actionSaveImage.data());
  specific_menu->addSeparator();
  specific_menu->addAction(m_actionPrint.data());
  specific_menu->addAction(m_actionSaveHtml.data());

  if (supportImagesLoading()) {
    specific_menu->addSeparator();
    specific_menu->addAction(m_actionExternalResources.data());
  }

  // Enable/disable.
  m_actionPrint->setEnabled(!html().simplified().isEmpty());
  m_actionSaveHtml->setEnabled(!html().simplified().isEmpty());
  m_actionOpenNewTab->setEnabled(m_contextMenuData.m_linkUrl.isValid());
  m_actionOpenExternalBrowser->setEnabled(m_contextMenuData.m_linkUrl.isValid());
  m_actionCopyText->setEnabled(!m_contextMenuData.m_selectedText.isEmpty());
  m_actionCopyLink->setEnabled(m_contextMenuData.m_linkUrl.isValid());
  m_actionCopyImage->setEnabled(m_contextMenuData.m_imgLinkUrl.isValid());
  m_actionCopyImageLink->setEnabled(m_contextMenuData.m_imgLinkUrl.isValid());
  m_actionSaveImage->setEnabled(m_contextMenuData.m_imgLinkUrl.isValid());

#if defined(ENABLE_MEDIAPLAYER)
  m_actionPlayLink->setEnabled(m_contextMenuData.m_linkUrl.isValid());
#endif
}

void WebViewer::saveHtmlAs() {
  QString filename = (url().isValid() && !url().host().isEmpty()) ? url().host() : QSL("contents");

  QString filter_html = QObject::tr("HTML files (*.htm *.html)");
  QString filter_plaintext = QObject::tr("Plain text files (*.txt)");
  QString filter;
  QString filter_selected;

  filter += filter_html;
  filter += QSL(";;");
  filter += filter_plaintext;

  QString selected_file = FileDialog::saveFileName(nullptr,
                                                   QObject::tr("Save contents in HTML or TXT format"),
                                                   qApp->documentsFolder(),
                                                   QSL("%1.html").arg(filename),
                                                   filter,
                                                   &filter_selected,
                                                   GENERAL_REMEMBERED_PATH);

  if (selected_file.isEmpty()) {
    return;
  }

  if (filter_selected == filter_html) {
    IOFactory::writeFile(selected_file, html().toUtf8());
  }

  if (filter_selected == filter_plaintext) {
    IOFactory::writeFile(selected_file, plainText().toUtf8());
  }
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

    qApp->web()->openUrlInExternalBrowser(resolved_url.toString(), true);

    if (qApp->settings()
          ->value(GROUP(Messages), SETTING(Messages::BringAppToFrontAfterMessageOpenedExternally))
          .toBool()) {
      QTimer::singleShot(1000, qApp, []() {
        qApp->mainForm()->display();
      });
    }
  }
}

void WebViewer::openClickedLinkInNewTab() {
  auto context_url = m_contextMenuData.m_linkUrl;

  if (context_url.isValid()) {
    const QUrl resolved_url = (url().isValid() && context_url.isRelative()) ? url().resolved(context_url) : context_url;

    emit openUrlInNewTab(false, resolved_url);
  }
}

void WebViewer::printContents() {
  if (m_printer.isNull()) {
    m_printer.reset(new QPrinter());
  }

  QPrintDialog d(m_printer.data(), qApp->mainFormWidget());

  if (d.exec() != QDialog::DialogCode::Accepted) {
    return;
  }

  printToPrinter(m_printer.data());
}

void WebViewer::initializeCommonMenuItems() {
  if (!m_actionOpenExternalBrowser.isNull()) {
    return;
  }

  m_actionPrint.reset(new QAction(qApp->icons()->fromTheme(QSL("document-print"), QSL("printer")),
                                  QObject::tr("Print...")));
  m_actionExternalResources.reset(new QAction(qApp->icons()->fromTheme(QSL("applications-internet")),
                                              QObject::tr("Load external images")));
  m_actionSaveHtml.reset(new QAction(qApp->icons()->fromTheme(QSL("document-save-as")), QObject::tr("Save as...")));
  m_actionOpenNewTab.reset(new QAction(qApp->icons()->fromTheme(QSL("link"), QSL("document-open")),
                                       QObject::tr("Open in new tab")));
  m_actionOpenExternalBrowser.reset(new QAction(qApp->icons()->fromTheme(QSL("document-open")),
                                                QObject::tr("Open in external browser")));
  m_actionPlayLink.reset(new QAction(qApp->icons()->fromTheme(QSL("player_play"), QSL("media-playback-start")),
                                     QObject::tr("Play in media player")));
  m_actionCopyText.reset(new QAction(qApp->icons()->fromTheme(QSL("edit-copy")), QObject::tr("Copy text")));
  m_actionCopyLink.reset(new QAction(qApp->icons()->fromTheme(QSL("edit-copy")), QObject::tr("Copy link")));
  m_actionCopyImage.reset(new QAction(qApp->icons()->fromTheme(QSL("viewimage"), QSL("image-x-generic")),
                                      QObject::tr("Copy image")));
  m_actionCopyImageLink.reset(new QAction(qApp->icons()->fromTheme(QSL("viewimage"), QSL("image-x-generic")),
                                          QObject::tr("Copy image link")));
  m_actionSaveImage.reset(new QAction(qApp->icons()->fromTheme(QSL("download"), QSL("document-save-as")),
                                      QObject::tr("Save image")));

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
  QObject::connect(m_actionOpenNewTab.data(), &QAction::triggered, m_actionOpenNewTab.data(), [this]() {
    openClickedLinkInNewTab();
  });
  QObject::connect(m_actionExternalResources.data(),
                   &QAction::triggered,
                   m_actionExternalResources.data(),
                   [this](bool checked) {
                     setLoadExternalResources(checked);
                   });
  QObject::connect(m_actionPrint.data(), &QAction::triggered, m_actionPrint.data(), [this]() {
    printContents();
  });
  QObject::connect(m_actionSaveHtml.data(), &QAction::triggered, m_actionSaveHtml.data(), [this]() {
    saveHtmlAs();
  });
  QObject::connect(m_actionPlayLink.data(), &QAction::triggered, m_actionPlayLink.data(), [this]() {
    playClickedLinkAsMedia();
  });

  QObject::connect(m_actionCopyText.data(), &QAction::triggered, m_actionCopyText.data(), [this]() {
    copySelectedText();
  });
  QObject::connect(m_actionCopyLink.data(), &QAction::triggered, m_actionCopyLink.data(), [this]() {
    copySelectedLink();
  });
  QObject::connect(m_actionCopyImage.data(), &QAction::triggered, m_actionCopyImage.data(), [this]() {
    copySelectedImage();
  });
  QObject::connect(m_actionCopyImageLink.data(), &QAction::triggered, m_actionCopyImageLink.data(), [this]() {
    copySelectedImageLink();
  });
  QObject::connect(m_actionSaveImage.data(), &QAction::triggered, m_actionSaveImage.data(), [this]() {
    saveImageAs();
  });
}

bool WebViewer::loadExternalResources() const {
  return m_loadExternalResources;
}

void WebViewer::setLoadExternalResources(bool load_resources) {
  m_loadExternalResources = load_resources;
  qApp->settings()->setValue(GROUP(Browser), Browser::LoadExternalResources, load_resources);
}

void WebViewer::onPrintingFinished(bool success) {
  if (success) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(QObject::tr("Done"),
                                    QObject::tr("Printing is finished on printer %1.")
                                      .arg(m_printer.data()->printerName())),
                         GuiMessageDestination(true, true, true));
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(QObject::tr("Error"),
                                    QObject::tr("Printing failed."),
                                    QSystemTrayIcon::MessageIcon::Critical),
                         GuiMessageDestination(true, true, true));
  }
}
