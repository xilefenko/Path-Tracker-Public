#ifndef PAWN_EDITOR_H
#define PAWN_EDITOR_H

#include <QDialog>
#include <QWidget>
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
    // Slot for the commit button
    void onCommitClicked();

private:
    Ui::pawn_editor *ui;
    PathTrackerStruct* DataPtr = nullptr;
    PawnInstance* PawnPtr = nullptr;

    // Helper to refresh the list/table
    void refreshAttributeTable();

    // Helper to get Attribute Name from ID (for display purposes)
    QString getAttributeNameFromID(const QString& id);
};

#endif // PAWN_EDITOR_H
