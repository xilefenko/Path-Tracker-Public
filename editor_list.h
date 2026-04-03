#ifndef EDITOR_LIST_H
#define EDITOR_LIST_H

#include <QWidget>
#include <QMenu>
#include <QListWidgetItem>
#include "Effect.h"
#include "Action.h"
#include "PathTrackerStruct.h"

namespace Ui { class editor_List; }

class editor_List : public QWidget
{
    Q_OBJECT

public:
    explicit editor_List(QWidget *parent = nullptr);
    editor_List(PathTrackerStruct* data, EffectTemplate* effect, int index, QWidget *parent = nullptr);
    ~editor_List();

private slots:
    void ShowActionContextMenu(const QPoint &pos);
    void ShowAttributeContextMenu(const QPoint &pos);
    void ShowAddEffectContextMenu(const QPoint &pos);

private:
    Ui::editor_List *ui;

    EffectTemplate*    m_effect = nullptr;
    Stage*             m_stage  = nullptr;
    PathTrackerStruct* m_data   = nullptr;

    void InitContextMenus();

    void ClearLists(QListWidget& list);
    void BuildLists();
    void AdjustListSize(QListWidget& list);

    void CreateActionWidget(Action* action);
    void CreateAttributeWidget(AttributeModifier* modifier);
    void CreateAddEffectWidget(ChildEffectRef* ref);

    void AddEffectToRound(QString effectID);
    void AddAttributeToRound(QString attributeKey);
    void AddActionToRound(QString typeName);

    bool WouldCauseCircularity(const QString& candidateID, const QString& targetID);

signals:
    void Save();
};

#endif
