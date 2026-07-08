// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/webviewer.h"

#include "3rd-party/gumbo/src/gumbo.h"
#include "gui/dialogs/filedialog.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "qtlinq/qtlinq.h"

#include <cstring>

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

namespace {
  bool isGumboVoidTag(GumboTag tag) {
    switch (tag) {
      case GUMBO_TAG_AREA:
      case GUMBO_TAG_BASE:
      case GUMBO_TAG_BR:
      case GUMBO_TAG_COL:
      case GUMBO_TAG_EMBED:
      case GUMBO_TAG_HR:
      case GUMBO_TAG_IMG:
      case GUMBO_TAG_INPUT:
      case GUMBO_TAG_LINK:
      case GUMBO_TAG_META:
      case GUMBO_TAG_PARAM:
      case GUMBO_TAG_SOURCE:
      case GUMBO_TAG_TRACK:
      case GUMBO_TAG_WBR:
        return true;

      default:
        return false;
    }
  }

  void processGumboNode(GumboNode* node, QString& out, bool escape_text = true) {
    if (!node) {
      return;
    }

    switch (node->type) {
      case GUMBO_NODE_TEXT:
        out +=
          escape_text ? QString::fromUtf8(node->v.text.text).toHtmlEscaped() : QString::fromUtf8(node->v.text.text);
        break;

      case GUMBO_NODE_ELEMENT: {
        GumboElement& el = node->v.element;

        if (el.tag == GUMBO_TAG_IMG) {
          GumboAttribute* attr_src = gumbo_get_attribute(&el.attributes, "src");
          GumboAttribute* attr_alt = gumbo_get_attribute(&el.attributes, "alt");
          GumboAttribute* attr_title = gumbo_get_attribute(&el.attributes, "title");

          if (attr_src && attr_src->value) {
            QString src = QString::fromUtf8(attr_src->value);
            QString href = src.toHtmlEscaped();
            QString link_text;

            if (attr_alt && attr_alt->value && strlen(attr_alt->value) > 0) {
              link_text = QString::fromUtf8(attr_alt->value).toHtmlEscaped();
            }
            else if (attr_title && attr_title->value && strlen(attr_title->value) > 0) {
              link_text = QString::fromUtf8(attr_title->value).toHtmlEscaped();
            }
            else {
              QUrl url(src);

              if (url.isValid() && !url.fileName().trimmed().isEmpty()) {
                link_text = url.fileName().toHtmlEscaped();
              }
              else if (url.isValid() && !url.host().isEmpty()) {
                link_text = url.host().toHtmlEscaped();
              }
            }

            out += QSL("<br/><a href=\"%1\">📷 %2 - %3</a><br/>").arg(href, QObject::tr("image"), link_text);
          }

          return;
        }

        const char* tag_name = gumbo_normalized_tagname(el.tag);

        if (tag_name && *tag_name) {
          out += QSL("<");
          out += tag_name;

          GumboVector* attrs = &el.attributes;

          for (unsigned int i = 0; i < attrs->length; ++i) {
            auto* attr = static_cast<GumboAttribute*>(attrs->data[i]);
            out += QSL(" ");
            out += attr->name;
            out += QSL("=\"");
            out += QString::fromUtf8(attr->value).toHtmlEscaped();
            out += QSL("\"");
          }

          out += QSL(">");
        }

        GumboVector* children = &el.children;
        const bool is_raw_text_element = el.tag == GUMBO_TAG_SCRIPT || el.tag == GUMBO_TAG_STYLE;

        for (unsigned int i = 0; i < children->length; ++i) {
          processGumboNode(static_cast<GumboNode*>(children->data[i]), out, !is_raw_text_element);
        }

        if (tag_name && *tag_name && !isGumboVoidTag(el.tag)) {
          out += QSL("</");
          out += tag_name;
          out += QSL(">");
        }

        break;
      }

      default:
        break;
    }
  }
} // namespace

QString WebViewer::htmlToDisplay(const QString& html) const {
  return loadExternalResources() ? html : convertToHtmlWithoutImages(html);
}

QString WebViewer::convertToHtmlWithoutImages(const QString& html) const {
  if (!TextFactory::couldBeHtml(html)) {
    return html;
  }

  QByteArray utf8 = html.toUtf8();
  GumboOutput* output = gumbo_parse(utf8.constData());
  QString result;
  GumboNode* root = output->root;

  processGumboNode(root, result);

  gumbo_destroy_output(&kGumboDefaultOptions, output);

  return result;
}

QUrl WebViewer::urlForMessage(const Message& message, RootItem* root) const {
  QUrl url;

  if (!message.m_url.isEmpty()) {
    url = message.m_url;
  }
  else {
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
        url = deducted_url;
      }
    }
  }

  if (url.isValid() && url.hasFragment()) {
    url.setFragment(QString());
  }

  return url;
}

QString WebViewer::htmlForMessage(const Message& message, RootItem* root, Feed* feed) const {
  auto html_message = qApp->skins()->generateHtmlOfArticle(message, root, feed, this);
  return html_message;
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
    QByteArray out;
    auto res = NetworkFactory::performNetworkOperation(m_contextMenuData.m_imgLinkUrl.toString(),
                                                       5000,
                                                       {},
                                                       out,
                                                       QNetworkAccessManager::Operation::GetOperation);

    if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
      QImage img;

      if (img.loadFromData(out)) {
        QGuiApplication::clipboard()->setImage(img);
      }
      else {
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             GuiMessage(QObject::tr("Image not decoded"),
                                        QObject::tr("Failed to decode image '%1'.")
                                          .arg(m_contextMenuData.m_imgLinkUrl.toString())),
                             GuiMessageDestination(true, true));
      }
    }
    else {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           GuiMessage(QObject::tr("Image not downloaded"),
                                      QObject::tr("Failed to download image '%1' with error '%2'.")
                                        .arg(m_contextMenuData.m_imgLinkUrl.toString(),
                                             NetworkFactory::networkErrorText(res.m_networkError))),
                           GuiMessageDestination(true, true));
    }
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
    QByteArray out;
    auto res = NetworkFactory::performNetworkOperation(m_contextMenuData.m_imgLinkUrl.toString(),
                                                       5000,
                                                       {},
                                                       out,
                                                       QNetworkAccessManager::Operation::GetOperation);

    if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
      IOFactory::writeFile(filename, out);
    }
    else {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           GuiMessage(QObject::tr("Image not downloaded"),
                                      QObject::tr("Failed to download image '%1' with error '%2'.")
                                        .arg(m_contextMenuData.m_imgLinkUrl.toString(),
                                             NetworkFactory::networkErrorText(res.m_networkError))),
                           GuiMessageDestination(true, true));
    }
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

    qApp->web()->openUrlInExternalBrowser({resolved_url}, true, true);
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
