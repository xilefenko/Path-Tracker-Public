#include "editor_encountertemplate.h"
#include "ui_editor_encountertemplate.h"
#include <QMenu>

editor_EncounterTemplate::editor_EncounterTemplate(QWidget *parent)
    : QWidget(parent), ui(new Ui::editor_EncounterTemplate)
{
    ui->setupUi(this);
}

editor_EncounterTemplate::editor_EncounterTemplate(PathTrackerStruct* data, EncounterTemplate* encounter, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_EncounterTemplate)
    , m_data(data)
    , m_encounter(encounter)
{
    ui->setupUi(this);
    PopulatePawnCombo();
    RefreshPawnList();

    ui->PawnList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->PawnList, &QListWidget::customContextMenuRequested,
            this, &editor_EncounterTemplate::ShowPawnContextMenu);
}

editor_EncounterTemplate::~editor_EncounterTemplate()
{
    delete ui;
}

void editor_EncounterTemplate::PopulatePawnCombo()
{
    ui->PawnCombo->clear();
    for (PawnTemplate* tmpl : m_data->allPawnTemplates())
        ui->PawnCombo->addItem(tmpl->name, tmpl->id);
}

void editor_EncounterTemplate::RefreshPawnList()
{
    ui->PawnList->clear();
    if (!m_encounter) return;
    for (const QString& id : m_encounter->pawnTemplateIds) {
        PawnTemplate* tmpl = m_data->findPawnTemplate(id);
        QString label = tmpl ? tmpl->name : id;
        QListWidgetItem* item = new QListWidgetItem(label, ui->PawnList);
        item->setData(Qt::UserRole, id);
    }
}

void editor_EncounterTemplate::on_PawnAddButton_clicked()
{
    if (!m_encounter) return;
    QString id = ui->PawnCombo->currentData().toString();
    if (id.isEmpty()) return;
    m_encounter->addPawnTemplate(id);
    RefreshPawnList();
    emit Save();
}

void editor_EncounterTemplate::ShowPawnContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = ui->PawnList->itemAt(pos);
    if (!item) return;
    int row = ui->PawnList->row(item);
    QMenu menu(this);
    QAction* removeAction = menu.addAction("Remove Pawn");
    connect(removeAction, &QAction::triggered, [this, row, item]() {
        if (row >= 0 && row < m_encounter->pawnTemplateIds.size()) {
            m_encounter->pawnTemplateIds.removeAt(row);
            delete item;
            emit Save();
        }
    });
    menu.exec(ui->PawnList->mapToGlobal(pos));
}
