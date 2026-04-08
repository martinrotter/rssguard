// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppcategory.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

XmppCategory::XmppCategory(Type type, RootItem* parent) : Category(parent), m_type(type) {
  updateTitleAndIcon();
}

XmppCategory::XmppCategory(const XmppCategory& other) : Category(other) {
  setType(other.type());
}

QVariantHash XmppCategory::customDatabaseData() const {
  QVariantHash data;

  data[QSL("type")] = int(type());

  return data;
}

void XmppCategory::setCustomDatabaseData(const QVariantHash& data) {
  int type = data[QSL("type")].toInt();

  setType(Type(type));
}

XmppCategory::Type XmppCategory::type() const {
  return m_type;
}

void XmppCategory::setType(Type type) {
  m_type = type;
}

QString XmppCategory::typeToString(Type type) {
  switch (type) {
    case SingleUserChats:
      return tr("Single-user Chats");

    case MultiUserChats:
      return tr("Multi-user Chats");

    case PubSubServices:
      return tr("PubSub Services");

    case PubSubPeps:
      return tr("PubSub PEPs");

    default:
      return tr("Unknown");
  }
}

void XmppCategory::updateTitleAndIcon() {
  setTitle(typeToString(m_type));

  switch (m_type) {
    case SingleUserChats:
      setIcon(qApp->icons()->fromTheme(QSL("user"), QSL("user-online")));
      break;

    case MultiUserChats:
      setIcon(qApp->icons()->fromTheme(QSL("user-group-new"), QSL("user")));
      break;

    case PubSubServices:
      setIcon(qApp->icons()->fromTheme(QSL("news-subscribe"), QSL("message-news")));
      break;

    case PubSubPeps:
      setIcon(qApp->icons()->fromTheme(QSL("news-subscribe"), QSL("message-news")));
      break;
  }
}
