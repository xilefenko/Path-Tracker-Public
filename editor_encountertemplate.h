#ifndef EDITOR_ENCOUNTERTEMPLATE_H
#define EDITOR_ENCOUNTERTEMPLATE_H

#include <QWidget>
#include "Encounter.h"
#include "PathTrackerStruct.h"

namespace Ui {
class editor_EncounterTemplate;
}

class editor_EncounterTemplate : public QWidget
{
    Q_OBJECT

public:
    explicit editor_EncounterTemplate(QWidget *parent = nullptr);
    explicit editor_EncounterTemplate(PathTrackerStruct* data, EncounterTemplate* encounter, QWidget *parent = nullptr);
    ~editor_EncounterTemplate();

signals:
    void Save();

private slots:
    void on_PawnAddButton_clicked();
    void ShowPawnContextMenu(const QPoint& pos);

private:
    Ui::editor_EncounterTemplate *ui;
    PathTrackerStruct*  m_data      = nullptr;
    EncounterTemplate*  m_encounter = nullptr;

    void PopulatePawnCombo();
    void RefreshPawnList();
};

#endif // EDITOR_ENCOUNTERTEMPLATE_H
