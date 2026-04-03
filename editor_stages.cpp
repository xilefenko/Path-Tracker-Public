#include "editor_stages.h"
#include "ui_editor_stages.h"

editor_Stages::editor_Stages(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_Stages)
{
    ui->setupUi(this);
}

editor_Stages::editor_Stages(PathTrackerStruct* data, EffectTemplate* effect, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_Stages)
    , m_effect(effect)
    , m_data(data)
{
    ui->setupUi(this);

    while (ui->toolBox->count() > 0)
        ui->toolBox->removeItem(0);

    for (int index = 0; index < m_effect->stages.size(); ++index) {
        auto* list = new editor_List(m_data, m_effect, index, ui->toolBox);
        connect(list, SIGNAL(Save()), this, SIGNAL(Save()));
        ui->toolBox->addItem(list, tr("Stage %1").arg(index + 1));
    }
}

editor_Stages::~editor_Stages()
{
    delete ui;
}

void editor_Stages::on_RemoveStageButton_clicked()
{
    int lastIndex = ui->toolBox->count() - 1;
    if (lastIndex < 0)
        return;

    m_effect->removeStage(lastIndex);

    QWidget* page = ui->toolBox->widget(lastIndex);
    ui->toolBox->removeItem(lastIndex);
    page->deleteLater();

    emit Save();
}

void editor_Stages::on_AddStageButton_clicked()
{
    m_effect->addStage();

    int index = ui->toolBox->count();

    auto* list = new editor_List(m_data, m_effect, index, ui->toolBox);
    connect(list, SIGNAL(Save()), this, SIGNAL(Save()));

    ui->toolBox->addItem(list, tr("Stage %1").arg(index + 1));
    ui->toolBox->setCurrentIndex(index);

    emit Save();
}
