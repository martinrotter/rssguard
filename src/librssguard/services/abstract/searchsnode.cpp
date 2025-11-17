// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/searchsnode.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/qtlinq.h"
#include "services/abstract/gui/formaddeditprobe.h"
#include "services/abstract/serviceroot.h"

SearchsNode::SearchsNode(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Probes);
  setId(ID_PROBES);
  setIcon(qApp->icons()->fromTheme(QSL("system-search")));
  setTitle(tr("Queries"));
  setDescription(tr("You can see all your permanent queries here."));
}

void SearchsNode::loadProbes(const QList<Search*>& probes) {
  for (auto* prb : probes) {
    appendChild(prb);
  }
}

Search* SearchsNode::probeById(const QString& custom_id) {
  auto chi = childItems();

  return qobject_cast<Search*>(qlinq::from(chi)
                                 .firstOrDefault([custom_id](RootItem* it) {
                                   return it->customId() == custom_id;
                                 })
                                 .value_or(nullptr));
}

QList<Search*> SearchsNode::probes() const {
  auto list = qlinq::from(childItems()).select([](RootItem* it) {
    return static_cast<Search*>(it);
  });

  return list.toList();
}

int SearchsNode::countOfUnreadMessages() const {
  return -1;
}

int SearchsNode::countOfAllMessages() const {
  return -1;
}

void SearchsNode::createProbe() {
  FormAddEditProbe frm(qApp->mainFormWidget());
  Search* new_prb = frm.execForAdd();

  if (new_prb != nullptr) {
    QSqlDatabase db = qApp->database()->driver()->connection(metaObject()->className());

    try {
      DatabaseQueries::createProbe(db, new_prb, account()->accountId());

      account()->requestItemReassignment(new_prb, this);
      account()->requestItemExpand({this}, true);
    }
    catch (const ApplicationException& ex) {
      new_prb->deleteLater();
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Not allowed"),
                            tr("Problem when creating probe: %1.").arg(ex.message()),
                            QSystemTrayIcon::MessageIcon::Critical});
    }
  }
}
