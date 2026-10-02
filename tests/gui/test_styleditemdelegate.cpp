// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/styleditemdelegate.h"

#include <QHeaderView>
#include <QProxyStyle>
#include <QStandardItemModel>
#include <QTest>
#include <QTextLayout>
#include <QTreeView>
#include <QtMath>

namespace {
  class TestTreeView : public QTreeView {
    public:
      QStyleOptionViewItem itemOption() const {
#if QT_VERSION_MAJOR >= 6
        QStyleOptionViewItem option;
        initViewItemOption(&option);
        return option;
#else
        return viewOptions();
#endif
      }
  };

  class WideMarginStyle : public QProxyStyle {
    public:
      WideMarginStyle() : QProxyStyle(QStringLiteral("Fusion")) {}

      int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override {
        return metric == PM_FocusFrameHMargin ? 10 : QProxyStyle::pixelMetric(metric, option, widget);
      }
  };

  int textHeight(const QString& text, const QFont& font, int width) {
    QTextLayout layout(text, font);
    QTextOption option;
    option.setWrapMode(QTextOption::WrapMode::WordWrap);
    layout.setTextOption(option);

    qreal height = 0;
    layout.beginLayout();
    for (QTextLine line = layout.createLine(); line.isValid(); line = layout.createLine()) {
      line.setLineWidth(width);
      line.setPosition(QPointF(0, height));
      height += line.height();
    }
    layout.endLayout();
    return qCeil(height);
  }
} // namespace

class TestStyledItemDelegate : public QObject {
    Q_OBJECT

  private slots:
    void wrapsAtPaintedWidth_data();
    void wrapsAtPaintedWidth();
    void preservesOtherSizingModes();
    void resizesWhenReadStateChanges();
};

void TestStyledItemDelegate::wrapsAtPaintedWidth_data() {
  QTest::addColumn<bool>("bold");
  QTest::addColumn<bool>("wide_margins");
  QTest::newRow("regular") << false << false;
  QTest::newRow("bold") << true << false;
  QTest::newRow("regular-wide-margins") << false << true;
  QTest::newRow("bold-wide-margins") << true << true;
}

void TestStyledItemDelegate::wrapsAtPaintedWidth() {
  QFETCH(bool, bold);
  QFETCH(bool, wide_margins);

  QStandardItemModel model(1, 2);
  const QModelIndex title = model.index(0, 1);
  const QString text = QStringLiteral("The best pumpkin recipes to make this autumn");
  QFont font(QStringLiteral("Segoe UI"), 12);
  font.setBold(bold);
  model.setData(title, text);
  model.setData(title, font, Qt::ItemDataRole::FontRole);

  TestTreeView view;
  QProxyStyle* style = wide_margins ? new WideMarginStyle() : new QProxyStyle(QStringLiteral("Fusion"));
  style->setParent(&view);
  view.setStyle(style);
  view.setModel(&model);
  view.setWordWrap(true);
  view.header()->setStretchLastSection(false);
  StyledItemDelegate delegate(0, 3, &view, 1);
  QStyleOptionViewItem option = view.itemOption();
  option.rect.setWidth(-1); // Match QTreeView's row-height request.
  const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, &view) + 1;

  for (int width = 90; width <= 250; ++width) {
    view.setColumnWidth(1, width);
    const int required_height = textHeight(text, font, view.columnWidth(1) - 2 * margin);
    const int actual_height = delegate.sizeHint(option, title).height();
    QVERIFY2(actual_height >= required_height + 6,
             qPrintable(QStringLiteral("Title cropped at width %1: needs %2 pixels, got %3")
                          .arg(width)
                          .arg(required_height + 6)
                          .arg(actual_height)));
  }
}

void TestStyledItemDelegate::preservesOtherSizingModes() {
  QStandardItemModel model(1, 2);
  model.setData(model.index(0, 0), QStringLiteral("Another column with a lengthy value"));
  model.setData(model.index(0, 1), QStringLiteral("A lengthy article title with several words"));

  TestTreeView view;
  view.setModel(&model);
  view.setWordWrap(true);
  view.header()->setStretchLastSection(false);
  view.setColumnWidth(1, 90);
  QStyleOptionViewItem option = view.itemOption();
  option.rect.setWidth(-1);
  QStyledItemDelegate standard;
  StyledItemDelegate wrapped(0, 0, &view, 1);
  StyledItemDelegate unconfigured(0, 0, &view);
  StyledItemDelegate fixed_height(24, 3, &view, 1);

  QCOMPARE(wrapped.sizeHint(option, model.index(0, 0)), standard.sizeHint(option, model.index(0, 0)));
  QCOMPARE(unconfigured.sizeHint(option, model.index(0, 1)), standard.sizeHint(option, model.index(0, 1)));
  QCOMPARE(fixed_height.sizeHint(option, model.index(0, 1)).height(), 30);

  option.features.setFlag(QStyleOptionViewItem::ViewItemFeature::WrapText, false);
  QCOMPARE(wrapped.sizeHint(option, model.index(0, 1)), standard.sizeHint(option, model.index(0, 1)));
}

void TestStyledItemDelegate::resizesWhenReadStateChanges() {
  QStandardItemModel model(1, 2);
  const QModelIndex title = model.index(0, 1);
  const QString text = QStringLiteral("The best pumpkin recipes to make this autumn");
  QFont regular(QStringLiteral("Segoe UI"), 12);
  QFont bold(regular);
  bold.setBold(true);
  model.setData(title, text);
  model.setData(title, regular, Qt::ItemDataRole::FontRole);

  TestTreeView view;
  auto* style = new QProxyStyle(QStringLiteral("Fusion"));
  style->setParent(&view);
  view.setStyle(style);
  view.setModel(&model);
  view.setRootIsDecorated(false);
  view.setColumnHidden(0, true);
  view.setUniformRowHeights(false);
  view.setWordWrap(true);
  view.header()->setStretchLastSection(false);
  view.setItemDelegate(new StyledItemDelegate(0, 0, &view, 1));
  view.resize(500, 300);
  const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, &view) + 1;
  int width = 90;
  for (; width <= 250; ++width) {
    if (textHeight(text, bold, width - 2 * margin) > textHeight(text, regular, width - 2 * margin)) {
      break;
    }
  }
  QVERIFY(width <= 250);
  view.setColumnWidth(1, width);
  view.show();
  QTRY_VERIFY(view.visualRect(title).height() >= textHeight(text, regular, width - 2 * margin));
  const int read_height = view.visualRect(title).height();

  model.setData(title, bold, Qt::ItemDataRole::FontRole);
  QTRY_VERIFY(view.visualRect(title).height() > read_height);
  QVERIFY(view.visualRect(title).height() >= textHeight(text, bold, width - 2 * margin));

  model.setData(title, regular, Qt::ItemDataRole::FontRole);
  QTRY_COMPARE(view.visualRect(title).height(), read_height);
}

QTEST_MAIN(TestStyledItemDelegate)

#include "test_styleditemdelegate.moc"
