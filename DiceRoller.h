#pragma once
#include <QString>
#include <QDialog>
#include <QAbstractTableModel>
#include <QList>

// ─────────────────────────────────────────────
//  DiceResult  (flat struct)
//  Single roll recorded in the session log.
// ─────────────────────────────────────────────
struct DiceResult
{
    QString formula;   // e.g. "2D6+5"
    int     result;
    QString timestamp;
};

// ─────────────────────────────────────────────
//  DiceHistoryModel
//  QAbstractTableModel backing the persistent session roll log.
//  Columns: Formula | Result | Time
// ─────────────────────────────────────────────
class DiceHistoryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit DiceHistoryModel(QObject* parent = nullptr);

    void appendResult(const DiceResult& r);
    void clear();

    // QAbstractTableModel overrides
    int      rowCount(const QModelIndex& parent = {}) const override;
    int      columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    static DiceHistoryModel& instance();   // session singleton

private:
    QList<DiceResult> m_results;
};

// ─────────────────────────────────────────────
//  DiceParser  (static utility)
//  Parses and evaluates dice formula strings.
//  Supports: NdM, NdM+K, NdM-K  (case-insensitive D)
// ─────────────────────────────────────────────
class DiceParser
{
public:
    // Returns the rolled total and appends to DiceHistoryModel::instance().
    static int roll(const QString& formula);

    // Pure parse – returns the formula components without rolling.
    struct Formula { int count; int sides; int modifier; bool valid; };
    static Formula parse(const QString& formula);
};

// ─────────────────────────────────────────────
//  DiceRollerPopup  (reusable QDialog)
//  Spawned by runtime action widgets.
//  On accept, appends result to DiceHistoryModel.
// ─────────────────────────────────────────────
class DiceRollerPopup : public QDialog
{
    Q_OBJECT
public:
    // formula may be empty – user can type one at runtime
    explicit DiceRollerPopup(const QString& formula = {},
                             QWidget* parent = nullptr);

    int lastResult() const { return m_lastResult; }

signals:
    void rolled(int result, const QString& formula);

private slots:
    void onRoll();

private:
    QString m_formula;
    int     m_lastResult = 0;
};
