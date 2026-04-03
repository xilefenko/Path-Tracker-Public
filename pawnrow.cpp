#include "pawnrow.h"
#include "ui_pawnrow.h"
#include "PathTrackerStruct.h"
#include "PictureHelper.h"
#include <QPixmap>
#include <QFileInfo>
#include <QLabel>

PawnRow::PawnRow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PawnRow)
{
    ui->setupUi(this);
    // Required for widget-level background-color QSS to paint on a custom widget
    setAttribute(Qt::WA_StyledBackground, true);
}

PawnRow::~PawnRow()
{
    delete ui;
}

void PawnRow::SetData(PawnInstance* PawnData, int initiative)
{
    ui->PriorityLabel->setText(QString::number(initiative));
    ui->NameLabel->setText(PawnData->displayName);

    setGraphicsEffect(nullptr);
    setStyleSheet("");

    // Portrait
    QString portraitPath;
    if (Player* player = dynamic_cast<Player*>(PawnData))
    {
        portraitPath = player->PictureFilePath;
    }
    else if (!PawnData->templateId.isEmpty())
    {
        PawnTemplate* tmpl = PathTrackerStruct::instance().findPawnTemplate(PawnData->templateId);
        if (tmpl) portraitPath = tmpl->PictureFilePath;
    }

    if (!portraitPath.isEmpty())
    {
        QPixmap pixmap(ResolvePicturePath(portraitPath));
        if (!pixmap.isNull())
            ui->PortraitLabel->setPixmap(pixmap);
        else
            ui->PortraitLabel->clear();
    }
    else
    {
        ui->PortraitLabel->clear();
    }

    // Effect icons
    QLayoutItem* layoutItem;
    while ((layoutItem = ui->IconContainer->takeAt(0)) != nullptr)
    {
        delete layoutItem->widget();
        delete layoutItem;
    }

    for (const auto& effectInstance : PawnData->effects)
    {
        EffectTemplate* effectTemplate = PathTrackerStruct::instance().findEffectTemplate(effectInstance->templateId);
        if (!effectTemplate || effectTemplate->PictureFilePath.isEmpty()) continue;

        QPixmap iconPixmap(ResolvePicturePath(effectTemplate->PictureFilePath));
        if (iconPixmap.isNull()) continue;

        auto* iconLabel = new QLabel(this);
        iconLabel->setFixedSize(24, 24);
        iconLabel->setScaledContents(true);
        iconLabel->setPixmap(iconPixmap);
        ui->IconContainer->addWidget(iconLabel);
    }
    ui->IconContainer->addStretch();

    // HP progress bar + numeric HP/AC labels
    int maxHp = 0;
    for (const AttributeValue& av : PawnData->attributes)
        if (av.key == HP_KEY) { maxHp = av.baseValue; break; }
    int currentHp = PawnData->getAttribute(HP_KEY);
    if (maxHp > 0) {
        ui->progressBar->setMaximum(maxHp);
        ui->progressBar->setValue(qMax(0, currentHp));
        ui->progressBar->show();
        ui->HpLabel->setText(QString("HP %1/%2").arg(currentHp).arg(maxHp));
        ui->HpLabel->show();
    } else {
        ui->progressBar->hide();
        ui->HpLabel->hide();
    }

    int ac = PawnData->getAttribute(AC_KEY);
    if (ac != 0) {
        ui->AcLabel->setText(QString("AC %1").arg(ac));
        ui->AcLabel->show();
    } else {
        ui->AcLabel->hide();
    }

    m_pawnData   = PawnData;
    m_initiative = initiative;
    ApplyStylesheet(false);
}

void PawnRow::RefreshIcons()
{
    if (!m_pawnData) return;

    QLayoutItem* layoutItem;
    while ((layoutItem = ui->IconContainer->takeAt(0)) != nullptr)
    {
        delete layoutItem->widget();
        delete layoutItem;
    }

    for (const auto& effectInstance : m_pawnData->effects)
    {
        EffectTemplate* effectTemplate = PathTrackerStruct::instance().findEffectTemplate(effectInstance->templateId);
        if (!effectTemplate || effectTemplate->PictureFilePath.isEmpty()) continue;

        QPixmap iconPixmap(ResolvePicturePath(effectTemplate->PictureFilePath));
        if (iconPixmap.isNull()) continue;

        auto* iconLabel = new QLabel(this);
        iconLabel->setFixedSize(24, 24);
        iconLabel->setScaledContents(true);
        iconLabel->setPixmap(iconPixmap);
        ui->IconContainer->addWidget(iconLabel);
    }
    ui->IconContainer->addStretch();
}

void PawnRow::ApplyStylesheet(bool selected)
{
    if (!m_pawnData) return;

    const AppTheme& t = PathTrackerStruct::instance().theme();

    QColor bg;
    switch (m_pawnData->state) {
    case PawnState::Active:      bg = t.pawnActiveColor;      break;
    case PawnState::Spent:       bg = t.pawnSpentColor;       break;
    case PawnState::Unconscious: bg = t.pawnUnconsciousColor; break;
    case PawnState::Dead:        bg = t.pawnDeadColor;        break;
    default:                     bg = t.pawnWaitingColor;     break;
    }

    QString border = selected
        ? QString("border-left: 4px solid %1;").arg(t.accentColor.name())
        : "border-left: none;";

    setStyleSheet(QString(
        "PawnRow { background-color: %1; %2 }"
        "PawnRow QLabel { background-color: transparent; color: %3; }"
        "PawnRow QLabel#PortraitLabel { border: 2px solid %4; border-radius: 2px; }"
    ).arg(bg.name(), border, t.textColor.name(), t.accentColor.name()));
}

void PawnRow::SetSelected(bool selected)
{
    ApplyStylesheet(selected);
}
