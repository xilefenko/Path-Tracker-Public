#ifndef EDITOR_EFFECTHEADER_H
#define EDITOR_EFFECTHEADER_H

#include <QWidget>
#include "Effect.h"

namespace Ui {
class editor_EffectHeader;
}

class editor_EffectHeader : public QWidget
{
    Q_OBJECT

public:
    explicit editor_EffectHeader(EffectTemplate* effect, QWidget* parent = nullptr);
    ~editor_EffectHeader();

signals:
    void Save();

private slots:
    void on_DescriptionInput_textChanged();
    void on_OnsetInput_textEdited(const QString& text);
    void on_SetIconButton_clicked();

private:
    Ui::editor_EffectHeader* ui;
    EffectTemplate* m_effect = nullptr;

    void AdjustDescriptionHeight();
};

#endif // EDITOR_EFFECTHEADER_H
