#ifndef PAWN_EDITOR_H
#define PAWN_EDITOR_H

#include <QDialog>
#include <QWidget>
#include <QTableWidgetItem>
#include "PathTrackerStruct.h"

namespace Ui {
class pawn_editor;
}

class pawn_editor : public QDialog
{
    Q_OBJECT

public:
    explicit pawn_editor(QWidget *parent = nullptr);
    explicit pawn_editor(PathTrackerStruct* Data, PawnInstance* Pawn, QWidget *parent = nullptr);
    ~pawn_editor();

private slots:
    void onCommitClicked();
    void onAttributeItemChanged(QTableWidgetItem* item);

private:
    Ui::pawn_editor *ui;
    PathTrackerStruct* DataPtr = nullptr;
    PawnInstance* PawnPtr = nullptr;
    bool m_refreshing = false;

    void refreshAttributeTable();
    void clampHp();
    QString getAttributeNameFromID(const QString& id);
};

#endif // PAWN_EDITOR_H
