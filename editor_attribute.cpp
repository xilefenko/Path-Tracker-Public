#include "editor_attribute.h"
#include "ui_editor_attribute.h"

editor_attribute::editor_attribute(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_attribute)
{
    ui->setupUi(this);
}

editor_attribute::editor_attribute(QString label, AttributeModifier* spec, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_attribute)
    , m_spec(spec)
{
    ui->setupUi(this);
    ui->AttributeLabel->setText(label);
    ui->ValueSpinBox->setValue(m_spec->value);
    ui->lineEdit->setText(m_spec->description);

    switch (m_spec->op) {
    case ModifierOp::Minus:
        ui->OperrandButton->setText("-");
        break;
    case ModifierOp::Plus:
        ui->OperrandButton->setText("+");
        break;
    case ModifierOp::Multiply:
        ui->OperrandButton->setText("x");
        break;
    }
}

editor_attribute::~editor_attribute()
{
    delete ui;
}

void editor_attribute::on_OperrandButton_clicked()
{
    switch (m_spec->op) {
    case ModifierOp::Plus:
        ui->OperrandButton->setText("-");
        m_spec->op = ModifierOp::Minus;
        break;
    case ModifierOp::Minus:
        ui->OperrandButton->setText("x");
        m_spec->op = ModifierOp::Multiply;
        break;
    case ModifierOp::Multiply:
        ui->OperrandButton->setText("+");
        m_spec->op = ModifierOp::Plus;
        break;
    }

    emit Save();
}

void editor_attribute::on_ValueSpinBox_valueChanged(int arg1)
{
    if (m_spec)
        m_spec->value = arg1;
    emit Save();
}

void editor_attribute::on_lineEdit_textChanged(const QString& arg1)
{
    if (m_spec)
        m_spec->description = arg1;
    emit Save();
}
