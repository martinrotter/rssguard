// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDICONSELECTION_H
#define FEEDICONSELECTION_H

#include <optional>

#include <QHash>
#include <QIcon>

// GUI-thread staging only. Keys are indices in the dialog's unchanged feed list.
class FeedIconSelection {
  public:
    bool isIndividual() const {
      return m_individual;
    }

    void selectShared() {
      m_individual = false;
      m_icons.clear();
    }

    void selectIndividual(const QHash<int, QIcon>& icons) {
      m_individual = true;
      m_icons = icons;
    }

    void invalidate() {
      m_icons.clear();
    }

    std::optional<QIcon> iconForFeed(int index, const QIcon& shared, bool enabled) const {
      if (!enabled) {
        return std::nullopt;
      }
      if (!m_individual) {
        return shared; // A null shared icon deliberately restores the default icon.
      }
      const auto icon = m_icons.constFind(index);
      return icon == m_icons.cend() ? std::nullopt : std::optional<QIcon>(icon.value());
    }

  private:
    bool m_individual = false;
    QHash<int, QIcon> m_icons;
};

#endif // FEEDICONSELECTION_H
