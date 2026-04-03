#ifndef EDITOR_PAWNTEMPLATE_H
#define EDITOR_PAWNTEMPLATE_H

#include <QWidget>
#include <QListWidgetItem>
#include "Pawn.h"
#include "PathTrackerStruct.h"

namespace Ui {
class editor_PawnTemplate;
}

class editor_PawnTemplate : public QWidget
{
    Q_OBJECT

public:
    explicit editor_PawnTemplate(QWidget *parent = nullptr);
    explicit editor_PawnTemplate(PathTrackerStruct* data, PawnTemplate* pawn, QWidget *parent = nullptr);
    ~editor_PawnTemplate();

signals:
    void Save();

private slots:
    void on_AttrAddButton_clicked();
    void on_EffectAddButton_clicked();
    void on_SetPortraitButton_clicked();
    void ShowAttrContextMenu(const QPoint& pos);
    void ShowEffectContextMenu(const QPoint& pos);
    void OnAttrItemDoubleClicked(QListWidgetItem* item);

private:
    Ui::editor_PawnTemplate *ui;
    PathTrackerStruct* m_data = nullptr;
    PawnTemplate*      m_pawn = nullptr;

    void PopulateAttrCombo();
    void PopulateEffectCombo();
    void RefreshAttrList();
    void RefreshEffectList();
};

#endif // EDITOR_PAWNTEMPLATE_H
