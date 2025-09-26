// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/labelsnode.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/globals.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/gui/formaddeditlabel.h"
#include "services/abstract/serviceroot.h"

LabelsNode::LabelsNode(RootItem* parent_item) : RootItem(parent_item), m_actLabelNew(nullptr) {
  setKind(RootItem::Kind::Labels);
  setId(ID_LABELS);
  setIcon(qApp->icons()->fromTheme(QSL("tag-folder"), QSL("emblem-favorite")));
  setTitle(tr("Labels"));
  setDescription(tr("You can see all your labels (tags) here."));
}

void LabelsNode::loadLabels(const QList<Label*>& labels) {
  for (Label* lbl : labels) {
    appendChild(lbl);
  }
}

QList<Message> LabelsNode::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedLabelledMessages(database, getParentServiceRoot()->accountId());
}

void LabelsNode::updateCounts(bool including_total_count) {
  // TODO: This is still rather slow because this is automatically
  // called when message is marked (un)read or starred.
  // It would be enough if only labels which are assigned to article
  // are recounted, not all.

  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  int account_id = getParentServiceRoot()->accountId();
  auto acc = DatabaseQueries::getMessageCountsForAllLabels(database, account_id);

  for (Label* lbl : labels()) {
    if (!acc.contains(lbl->customId())) {
      if (including_total_count) {
        lbl->setCountOfAllMessages(0);
      }

      lbl->setCountOfUnreadMessages(0);
    }
    else {
      auto ac = acc.value(lbl->customId());

      if (including_total_count) {
        lbl->setCountOfAllMessages(ac.m_total);
      }

      lbl->setCountOfUnreadMessages(ac.m_unread);
    }
  }
}

Label* LabelsNode::labelById(const QString& custom_id) {
  auto chi = childItems();

  return qobject_cast<Label*>(boolinq::from(chi).firstOrDefault([custom_id](RootItem* it) {
    return it->customId() == custom_id;
  }));
}

QList<Label*> LabelsNode::labels() const {
  auto list = boolinq::from(childItems())
                .select([](RootItem* it) {
                  return static_cast<Label*>(it);
                })
                .toStdList();

  return FROM_STD_LIST(QList<Label*>, list);
}

QList<QAction*> LabelsNode::contextMenuFeedsList() {
  if (m_actLabelNew == nullptr) {
    // Initialize it all.
    m_actLabelNew = new QAction(qApp->icons()->fromTheme(QSL("tag-new")), tr("New label"), this);

    connect(m_actLabelNew, &QAction::triggered, this, &LabelsNode::createLabel);
  }

  return QList<QAction*>{m_actLabelNew};
}

void LabelsNode::createLabel() {
  if (Globals::hasFlag(getParentServiceRoot()->supportedLabelOperations(), ServiceRoot::LabelOperation::Adding)) {
    FormAddEditLabel frm(qApp->mainFormWidget());
    Label* new_lbl = frm.execForAdd();

    if (new_lbl != nullptr) {
      QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

      try {
        DatabaseQueries::createLabel(db, new_lbl, getParentServiceRoot()->accountId());

        getParentServiceRoot()->requestItemReassignment(new_lbl, this);
        getParentServiceRoot()->requestItemExpand({this}, true);
      }
      catch (const ApplicationException& ex) {
        new_lbl->deleteLater();
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             {tr("Not allowed"),
                              tr("Cannot create label: %1.").arg(ex.message()),
                              QSystemTrayIcon::MessageIcon::Critical});
      }
    }
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Not allowed"),
                          tr("This account does not allow you to create labels."),
                          QSystemTrayIcon::MessageIcon::Critical});
  }
}
