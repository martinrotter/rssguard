// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/iconfactory.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QBuffer>
#include <QGridLayout>
#include <QPainter>
#include <QScrollArea>
#include <QToolButton>
#include <QWidgetAction>

IconFactory::IconFactory(QObject* parent) : QObject(parent) {}

IconFactory::~IconFactory() {
  qDebugNN << LOGSEC_GUI << "Destroying IconFactory instance.";
}

QIcon IconFactory::generateIcon(const QColor& color) {
  QPixmap pxm(64, 64);

  pxm.fill(Qt::GlobalColor::transparent);

  QPainter paint(&pxm);

  paint.setBrush(color);
  paint.setPen(Qt::GlobalColor::transparent);
  paint.drawEllipse(pxm.rect().marginsRemoved(QMargins(2, 2, 2, 2)));

  return pxm;
}

QUuid IconFactory::iconGuid(const QIcon& icon) {
  QPixmap pixmap = icon.pixmap({32, 32});
  QImage image = pixmap.toImage();

  image = image.convertToFormat(QImage::Format_ARGB32);

  const uchar* bits = image.constBits();
  int byte_count = image.sizeInBytes();

  QByteArray pixel_data(reinterpret_cast<const char*>(bits), byte_count);
  QByteArray hash = QCryptographicHash::hash(pixel_data, QCryptographicHash::Algorithm::Md5);

  QUuid guid = QUuid::fromRfc4122(hash);
  return guid;
}

QAction* IconFactory::iconSelectionMenu(QMenu* menu,
                                        const QList<QIcon>& icons,
                                        const std::function<void(QIcon)>& handler) {
  QWidgetAction* w_a = new QWidgetAction(menu);
  QScrollArea* scroll_area = new QScrollArea();

  scroll_area->setWidgetResizable(true);
  scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
  scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
  scroll_area->setFrameShape(QFrame::Shape::NoFrame);
  scroll_area->setFixedHeight(200); // Limit height to trigger scrolling.

  QWidget* container = new QWidget();
  QGridLayout* grid = new QGridLayout(container);

  grid->setSpacing(2);
  grid->setContentsMargins(4, 4, 4, 4);

  int col = 0, row = 0;

  for (const auto& icon : icons) {
    if (icon.isNull()) {
      continue;
    }

    QToolButton* btn = new QToolButton();

    btn->setIcon(icon);
    btn->setIconSize(QSize(28, 28));
    btn->setAutoRaise(true);
    btn->setFixedSize(32, 32);

    connect(btn, &QToolButton::clicked, w_a, [=]() {
      menu->close();
      handler(btn->icon());
    });

    grid->addWidget(btn, row, col);

    if (++col >= 4) { // 4 icons per row.
      col = 0;
      row++;
    }
  }

  scroll_area->setWidget(container);
  w_a->setDefaultWidget(scroll_area);

  return w_a;
}

QIcon IconFactory::fromByteArray(QByteArray array) {
  if (array.isEmpty()) {
    return {};
  }

  array = QByteArray::fromBase64(array);
  QIcon icon;
  QBuffer buffer(&array);

  buffer.open(QIODevice::OpenModeFlag::ReadOnly);
  QDataStream in(&buffer);

  in.setVersion(QDataStream::Version::Qt_5_15);
  in >> icon;
  buffer.close();
  return icon;
}

QByteArray IconFactory::toByteArray(const QIcon& icon) {
  if (icon.isNull()) {
    return {};
  }

  QByteArray array;
  QBuffer buffer(&array);

  buffer.open(QIODevice::OpenModeFlag::WriteOnly);
  QDataStream out(&buffer);

  out.setVersion(QDataStream::Version::Qt_5_15);
  out << icon;
  buffer.close();
  return array.toBase64();
}

QPixmap IconFactory::fromByteArray(const QByteArray& array, const QString& format) {
  QPixmap pixmap;
  pixmap.loadFromData(array, format.toLocal8Bit().constData());

  return pixmap;
}

QByteArray IconFactory::toByteArray(const QPixmap& pixmap, const QString& format) {
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  pixmap.save(&buffer, format.toLocal8Bit().constData());

  return buffer.data();
}

QIcon IconFactory::fromTheme(const QString& name, const QString& fallback) {
  QIcon original = QIcon::fromTheme(name);

  return (original.isNull() && !fallback.isEmpty()) ? QIcon::fromTheme(fallback) : original;
}

QPixmap IconFactory::miscPixmap(const QString& name) {
  return QPixmap(QString(APP_THEME_PATH) + QDir::separator() + "misc" + QDir::separator() + name + ".png");
}

QIcon IconFactory::miscIcon(const QString& name) {
  return QIcon(QString(APP_THEME_PATH) + QDir::separator() + "misc" + QDir::separator() + name + ".png");
}

void IconFactory::setupSearchPaths() {
  auto paths = QIcon::themeSearchPaths();

  paths << APP_THEME_PATH << qApp->userDataFolder() + QDir::separator() + APP_LOCAL_ICON_THEME_FOLDER
        << qApp->applicationDirPath() + QDir::separator() + APP_LOCAL_ICON_THEME_FOLDER;

  QIcon::setThemeSearchPaths(paths);
  qDebugNN << LOGSEC_GUI << "Available icon theme paths: " << paths;
}

void IconFactory::setCurrentIconTheme(const QString& theme_name) {
  qApp->settings()->setValue(GROUP(GUI), GUI::IconTheme, theme_name);
}

QString IconFactory::currentIconTheme() const {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::IconTheme)).toString();
}

void IconFactory::loadCurrentIconTheme() {
  const QStringList installed_themes = installedIconThemes();
  const QString theme_name_from_settings = qApp->settings()->value(GROUP(GUI), SETTING(GUI::IconTheme)).toString();

  if (QIcon::themeName() == theme_name_from_settings) {
    qDebugNN << LOGSEC_GUI << "Icon theme" << QUOTE_W_SPACE(theme_name_from_settings) << "already loaded.";
    return;
  }

  // Display list of installed themes.
  qDebugNN << LOGSEC_GUI << "Installed icon themes are: "
           << QStringList(installed_themes)
                .replaceInStrings(QRegularExpression(QSL("^|$")), QSL("\'"))
                .replaceInStrings(QRegularExpression(QSL("^\\'$")), QSL("\'\'"))
                .join(QSL(", "));

  if (installed_themes.contains(theme_name_from_settings)) {
    // Desired icon theme is installed and can be loaded.
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    if (theme_name_from_settings.isEmpty()) {
      qDebugNN << LOGSEC_GUI << "Loading default system icon theme.";
    }
    else {
      qDebugNN << LOGSEC_GUI << "Loading icon theme" << QUOTE_W_SPACE_DOT(theme_name_from_settings);
      QIcon::setThemeName(theme_name_from_settings);
    }
#else
    qDebugNN << LOGSEC_GUI << "Loading icon theme" << QUOTE_W_SPACE_DOT(theme_name_from_settings);
    QIcon::setThemeName(theme_name_from_settings);
#endif
  }
  else {
    // Desired icon theme is not currently available.
    // Activate "default" or "no" icon theme instead.
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    qWarningNN << "Icon theme" << QUOTE_W_SPACE(theme_name_from_settings)
               << "cannot be loaded because it is not installed. Activating \"no\" icon theme.";
#else
    qWarningNN << "Icon theme" << QUOTE_W_SPACE(theme_name_from_settings)
               << "cannot be loaded because it is not installed. Activating \"no\" icon theme.";
    QIcon::setThemeName(QSL(APP_NO_THEME));
#endif
  }
}

QStringList IconFactory::installedIconThemes() const {
  QStringList icon_theme_names = {QSL(APP_NO_THEME)};

  // Iterate all directories with icon themes.
  QStringList icon_themes_paths = QIcon::themeSearchPaths();
  QStringList filters_index;

  filters_index.append(QSL("index.theme"));
  icon_themes_paths.removeDuplicates();

  for (const QString& icon_path : icon_themes_paths) {
    const QDir icon_dir(icon_path);
    auto icon_paths =
      icon_dir.entryInfoList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::Readable |
                               QDir::Filter::CaseSensitive | QDir::Filter::NoSymLinks,
                             QDir::SortFlag::Time);

    // Iterate all icon themes in this directory.
    for (const QFileInfo& icon_theme_path : std::as_const(icon_paths)) {
      QDir icon_theme_dir = QDir(icon_theme_path.absoluteFilePath());

      if (icon_theme_dir.exists(filters_index.at(0))) {
        icon_theme_names << icon_theme_dir.dirName();
      }
    }
  }

  icon_theme_names.removeDuplicates();
  return icon_theme_names;
}
