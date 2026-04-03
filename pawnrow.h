#ifndef PAWNROW_H
#define PAWNROW_H

#include <QWidget>
#include "Pawn.h"

namespace Ui {
class PawnRow;
}

class PawnRow : public QWidget
{
    Q_OBJECT

public:
    explicit PawnRow(QWidget *parent = nullptr);
    ~PawnRow();

    void SetData(PawnInstance* PawnData, int initiative = 0);
    void SetSelected(bool selected);
    void RefreshIcons();

private:
    Ui::PawnRow *ui;

    PawnInstance* m_pawnData   = nullptr;
    int           m_initiative = 0;

    void ApplyStylesheet(bool selected);
};

#endif // PAWNROW_H
