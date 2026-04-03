#ifndef EDITOR_ADDEFFECT_H
#define EDITOR_ADDEFFECT_H

#include <QWidget>
#include "Effect.h"

namespace Ui {
class editor_addeffect;
}

class editor_addeffect : public QWidget
{
    Q_OBJECT

public:
    explicit editor_addeffect(QWidget *parent = nullptr);
    explicit editor_addeffect(QString name, ChildEffectRef* ref, QWidget *parent = nullptr);
    ~editor_addeffect();

private slots:
    void on_Level_valueChanged(int arg1);   // writes to ChildEffectRef::level
    void on_checkBox_checkStateChanged(const Qt::CheckState &arg1);

private:
    Ui::editor_addeffect *ui;
    ChildEffectRef* m_ref = nullptr;

signals:
    void Save();
};

#endif // EDITOR_ADDEFFECT_H
