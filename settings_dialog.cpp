#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include "PathTrackerStruct.h"

#include <QApplication>
#include <QColorDialog>
#include <QGuiApplication>
#include <QStyleHints>
#include <QLabel>
#include <QPushButton>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // Copy current theme into working copy
    m_working = PathTrackerStruct::instance().theme();

    // Populate all controls from the working copy
    populateControlsFromTheme();

    // Theme mode combo
    connect(ui->ThemeModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onThemeModeChanged);

    // Connect color buttons (manual — names don't follow on_X_signal convention)
    connect(ui->WindowBgButton,    &QPushButton::clicked, this, &SettingsDialog::onWindowBgClicked);
    connect(ui->TextColorButton,   &QPushButton::clicked, this, &SettingsDialog::onTextColorClicked);
    connect(ui->AccentColorButton, &QPushButton::clicked, this, &SettingsDialog::onAccentColorClicked);
    connect(ui->ButtonBgButton,    &QPushButton::clicked, this, &SettingsDialog::onButtonBgClicked);
    connect(ui->ButtonTextButton,  &QPushButton::clicked, this, &SettingsDialog::onButtonTextClicked);

    // Connect sliders
    connect(ui->BorderRadiusSlider,  &QSlider::valueChanged, this, &SettingsDialog::onBorderRadiusChanged);
    connect(ui->ButtonPaddingSlider, &QSlider::valueChanged, this, &SettingsDialog::onButtonPaddingChanged);
    connect(ui->FontSizeSlider,      &QSlider::valueChanged, this, &SettingsDialog::onFontSizeChanged);

    // Connect input field and pawn state color buttons
    connect(ui->InputBgButton,        &QPushButton::clicked, this, &SettingsDialog::onInputBgClicked);
    connect(ui->PawnWaitingButton,    &QPushButton::clicked, this, &SettingsDialog::onPawnWaitingClicked);
    connect(ui->PawnActiveButton,     &QPushButton::clicked, this, &SettingsDialog::onPawnActiveClicked);
    connect(ui->PawnSpentButton,      &QPushButton::clicked, this, &SettingsDialog::onPawnSpentClicked);
    connect(ui->PawnUnconsciousButton,&QPushButton::clicked, this, &SettingsDialog::onPawnUnconsciousClicked);
    connect(ui->PawnDeadButton,       &QPushButton::clicked, this, &SettingsDialog::onPawnDeadClicked);

    // Pre-populate advanced tab with the current stylesheet
    ui->StylesheetEdit->setPlainText(PathTrackerStruct::instance().stylesheet());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

void SettingsDialog::applyColorToButton(QPushButton* btn, const QColor& color)
{
    btn->setStyleSheet(
        QString("background-color: %1; border: 1px solid #555555;").arg(color.name()));
}

void SettingsDialog::openColorPicker(QPushButton* btn, QColor& target, QLabel* hexLabel)
{
    // DontUseNativeDialog: the native Windows color dialog spawns behind its
    // parent QDialog, making it invisible to the user. Qt's own dialog is used
    // on all platforms for consistent, always-on-top behaviour.
    const QColor chosen = QColorDialog::getColor(
        target, this, QString(), QColorDialog::DontUseNativeDialog);
    if (!chosen.isValid())
        return;
    target = chosen;
    applyColorToButton(btn, chosen);
    hexLabel->setText(chosen.name());
}

void SettingsDialog::populateControlsFromTheme()
{
    // Sync combo without re-triggering onThemeModeChanged
    {
        QSignalBlocker blocker(ui->ThemeModeCombo);
        ui->ThemeModeCombo->setCurrentIndex(static_cast<int>(m_working.themeMode));
    }

    // Disable individual colour pickers in System mode (colours are OS-determined)
    const bool editable = (m_working.themeMode != ThemeMode::System);
    ui->ColorsGroup->setEnabled(editable);
    ui->ButtonsGroup->setEnabled(editable);
    ui->InputFieldsGroup->setEnabled(editable);
    ui->PawnStatesGroup->setEnabled(editable);

    applyColorToButton(ui->WindowBgButton,    m_working.windowBackground);
    applyColorToButton(ui->TextColorButton,   m_working.textColor);
    applyColorToButton(ui->AccentColorButton, m_working.accentColor);
    applyColorToButton(ui->ButtonBgButton,    m_working.buttonBackground);
    applyColorToButton(ui->ButtonTextButton,  m_working.buttonTextColor);

    ui->WindowBgHex->setText(m_working.windowBackground.name());
    ui->TextColorHex->setText(m_working.textColor.name());
    ui->AccentColorHex->setText(m_working.accentColor.name());
    ui->ButtonBgHex->setText(m_working.buttonBackground.name());
    ui->ButtonTextHex->setText(m_working.buttonTextColor.name());

    ui->BorderRadiusSlider->setValue(m_working.buttonBorderRadius);
    ui->BorderRadiusValue->setText(QString("%1 px").arg(m_working.buttonBorderRadius));

    ui->ButtonPaddingSlider->setValue(m_working.buttonPadding);
    ui->ButtonPaddingValue->setText(QString("%1 px").arg(m_working.buttonPadding));

    ui->FontSizeSlider->setValue(m_working.fontSize);
    ui->FontSizeValue->setText(QString("%1 pt").arg(m_working.fontSize));

    applyColorToButton(ui->InputBgButton, m_working.inputBackground);
    ui->InputBgHex->setText(m_working.inputBackground.name());

    applyColorToButton(ui->PawnWaitingButton,     m_working.pawnWaitingColor);
    ui->PawnWaitingHex->setText(m_working.pawnWaitingColor.name());

    applyColorToButton(ui->PawnActiveButton,      m_working.pawnActiveColor);
    ui->PawnActiveHex->setText(m_working.pawnActiveColor.name());

    applyColorToButton(ui->PawnSpentButton,       m_working.pawnSpentColor);
    ui->PawnSpentHex->setText(m_working.pawnSpentColor.name());

    applyColorToButton(ui->PawnUnconsciousButton, m_working.pawnUnconsciousColor);
    ui->PawnUnconsciousHex->setText(m_working.pawnUnconsciousColor.name());

    applyColorToButton(ui->PawnDeadButton,        m_working.pawnDeadColor);
    ui->PawnDeadHex->setText(m_working.pawnDeadColor.name());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Theme mode slot
// ─────────────────────────────────────────────────────────────────────────────

void SettingsDialog::onThemeModeChanged(int index)
{
    const ThemeMode mode = static_cast<ThemeMode>(index);
    if (mode == ThemeMode::Dark)
    {
        m_working = AppTheme::darkPreset();
    }
    else if (mode == ThemeMode::Light)
    {
        m_working = AppTheme::lightPreset();
    }
    else // System
    {
        // Keep current colours but switch the mode flag;
        // only the mode matters for System — colours are ignored at runtime
        m_working.themeMode = ThemeMode::System;
    }
    populateControlsFromTheme();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Color slots
// ─────────────────────────────────────────────────────────────────────────────

void SettingsDialog::onWindowBgClicked()
{
    openColorPicker(ui->WindowBgButton, m_working.windowBackground, ui->WindowBgHex);
}

void SettingsDialog::onTextColorClicked()
{
    openColorPicker(ui->TextColorButton, m_working.textColor, ui->TextColorHex);
}

void SettingsDialog::onAccentColorClicked()
{
    openColorPicker(ui->AccentColorButton, m_working.accentColor, ui->AccentColorHex);
}

void SettingsDialog::onButtonBgClicked()
{
    openColorPicker(ui->ButtonBgButton, m_working.buttonBackground, ui->ButtonBgHex);
}

void SettingsDialog::onButtonTextClicked()
{
    openColorPicker(ui->ButtonTextButton, m_working.buttonTextColor, ui->ButtonTextHex);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slider slots
// ─────────────────────────────────────────────────────────────────────────────

void SettingsDialog::onBorderRadiusChanged(int value)
{
    m_working.buttonBorderRadius = value;
    ui->BorderRadiusValue->setText(QString("%1 px").arg(value));
}

void SettingsDialog::onButtonPaddingChanged(int value)
{
    m_working.buttonPadding = value;
    ui->ButtonPaddingValue->setText(QString("%1 px").arg(value));
}

void SettingsDialog::onFontSizeChanged(int value)
{
    m_working.fontSize = value;
    ui->FontSizeValue->setText(QString("%1 pt").arg(value));
}

void SettingsDialog::onInputBgClicked()
{
    openColorPicker(ui->InputBgButton, m_working.inputBackground, ui->InputBgHex);
}

void SettingsDialog::onPawnWaitingClicked()
{
    openColorPicker(ui->PawnWaitingButton, m_working.pawnWaitingColor, ui->PawnWaitingHex);
}

void SettingsDialog::onPawnActiveClicked()
{
    openColorPicker(ui->PawnActiveButton, m_working.pawnActiveColor, ui->PawnActiveHex);
}

void SettingsDialog::onPawnSpentClicked()
{
    openColorPicker(ui->PawnSpentButton, m_working.pawnSpentColor, ui->PawnSpentHex);
}

void SettingsDialog::onPawnUnconsciousClicked()
{
    openColorPicker(ui->PawnUnconsciousButton, m_working.pawnUnconsciousColor, ui->PawnUnconsciousHex);
}

void SettingsDialog::onPawnDeadClicked()
{
    openColorPicker(ui->PawnDeadButton, m_working.pawnDeadColor, ui->PawnDeadHex);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Button slots
// ─────────────────────────────────────────────────────────────────────────────

void SettingsDialog::on_ApplyButton_clicked()
{
    // For System mode, derive the effective stylesheet from the current OS palette
    AppTheme effective = m_working;
    if (m_working.themeMode == ThemeMode::System)
    {
        const bool isDark = (QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light);
        effective = isDark ? AppTheme::darkPreset() : AppTheme::lightPreset();
        effective.themeMode = ThemeMode::System;
    }

    const QString qss = effective.toStylesheet();
    qApp->setStyleSheet(qss);

    // setStyleSheet font-size on QWidget doesn't reliably cascade to existing
    // widgets on Windows. Driving the font through QApplication::setFont()
    // guarantees all widgets pick up the new size on every platform.
    QFont font = qApp->font();
    font.setPointSize(m_working.fontSize);
    qApp->setFont(font);

    PathTrackerStruct::instance().setTheme(m_working);
    // Sync advanced tab so the user can see the generated QSS
    ui->StylesheetEdit->setPlainText(qss);
    emit ThemeApplied();
}

void SettingsDialog::on_ResetButton_clicked()
{
    m_working = AppTheme::darkPreset();
    populateControlsFromTheme();
    const QString qss = m_working.toStylesheet();
    qApp->setStyleSheet(qss);
    QFont font = qApp->font();
    font.setPointSize(m_working.fontSize);
    qApp->setFont(font);
    PathTrackerStruct::instance().setTheme(m_working);
    ui->StylesheetEdit->setPlainText(qss);
}

void SettingsDialog::on_CloseButton_clicked()
{
    reject();
}

void SettingsDialog::on_AdvancedApplyButton_clicked()
{
    const QString qss = ui->StylesheetEdit->toPlainText();
    qApp->setStyleSheet(qss);
    PathTrackerStruct::instance().setStylesheet(qss);
    emit ThemeApplied();
}
