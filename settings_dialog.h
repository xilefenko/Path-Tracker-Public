#pragma once

#include <QDialog>
#include "PathTrackerStruct.h"

class QLabel;
class QPushButton;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

private slots:
    void onThemeModeChanged(int index);
    void onWindowBgClicked();
    void onTextColorClicked();
    void onAccentColorClicked();
    void onButtonBgClicked();
    void onButtonTextClicked();
    void onBorderRadiusChanged(int value);
    void onButtonPaddingChanged(int value);
    void onFontSizeChanged(int value);

    void onInputBgClicked();
    void onPawnWaitingClicked();
    void onPawnActiveClicked();
    void onPawnSpentClicked();
    void onPawnUnconsciousClicked();
    void onPawnDeadClicked();

    void on_ApplyButton_clicked();
    void on_ResetButton_clicked();
    void on_CloseButton_clicked();
    void on_AdvancedApplyButton_clicked();

signals:
    void ThemeApplied();

private:
    void applyColorToButton(QPushButton* btn, const QColor& color);
    void openColorPicker(QPushButton* btn, QColor& target, QLabel* hexLabel);
    void populateControlsFromTheme();

    Ui::SettingsDialog* ui;
    AppTheme m_working;
};
