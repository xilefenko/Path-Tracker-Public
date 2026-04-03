#ifndef EDITOR_H
#define EDITOR_H

#include <QDialog>
#include "PathTrackerStruct.h"
#include "QListWidgetItem"

namespace Ui {
class Editor;
}

class Editor : public QDialog
{
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr);
    explicit Editor(PathTrackerStruct* Data, QWidget *parent = nullptr);
    ~Editor();

    bool rename = false;
    void RequestRefresh();
    void RefreshDetailPanel();

    void ShowActionContextMenu(const QPoint &pos);
    void ShowEffectContextMenu(const QPoint &pos);

    void ShowPawnContextMenu(const QPoint &pos);
    void ShowEncounterContextMenu(const QPoint &pos);
    void ShowPlayerContextMenu(const QPoint &pos);

public slots:
    void SaveSlot(){
        DataPtr->save();
    };

private slots:
    void on_ComittAttribute_clicked();

    void on_AttributeList_itemClicked(QListWidgetItem *item);

    void on_EffectButton_clicked();

    void on_EffectLine_textChanged(const QString &arg1);

    void on_EffectList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_tabWidget_currentChanged(int index);

    void on_PawnButton_clicked();
    void on_PawnList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_PawnLine_textChanged(const QString &arg1);

    void on_EncounterButton_clicked();
    void on_EncounterList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_EncounterLine_textChanged(const QString &arg1);

    void on_PlayerButton_clicked();
    void on_PlayerList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_PlayerLine_textChanged(const QString &arg1);

private:
    Ui::Editor *ui;

    QString CurrentEditID;
    QString CurrentEffectID;
    QString CurrentPawnID;
    QString CurrentEncounterID;
    QString CurrentPlayerID;

    PathTrackerStruct* DataPtr;

    template <typename T, typename... Args>
    T* AddToDetailPanel(Args&&... args);

    template <typename T, typename... Args>
    T* AddToPanel(QWidget* body, Args&&... args);

    void ClearDetailsPanel();
    void ClearPanel(QWidget* body);

    void RefreshPawnPanel();
    void RefreshEncounterPanel();
    void RefreshPlayerPanel();
};

#endif // EDITOR_H
