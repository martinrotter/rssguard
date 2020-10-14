// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/label.h"

#include "gui/dialogs/formaddeditlabel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/databasequeries.h"
#include "services/abstract/serviceroot.h"

#include <QPainter>
#include <QPainterPath>

Label::Label(const QString& name, const QColor& color, RootItem* parent_item) : Label(parent_item) {
  setColor(color);
  setTitle(name);
}

Label::Label(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Label);
}

QColor Label::color() const {
  return m_color;
}

void Label::setColor(const QColor& color) {
  setIcon(generateIcon(color));
  m_color = color;
}

int Label::countOfUnreadMessages() const {
  return m_unreadCount;
}

int Label::countOfAllMessages() const {
  return m_totalCount;
}

bool Label::canBeEdited() const {
  return true;
}

bool Label::editViaGui() {
  FormAddEditLabel form(qApp->mainFormWidget());

  if (form.execForEdit(this)) {
    QSqlDatabase db = qApp->database()->connection(metaObject()->className());

    return DatabaseQueries::updateLabel(db, this);
  }
  else {
    return false;
  }
}

bool Label::canBeDeleted() const {
  return true;
}

bool Label::deleteViaGui() {
  QSqlDatabase db = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::deleteLabel(db, this)) {
    getParentServiceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

void Label::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  int account_id = getParentServiceRoot()->accountId();

  if (including_total_count) {
    setCountOfAllMessages(DatabaseQueries::getMessageCountsForLabel(database, this, account_id, true));
  }

  setCountOfUnreadMessages(DatabaseQueries::getMessageCountsForLabel(database, this, account_id, false));
}

QIcon Label::generateIcon(const QColor& color) {
  QPixmap pxm(64, 64);

  pxm.fill(Qt::GlobalColor::transparent);

  QPainter paint(&pxm);

  paint.setBrush(color);
  paint.setPen(Qt::GlobalColor::transparent);
  paint.drawEllipse(pxm.rect().marginsRemoved(QMargins(2, 2, 2, 2)));

  return pxm;
}

void Label::setCountOfAllMessages(int totalCount) {
  m_totalCount = totalCount;
}

void Label::setCountOfUnreadMessages(int unreadCount) {
  m_unreadCount = unreadCount;
}
