#pragma once
#include <functional>

#include <QAbstractListModel>
#include <QString>

namespace search {

/** One command in the palette's Actions section. */
struct ActionItem final {
  QString id;
  QString label;
  QString glyph;
  QString shortcut;
  std::function<void()> run;
};

/**
 * The fixed set of commands the palette offers alongside symbol matches.
 *
 * Each entry carries what it does, so the palette needs no switch over action ids and adding a
 * command is one entry rather than an edit in two files. Not registered with QML -- it reaches the
 * scene as a QAbstractItemModel* on SearchViewModel.
 */
class ActionModel final : public QAbstractListModel {
  Q_OBJECT

public:
  enum Role : int {
    IdRole = Qt::UserRole + 1,
    LabelRole,
    GlyphRole,
    ShortcutRole,
  };

  explicit ActionModel(QObject* parent = nullptr);
  ~ActionModel() override;

  void setActions(QList<ActionItem> actions);

  /** Runs the action at `row`; a row outside the model is ignored rather than asserted. */
  void run(int row) const;

  /** Rows whose label contains `query`, case-insensitively. Empty query means every row. */
  [[nodiscard]] QList<int> filter(const QString& query) const;

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
  QList<ActionItem> mActions;
};

}    // namespace search
