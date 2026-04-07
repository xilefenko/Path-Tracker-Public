#include "pawn_editor.h"
#include "ui_pawn_editor.h"
#include <QMessageBox>
#include <QPushButton>

// Default Constructor
pawn_editor::pawn_editor(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::pawn_editor)
{
    ui->setupUi(this);
}

// Main Constructor
pawn_editor::pawn_editor(PathTrackerStruct* Data, PawnInstance* Pawn, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::pawn_editor)
    , DataPtr(Data)
    , PawnPtr(Pawn)
{
    ui->setupUi(this);

    if (!DataPtr || !PawnPtr) return;

    // --- Fill the Combobox with all registered attributes ---
    ui->attributeComboBox->clear();
    for (const Attribute& attr : PathTrackerStruct::attributes().allAttributes()) {
        ui->attributeComboBox->addItem(attr.displayName, attr.key);
    }

    // --- Setup Table Headers ---
    ui->attributesTable->setColumnCount(3);
    ui->attributesTable->setHorizontalHeaderLabels({"Attribute", "Base Value", ""});
    ui->attributesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->attributesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->attributesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->attributesTable->setColumnWidth(2, 30);

    // --- Connect Signals ---
    connect(ui->commitButton, &QPushButton::clicked, this, &pawn_editor::onCommitClicked);
    connect(ui->attributesTable, &QTableWidget::itemChanged, this, &pawn_editor::onAttributeItemChanged);

    // --- Initial Table Load ---
    refreshAttributeTable();
}

pawn_editor::~pawn_editor()
{
    delete ui;
}

// --- Set base value for the selected attribute ---
void pawn_editor::onCommitClicked()
{
    if (!PawnPtr) return;

    QString selectedKey = ui->attributeComboBox->currentData().toString();
    int     value       = ui->valueSpinBox->value();

    if (selectedKey.isEmpty()) return;

    // Update baseValue directly; resolveAttributes() recalculates currentValue.
    bool found = false;
    for (AttributeValue& av : PawnPtr->attributes) {
        if (av.key == selectedKey) {
            av.baseValue = value;
            found = true;
            break;
        }
    }
    if (!found) {
        AttributeValue av;
        av.key          = selectedKey;
        av.baseValue    = value;
        av.currentValue = value;
        PawnPtr->attributes.append(av);
    }
    PawnPtr->resolveAttributes();
    clampHp();

    refreshAttributeTable();
    ui->valueSpinBox->setValue(0);
}

// --- Rebuild the attribute table from the pawn's live data ---
void pawn_editor::refreshAttributeTable()
{
    if (!PawnPtr) return;

    m_refreshing = true;
    ui->attributesTable->setRowCount(0);

    for (const AttributeValue& attrVal : PawnPtr->attributes) {
        int row = ui->attributesTable->rowCount();
        ui->attributesTable->insertRow(row);

        QString displayName = getAttributeNameFromID(attrVal.key);
        QTableWidgetItem* nameItem = new QTableWidgetItem(displayName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setData(Qt::UserRole, attrVal.key);
        ui->attributesTable->setItem(row, 0, nameItem);

        QTableWidgetItem* valItem = new QTableWidgetItem(QString::number(attrVal.baseValue));
        ui->attributesTable->setItem(row, 1, valItem);

        auto* deleteButton = new QPushButton("X");
        deleteButton->setFixedWidth(30);
        ui->attributesTable->setCellWidget(row, 2, deleteButton);
        QString keyToDelete = attrVal.key;
        connect(deleteButton, &QPushButton::clicked, this, [this, keyToDelete]() {
            for (int i = 0; i < PawnPtr->attributes.size(); ++i) {
                if (PawnPtr->attributes[i].key == keyToDelete) {
                    PawnPtr->attributes.removeAt(i);
                    break;
                }
            }
            PawnPtr->resolveAttributes();
            clampHp();
            refreshAttributeTable();
        });
    }
    m_refreshing = false;
}

// --- Clamp HP currentValue to its baseValue (max HP) ---
void pawn_editor::clampHp()
{
    if (!PawnPtr) return;
    for (AttributeValue& av : PawnPtr->attributes) {
        if (av.key == HP_KEY && av.currentValue > av.baseValue)
            av.currentValue = av.baseValue;
    }
}

// --- Persist inline edits from the Base Value column ---
void pawn_editor::onAttributeItemChanged(QTableWidgetItem* item)
{
    if (m_refreshing || !PawnPtr) return;
    if (item->column() != 1) return;

    QTableWidgetItem* nameItem = ui->attributesTable->item(item->row(), 0);
    if (!nameItem) return;

    QString key = nameItem->data(Qt::UserRole).toString();
    bool ok = false;
    int value = item->text().toInt(&ok);
    if (!ok) return;

    for (AttributeValue& av : PawnPtr->attributes) {
        if (av.key == key) {
            av.baseValue = value;
            break;
        }
    }
    PawnPtr->resolveAttributes();
    clampHp();
}

// --- Resolve display name from attribute key ---
QString pawn_editor::getAttributeNameFromID(const QString& id)
{
    if (PathTrackerStruct::attributes().hasAttribute(id))
        return PathTrackerStruct::attributes().getAttribute(id).displayName;
    return id;
}
