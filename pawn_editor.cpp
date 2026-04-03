#include "pawn_editor.h"
#include "ui_pawn_editor.h"
#include <QMessageBox>

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
    ui->attributesTable->setColumnCount(2);
    ui->attributesTable->setHorizontalHeaderLabels({"Attribute", "Base Value"});
    ui->attributesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // --- Connect Signals ---
    connect(ui->commitButton, &QPushButton::clicked, this, &pawn_editor::onCommitClicked);

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

    refreshAttributeTable();
    ui->valueSpinBox->setValue(0);
}

// --- Rebuild the attribute table from the pawn's live data ---
void pawn_editor::refreshAttributeTable()
{
    if (!PawnPtr) return;

    ui->attributesTable->setRowCount(0);

    for (const AttributeValue& attrVal : PawnPtr->attributes) {
        int row = ui->attributesTable->rowCount();
        ui->attributesTable->insertRow(row);

        QString displayName = getAttributeNameFromID(attrVal.key);
        QTableWidgetItem* nameItem = new QTableWidgetItem(displayName);
        nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);
        ui->attributesTable->setItem(row, 0, nameItem);

        QTableWidgetItem* valItem = new QTableWidgetItem(QString::number(attrVal.baseValue));
        ui->attributesTable->setItem(row, 1, valItem);
    }
}

// --- Resolve display name from attribute key ---
QString pawn_editor::getAttributeNameFromID(const QString& id)
{
    if (PathTrackerStruct::attributes().hasAttribute(id))
        return PathTrackerStruct::attributes().getAttribute(id).displayName;
    return id;
}
