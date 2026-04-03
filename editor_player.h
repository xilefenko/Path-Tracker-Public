#ifndef EDITOR_PLAYER_H
#define EDITOR_PLAYER_H

#include <QWidget>
#include <QListWidgetItem>
#include "Pawn.h"
#include "PathTrackerStruct.h"

namespace Ui {
class editor_Player;
}

class editor_Player : public QWidget
{
    Q_OBJECT

public:
    explicit editor_Player(QWidget *parent = nullptr);
    explicit editor_Player(Player* player, QWidget *parent = nullptr);
    ~editor_Player();

signals:
    void Save();

private slots:
    void on_AttrAddButton_clicked();
    void on_SetPortraitButton_clicked();
    void ShowAttrContextMenu(const QPoint& pos);
    void OnAttrItemDoubleClicked(QListWidgetItem* item);

private:
    Ui::editor_Player *ui;
    Player* m_player = nullptr;

    void PopulateAttrCombo();
    void RefreshAttrList();
};

#endif // EDITOR_PLAYER_H
