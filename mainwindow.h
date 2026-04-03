#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "PathTrackerStruct.h"
#include "editor.h"
#include "DiceRoller.h"
#include <QUuid>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QTreeWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool EncounterOngoing = false;

    QString CurrentPawnID;
    bool FirstItem = false;

    void FullRefresh();
    void RefreshPawnList();
    void RefreshPawnIcons();
    void AddPawnListVisual(PawnInstance* Pawn, int initiative = 0);
    void UpdatePawnSelectionIndicators();

    void DeletePawn(QString PawnID);

    void ShowPawnContextMenu(const QPoint &pos);

private slots:
    void OnSystemColorSchemeChanged();
    void on_pushButton_4_clicked();
    void on_SettingsButton_clicked();

    void on_PawnAdd_clicked();

    void on_PawnList_itemClicked(QListWidgetItem *item);

    void on_PawnList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_NextTurnButton_clicked();

    void on_StartStopButton_clicked();

    void on_PawnList_itemDoubleClicked(QListWidgetItem *item);

    void on_PawnAdderPrev_clicked();
    void on_PawnAdderNext_clicked();
    void on_EncounterAddBtn_clicked();
    void on_PlayerAddBtn_clicked();
    void on_PawnTemplateAddBtn_clicked();

    void on_UndoButton_clicked();
    void on_RedoButton_clicked();
    void on_DiceHistoryButton_clicked();

private:
    Ui::MainWindow *ui;

    QList<QJsonObject> m_pastStates;
    QList<QJsonObject> m_futureStates;

    void SaveSnapshot();
    void RestoreFromSnapshot(const QJsonObject& snap);

    EncounterInstance* enc();

    void UpdateAttribute(const Attribute& Attribute);
    void UpdateEffect(const EffectTemplate& Effect);

    void ClearLayout(QLayout* layout);

    void AddPropertyWithValue(QTreeWidgetItem* Parent, QString Property, QString Value);

    void SaveToJson();
    void LoadFromJSon();

    void Refresh();

    void PopulatePawnAdderCombos();

    void UpdateStartStopButtonAppearance();

};
#endif // MAINWINDOW_H
