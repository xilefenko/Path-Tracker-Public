#ifndef EDITOR_STAGES_H
#define EDITOR_STAGES_H

#include <QWidget>
#include "Effect.h"
#include "PathTrackerStruct.h"
#include "editor_list.h"

namespace Ui {
class editor_Stages;
}

class editor_Stages : public QWidget
{
    Q_OBJECT

public:
    explicit editor_Stages(QWidget *parent = nullptr);
    explicit editor_Stages(PathTrackerStruct* data, EffectTemplate* effect, QWidget *parent = nullptr);
    ~editor_Stages();

private slots:
    void on_RemoveStageButton_clicked();
    void on_AddStageButton_clicked();

private:
    Ui::editor_Stages *ui;
    EffectTemplate*    m_effect = nullptr;
    PathTrackerStruct* m_data   = nullptr;

signals:
    void Save();
};

#endif // EDITOR_STAGES_H
