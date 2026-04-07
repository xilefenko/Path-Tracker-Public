#include "DiceRoller.h"

#include <QRegularExpression>
#include <QRandomGenerator>
#include <QDateTime>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDebug>

// ═══════════════════════════════════════════════════
//  DiceHistoryModel
// ═══════════════════════════════════════════════════

DiceHistoryModel& DiceHistoryModel::instance()
{
    static DiceHistoryModel s_instance;
    return s_instance;
}

DiceHistoryModel::DiceHistoryModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

void DiceHistoryModel::appendResult(const DiceResult& r)
{
    const int row = m_results.size();
    beginInsertRows(QModelIndex(), row, row);
    m_results.append(r);
    endInsertRows();
}

void DiceHistoryModel::clear()
{
    beginResetModel();
    m_results.clear();
    endResetModel();
}

QJsonArray DiceHistoryModel::saveToJson() const
{
    QJsonArray arr;
    for (const DiceResult& r : m_results)
    {
        QJsonObject obj;
        obj["formula"]   = r.formula;
        obj["result"]    = r.result;
        obj["timestamp"] = r.timestamp;
        arr.append(obj);
    }
    return arr;
}

void DiceHistoryModel::loadFromJson(const QJsonArray& arr)
{
    beginResetModel();
    m_results.clear();
    for (const QJsonValue& v : arr)
    {
        const QJsonObject obj = v.toObject();
        DiceResult r;
        r.formula   = obj["formula"].toString();
        r.result    = obj["result"].toInt();
        r.timestamp = obj["timestamp"].toString();
        m_results.append(r);
    }
    endResetModel();
}

int DiceHistoryModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_results.size();
}

int DiceHistoryModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return 3;   // Formula | Result | Time
}

QVariant DiceHistoryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const DiceResult& r = m_results.at(index.row());
    switch (index.column())
    {
    case 0: return r.formula;
    case 1: return r.result;
    case 2: return r.timestamp;
    default: return {};
    }
}

QVariant DiceHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch (section)
    {
    case 0: return QStringLiteral("Formula");
    case 1: return QStringLiteral("Result");
    case 2: return QStringLiteral("Time");
    default: return {};
    }
}

// ═══════════════════════════════════════════════════
//  DiceParser
// ═══════════════════════════════════════════════════

DiceParser::Formula DiceParser::parse(const QString& formula)
{
    Formula f{ 0, 0, 0, false };

    // Accepts:  NdM  |  NdM+K  |  NdM-K   (case-insensitive 'd')
    static const QRegularExpression re(
        R"(^\s*(\d+)[dD](\d+)(?:\s*([+-])\s*(\d+))?\s*$)");

    const QRegularExpressionMatch m = re.match(formula.trimmed());
    if (!m.hasMatch())
    {
        qWarning() << "DiceParser::parse – unrecognised formula:" << formula;
        return f;
    }

    f.count    = m.captured(1).toInt();
    f.sides    = m.captured(2).toInt();
    f.modifier = 0;
    f.valid    = (f.count > 0 && f.sides > 0);

    if (!m.captured(3).isEmpty())
    {
        const int mod = m.captured(4).toInt();
        f.modifier    = (m.captured(3) == "-") ? -mod : mod;
    }

    return f;
}

int DiceParser::roll(const QString& formula)
{
    const Formula f = parse(formula);
    int total = f.modifier;

    if (f.valid)
    {
        for (int i = 0; i < f.count; ++i)
            total += static_cast<int>(QRandomGenerator::global()->bounded(f.sides)) + 1;
    }

    DiceResult result;
    result.formula   = formula;
    result.result    = total;
    result.timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    DiceHistoryModel::instance().appendResult(result);

    return total;
}

// ═══════════════════════════════════════════════════
//  DiceRollerPopup
// ═══════════════════════════════════════════════════

DiceRollerPopup::DiceRollerPopup(const QString& formula, QWidget* parent)
    : QDialog(parent)
    , m_formula(formula)
{
    setWindowTitle("Dice Roller");
    setModal(true);
    setMinimumWidth(300);

    auto* layout = new QVBoxLayout(this);

    // Formula row
    auto* formulaLayout = new QHBoxLayout;
    formulaLayout->addWidget(new QLabel("Formula:", this));
    auto* formulaEdit = new QLineEdit(formula, this);
    formulaEdit->setPlaceholderText("e.g. 2D6+3");
    formulaLayout->addWidget(formulaEdit);
    layout->addLayout(formulaLayout);

    // Result display
    auto* resultLabel = new QLabel("—", this);
    resultLabel->setAlignment(Qt::AlignCenter);
    QFont f = resultLabel->font();
    f.setPointSize(24);
    f.setBold(true);
    resultLabel->setFont(f);
    layout->addWidget(resultLabel);

    // Buttons
    auto* btnLayout = new QHBoxLayout;
    auto* rollBtn   = new QPushButton("Roll", this);
    auto* closeBtn  = new QPushButton("Accept", this);
    btnLayout->addWidget(rollBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    // Wire up: update formula from editor
    connect(formulaEdit, &QLineEdit::textChanged, this, [this](const QString& t){
        m_formula = t;
    });

    // Wire up: roll button
    connect(rollBtn, &QPushButton::clicked, this, [this, resultLabel](){
        m_lastResult = DiceParser::roll(m_formula);
        resultLabel->setText(QString::number(m_lastResult));
        emit rolled(m_lastResult, m_formula);
    });

    // Wire up: accept
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void DiceRollerPopup::onRoll()
{
    // Legacy slot – delegates to inline lambda above. Kept for moc compatibility.
    m_lastResult = DiceParser::roll(m_formula);
    emit rolled(m_lastResult, m_formula);
}
