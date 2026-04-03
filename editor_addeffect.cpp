#include "editor_addeffect.h"
#include "ui_editor_addeffect.h"

editor_addeffect::editor_addeffect(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_addeffect)
{
    ui->setupUi(this);
}

editor_addeffect::editor_addeffect(QString name, ChildEffectRef* ref, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_addeffect)
    , m_ref(ref)
{
    ui->setupUi(this);
    ui->EffectName->setText(name);
    ui->Level->setValue(m_ref->level);

    if (m_ref->binding == ChildBinding::Bound)
        ui->checkBox->setCheckState(Qt::CheckState::Checked);
}

editor_addeffect::~editor_addeffect()
{
    delete ui;
}

void editor_addeffect::on_Level_valueChanged(int arg1)
{
    if (m_ref) m_ref->level = arg1;
    emit Save();
}

void editor_addeffect::on_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (m_ref)
        m_ref->binding = (arg1 == Qt::Checked) ? ChildBinding::Bound : ChildBinding::Unbound;
    emit Save();
}
