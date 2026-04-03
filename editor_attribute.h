#ifndef EDITOR_ATTRIBUTE_H
#define EDITOR_ATTRIBUTE_H

#include <QWidget>
#include "Attribute.h"

namespace Ui {
class editor_attribute;
}

class editor_attribute : public QWidget
{
    Q_OBJECT

public:
    explicit editor_attribute(QWidget *parent = nullptr);
    explicit editor_attribute(QString label, AttributeModifier* spec, QWidget *parent = nullptr);
    ~editor_attribute();

private slots:
    void on_OperrandButton_clicked();
    void on_ValueSpinBox_valueChanged(int arg1);
    void on_lineEdit_textChanged(const QString &arg1);   // kept to satisfy Qt auto-connect; no-op

private:
    Ui::editor_attribute *ui;
    AttributeModifier* m_spec = nullptr;

signals:
    void Save();
};

#endif // EDITOR_ATTRIBUTE_H
