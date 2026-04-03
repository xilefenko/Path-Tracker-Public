#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings_dialog.h"
#include "pawnrow.h"
#include "QUuid"
#include "QJsonArray"
#include "QJsonObject"
#include "QFile"
#include "QListWidget"
#include "QListWidgetItem"
#include "QGraphicsOpacityEffect"
#include "QMessageBox"
#include "pawn_editor.h"
#include "QWidgetAction"
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QTableView>
#include <QHeaderView>
#include <QPushButton>
#include <QInputDialog>
#include <QGuiApplication>
#include <QStyleHints>
#include <algorithm>

// Helper: compute the correct stylesheet for System mode based on current OS palette
static QString systemModeStylesheet()
{
    const bool isDark = (QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light);
    return (isDark ? AppTheme::darkPreset() : AppTheme::lightPreset()).toStylesheet();
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    LoadFromJSon();

    // Apply initial stylesheet — use OS-matched preset when in System mode
    if (PathTrackerStruct::instance().theme().themeMode == ThemeMode::System)
        qApp->setStyleSheet(systemModeStylesheet());
    else
        qApp->setStyleSheet(PathTrackerStruct::instance().stylesheet());

    // Drive font size through QApplication::setFont() so Windows picks it up.
    // QSS font-size on QWidget doesn't reliably cascade on Windows at startup.
    {
        QFont f = qApp->font();
        f.setPointSize(PathTrackerStruct::instance().theme().fontSize);
        qApp->setFont(f);
    }

    // Track OS colour scheme changes so System mode stays in sync
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &MainWindow::OnSystemColorSchemeChanged);

    // Ensure there is always an active encounter so pawns can be added immediately.
    // Use startEmptyEncounter() — does NOT add a template to m_encounterTemplates,
    // so the runtime encounter never leaks into the Editor's Encounter list.
    if (!PathTrackerStruct::instance().activeEncounter())
        PathTrackerStruct::instance().startEmptyEncounter();

    ui->PawnList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->PawnList, &QListWidget::customContextMenuRequested, this, &MainWindow::ShowPawnContextMenu);

    ui->EffectList->setColumnCount(2);
    ui->EffectList->setHeaderLabels({"Property", "Value"});
    ui->EffectList->setRootIsDecorated(true);
    ui->EffectList->setAlternatingRowColors(true);
    ui->EffectList->setUniformRowHeights(true);
    ui->EffectList->setSortingEnabled(false);

    ui->AttributeList->setColumnCount(2);
    ui->AttributeList->setHeaderLabels({"Property", "Value"});
    ui->AttributeList->setRootIsDecorated(true);
    ui->AttributeList->setAlternatingRowColors(true);
    ui->AttributeList->setUniformRowHeights(true);
    ui->AttributeList->setSortingEnabled(false);

    ui->NextTurnButton->setEnabled(false);
    ui->UndoButton->setEnabled(false);
    ui->RedoButton->setEnabled(false);

    if (enc() && enc()->isOngoing) {
        for (PawnInstance* p : enc()->allCombatants()) {
            if (p->state == PawnState::Active) { CurrentPawnID = p->instanceId; break; }
        }
        if (CurrentPawnID.isEmpty() && !enc()->initiativeOrder.isEmpty() && enc()->initiativeOrder[0].pawn)
            CurrentPawnID = enc()->initiativeOrder[0].pawn->instanceId;
    }

    RefreshPawnList();

    if (enc() && enc()->isOngoing) {
        EncounterOngoing = true;
        ui->NextTurnButton->setEnabled(true);
    }
    UpdateStartStopButtonAppearance();

    if (!CurrentPawnID.isEmpty())
        FullRefresh();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  System colour scheme tracking
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::OnSystemColorSchemeChanged()
{
    if (PathTrackerStruct::instance().theme().themeMode != ThemeMode::System)
        return;
    qApp->setStyleSheet(systemModeStylesheet());
    RefreshPawnList();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Encounter accessor
// ─────────────────────────────────────────────────────────────────────────────

EncounterInstance* MainWindow::enc()
{
    return PathTrackerStruct::instance().activeEncounter();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Open editor dialog
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_pushButton_4_clicked()
{
    Editor editor(&PathTrackerStruct::instance(), this);
    editor.exec();
    RefreshPawnList();
}

void MainWindow::on_SettingsButton_clicked()
{
    SettingsDialog dialog(this);
    connect(&dialog, &SettingsDialog::ThemeApplied, this, &MainWindow::RefreshPawnList);
    dialog.exec();
    RefreshPawnList();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Save / Load
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::SaveToJson()
{
    PathTrackerStruct::instance().save();
}

void MainWindow::LoadFromJSon()
{
    PathTrackerStruct::instance().load();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Undo / Redo
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::SaveSnapshot()
{
    if (!enc()) return;
    m_pastStates.append(enc()->saveToJson());
    m_futureStates.clear();
    ui->UndoButton->setEnabled(true);
    ui->RedoButton->setEnabled(false);
}

void MainWindow::RestoreFromSnapshot(const QJsonObject& snap)
{
    if (!enc()) return;
    enc()->loadFromJson(snap);
    ui->RoundNumbeLabel->setText(QString::number(enc()->currentRound));
    if (!enc()->initiativeOrder.isEmpty() && enc()->initiativeOrder[0].pawn)
        CurrentPawnID = enc()->initiativeOrder[0].pawn->instanceId;
    RefreshPawnList();
    FullRefresh();
    SaveToJson();
}

void MainWindow::on_UndoButton_clicked()
{
    if (m_pastStates.isEmpty()) return;
    m_futureStates.prepend(enc()->saveToJson());
    RestoreFromSnapshot(m_pastStates.takeLast());
    ui->UndoButton->setEnabled(!m_pastStates.isEmpty());
    ui->RedoButton->setEnabled(true);
}

void MainWindow::on_DiceHistoryButton_clicked()
{
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Dice Roll History");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->resize(400, 300);

    auto* layout  = new QVBoxLayout(dlg);
    auto* view    = new QTableView(dlg);
    view->setModel(&DiceHistoryModel::instance());
    view->horizontalHeader()->setStretchLastSection(true);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(view);

    auto* btnRow   = new QHBoxLayout;
    auto* clearBtn = new QPushButton("Clear", dlg);
    auto* closeBtn = new QPushButton("Close", dlg);
    btnRow->addWidget(clearBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    connect(clearBtn, &QPushButton::clicked, dlg, [](){ DiceHistoryModel::instance().clear(); });
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    dlg->show();
}

void MainWindow::on_RedoButton_clicked()
{
    if (m_futureStates.isEmpty()) return;
    m_pastStates.append(enc()->saveToJson());
    RestoreFromSnapshot(m_futureStates.takeFirst());
    ui->UndoButton->setEnabled(true);
    ui->RedoButton->setEnabled(!m_futureStates.isEmpty());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pawn CRUD
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_PawnAdd_clicked()
{
    if (!enc()) return;

    int     initiative = ui->PawnNr->value();
    QString name       = ui->PawnLine->text();

    if (initiative == 0 || name.isEmpty()) return;

    SaveSnapshot();

    // Create a PawnTemplate, register it, then instantiate into the encounter.
    auto tmpl = std::make_unique<PawnTemplate>();
    tmpl->id   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    tmpl->name = name;
    QString templateId = tmpl->id;
    PathTrackerStruct::instance().addPawnTemplate(std::move(tmpl));

    PawnTemplate* pawnTemplate = PathTrackerStruct::instance().findPawnTemplate(templateId);
    enc()->addPawnInstance(*pawnTemplate);

    // The new pawn is the last entry added to ownedPawns.
    PawnInstance* newPawn = enc()->ownedPawns.back().get();
    enc()->setInitiative(newPawn->instanceId, initiative);
    enc()->sortInitiative();

    RefreshPawnList();
    SaveToJson();
}

void MainWindow::RefreshPawnList()
{
    ui->PawnList->clear();
    if (!enc()) return;

    for (const InitiativeEntry& entry : enc()->initiativeOrder) {
        if (entry.pawn)
            AddPawnListVisual(entry.pawn, entry.initiative);
    }

    // Graveyard: dead pawns removed from initiativeOrder
    QList<PawnInstance*> deadPawns;
    for (PawnInstance* pawn : enc()->allCombatants())
        if (pawn->state == PawnState::Dead) deadPawns.append(pawn);

    if (!deadPawns.isEmpty()) {
        auto* graveyardSeparator = new QListWidgetItem(QString::fromUtf8("\u26B0  Graveyard"), ui->PawnList);
        graveyardSeparator->setFlags(Qt::NoItemFlags);
        graveyardSeparator->setBackground(QColor(50, 15, 15));
        graveyardSeparator->setForeground(Qt::lightGray);
        for (PawnInstance* deadPawn : deadPawns)
            AddPawnListVisual(deadPawn, deadPawn->savedInitiative);
    }

    PopulatePawnAdderCombos();
}

void MainWindow::AddPawnListVisual(PawnInstance* Pawn, int initiative)
{
    QListWidgetItem* item = new QListWidgetItem(ui->PawnList);
    item->setData(Qt::UserRole, Pawn->instanceId);
    QWidget* row = Pawn->createListWidget(initiative);
    item->setSizeHint(row->sizeHint());
    ui->PawnList->setItemWidget(item, row);

    if (PawnRow* pawnRow = qobject_cast<PawnRow*>(row))
        pawnRow->SetSelected(Pawn->instanceId == CurrentPawnID);
}

void MainWindow::UpdatePawnSelectionIndicators()
{
    for (int i = 0; i < ui->PawnList->count(); ++i) {
        QListWidgetItem* item = ui->PawnList->item(i);
        if (!item) continue;
        QString itemId = item->data(Qt::UserRole).toString();
        if (PawnRow* row = qobject_cast<PawnRow*>(ui->PawnList->itemWidget(item)))
            row->SetSelected(itemId == CurrentPawnID);
    }
}

void MainWindow::DeletePawn(QString PawnID)
{
    if (!enc()) return;

    SaveSnapshot();

    // Clear initiative entries for this pawn first (they hold non-owning pointers).
    for (int i = static_cast<int>(enc()->initiativeOrder.size()) - 1; i >= 0; --i) {
        if (enc()->initiativeOrder[i].pawn &&
            enc()->initiativeOrder[i].pawn->instanceId == PawnID)
            enc()->initiativeOrder.removeAt(i);
    }

    // Remove from owned pawns (destructs the PawnInstance).
    for (int i = static_cast<int>(enc()->ownedPawns.size()) - 1; i >= 0; --i) {
        if (enc()->ownedPawns[i]->instanceId == PawnID) {
            enc()->ownedPawns.erase(enc()->ownedPawns.begin() + i);
            break;
        }
    }

    // Remove from active players if this is a player pawn (non-owning pointer, just unregisters).
    enc()->removePlayer(PawnID);

    if (CurrentPawnID == PawnID) {
        CurrentPawnID.clear();
        ui->EffectList->clear();
        ui->AttributeList->clear();
        ui->ActionList->clear();
    }

    RefreshPawnList();
    SaveToJson();
}


// ─────────────────────────────────────────────────────────────────────────────
//  Pawn list selection
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_PawnList_itemClicked(QListWidgetItem *item)
{
    CurrentPawnID = item->data(Qt::UserRole).toString();
    UpdatePawnSelectionIndicators();
}

void MainWindow::on_PawnList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!current) return;

    FirstItem = (ui->PawnList->row(current) == 0);
    CurrentPawnID = current->data(Qt::UserRole).toString();
    UpdatePawnSelectionIndicators();
    FullRefresh();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pawn context menu
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::ShowPawnContextMenu(const QPoint &pos)
{
    QListWidgetItem* item = ui->PawnList->itemAt(pos);
    if (!item) return;

    QString PawnID = item->data(Qt::UserRole).toString();

    PawnInstance* Pawn = nullptr;
    for (PawnInstance* p : enc()->allCombatants()) {
        if (p->instanceId == PawnID) { Pawn = p; break; }
    }
    if (!Pawn) return;

    QMenu Menu(this);

    // --- Edit Effect submenu ---
    QMenu* EditEffectMenu = Menu.addMenu(QIcon::fromTheme("document-properties"), "Edit Effect");
    for (const auto& effect : Pawn->effects) {
        EffectTemplate* tmpl = PathTrackerStruct::instance().findEffectTemplate(effect->templateId);
        QString effectName = tmpl ? tmpl->name : effect->templateId;

        QMenu* effectMenu = EditEffectMenu->addMenu(effectName);

        QAction* IncreaseLevel = effectMenu->addAction(QIcon::fromTheme("go-up"), "Increase Level");
        connect(IncreaseLevel, &QAction::triggered,
                this, [this, Pawn, effectPtr = effect.get()]() {
                    SaveSnapshot();
                    effectPtr->level++;
                    Pawn->resolveAttributes();
                    FullRefresh();
                    SaveToJson();
                });

        QAction* DecreaseLevel = effectMenu->addAction(QIcon::fromTheme("go-down"), "Decrease Level");
        connect(DecreaseLevel, &QAction::triggered,
                this, [this, Pawn, effectPtr = effect.get()]() {
                    SaveSnapshot();
                    effectPtr->level--;
                    Pawn->resolveAttributes();
                    FullRefresh();
                    SaveToJson();
                });

        QAction* IncreaseStage = effectMenu->addAction(QIcon::fromTheme("go-next"), "Increase Stage");
        connect(IncreaseStage, &QAction::triggered,
                this, [this, effectPtr = effect.get()]() {
                    SaveSnapshot();
                    effectPtr->advanceStage(); // syncBoundChildEffects called internally
                    FullRefresh();
                    SaveToJson();
                });

        QAction* DecreaseStage = effectMenu->addAction(QIcon::fromTheme("go-previous"), "Decrease Stage");
        connect(DecreaseStage, &QAction::triggered,
                this, [this, effectPtr = effect.get()]() {
                    SaveSnapshot();
                    effectPtr->regressStage(); // syncBoundChildEffects called internally
                    FullRefresh();
                    SaveToJson();
                });

    }

    // --- Remove Effect submenu ---
    QMenu* RemoveEffectMenu = Menu.addMenu(QIcon::fromTheme("list-remove"), "Remove Effect");
    for (const auto& effect : Pawn->effects) {
        EffectTemplate* tmpl = PathTrackerStruct::instance().findEffectTemplate(effect->templateId);
        QString effectName = tmpl ? tmpl->name : effect->templateId;

        QAction* RemoveEffect = RemoveEffectMenu->addAction(effectName);
        connect(RemoveEffect, &QAction::triggered,
                this, [this, Pawn, instId = effect->instanceId]() {
                    SaveSnapshot();
                    Pawn->removeEffect(instId);
                    FullRefresh();
                    SaveToJson();
                });
    }

    // --- Add Effect submenu ---
    QMenu* AddEffectMenu = Menu.addMenu(QIcon::fromTheme("list-add"), "Add Effect...");
    for (EffectTemplate* tmpl : PathTrackerStruct::instance().allEffectTemplates()) {
        QAction* act = AddEffectMenu->addAction(tmpl->name);
        connect(act, &QAction::triggered,
                this, [this, Pawn, tmpl]() {
                    SaveSnapshot();
                    Pawn->applyEffect(*tmpl);
                    FullRefresh();
                    SaveToJson();
                });
    }

    // State-change actions (only during an encounter)
    if (EncounterOngoing) {
        Menu.addSeparator();

        if (Pawn->state == PawnState::Unconscious) {
            QAction* markConsciousAction = Menu.addAction(
                QIcon::fromTheme("dialog-ok"), "Mark Conscious");
            connect(markConsciousAction, &QAction::triggered, this, [this, Pawn]() {
                SaveSnapshot();
                enc()->markUnconscious(Pawn->instanceId);
                RefreshPawnList(); FullRefresh(); SaveToJson();
            });
        } else if (Pawn->state != PawnState::Dead) {
            QAction* markUnconsciousAction = Menu.addAction(
                QIcon::fromTheme("dialog-warning"), "Mark Unconscious");
            connect(markUnconsciousAction, &QAction::triggered, this, [this, Pawn]() {
                SaveSnapshot();
                enc()->markUnconscious(Pawn->instanceId);
                RefreshPawnList(); FullRefresh(); SaveToJson();
            });
        }

        if (Pawn->state == PawnState::Dead) {
            QAction* reviveAction = Menu.addAction(
                QIcon::fromTheme("view-refresh"), "Revive");
            connect(reviveAction, &QAction::triggered, this, [this, Pawn]() {
                SaveSnapshot();
                enc()->revive(Pawn->instanceId);
                RefreshPawnList(); FullRefresh(); SaveToJson();
            });
        } else {
            QAction* killAction = Menu.addAction(
                QIcon::fromTheme("process-stop"), "Kill Pawn");
            connect(killAction, &QAction::triggered, this, [this, Pawn]() {
                SaveSnapshot();
                enc()->markDead(Pawn->instanceId);
                RefreshPawnList(); FullRefresh(); SaveToJson();
            });
        }
    }

    // HP manipulation actions
    Menu.addSeparator();
    {
        int currentHp = Pawn->getAttribute(HP_KEY);
        int maxHp = 0;
        for (const AttributeValue& av : Pawn->attributes)
            if (av.key == HP_KEY) { maxHp = av.baseValue; break; }

        QAction* healAction = Menu.addAction(QIcon::fromTheme("go-up"), "Heal...");
        connect(healAction, &QAction::triggered, this, [this, Pawn, currentHp, maxHp]() {
            bool ok = false;
            int amount = QInputDialog::getInt(this, "Heal", "Heal amount:", 0, 0, 9999, 1, &ok);
            if (!ok || amount <= 0) return;
            SaveSnapshot();
            int newHp = (maxHp > 0) ? qMin(currentHp + amount, maxHp) : currentHp + amount;
            Pawn->setAttribute(HP_KEY, newHp);
            Pawn->resolveAttributes();
            RefreshPawnList(); FullRefresh(); SaveToJson();
        });

        QAction* damageAction = Menu.addAction(QIcon::fromTheme("go-down"), "Damage...");
        connect(damageAction, &QAction::triggered, this, [this, Pawn, currentHp]() {
            bool ok = false;
            int amount = QInputDialog::getInt(this, "Damage", "Damage amount:", 0, 0, 9999, 1, &ok);
            if (!ok || amount <= 0) return;
            SaveSnapshot();
            Pawn->setAttribute(HP_KEY, currentHp - amount);
            Pawn->resolveAttributes();
            RefreshPawnList(); FullRefresh(); SaveToJson();
        });

        QAction* setHpAction = Menu.addAction(QIcon::fromTheme("document-edit"), "Set HP...");
        connect(setHpAction, &QAction::triggered, this, [this, Pawn, currentHp]() {
            bool ok = false;
            int newHp = QInputDialog::getInt(this, "Set HP", "New HP value:", currentHp, -9999, 9999, 1, &ok);
            if (!ok) return;
            SaveSnapshot();
            Pawn->setAttribute(HP_KEY, newHp);
            Pawn->resolveAttributes();
            RefreshPawnList(); FullRefresh(); SaveToJson();
        });
    }

    Menu.addSeparator();

    QAction* DeleteAction = Menu.addAction(QIcon::fromTheme("edit-delete"), "Delete Pawn");
    connect(DeleteAction, &QAction::triggered,
            this, [this, PawnID]() {
                DeletePawn(PawnID);
            });

    Menu.exec(ui->PawnList->mapToGlobal(pos));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Runtime display
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::FullRefresh()
{
    ui->EffectList->clear();
    ui->ActionList->clear();
    ui->AttributeList->clear();

    if (!enc()) return;

    PawnInstance* Pawn = nullptr;
    for (PawnInstance* p : enc()->allCombatants()) {
        if (p->instanceId == CurrentPawnID) { Pawn = p; break; }
    }
    if (!Pawn) return;

    // --- Active Effects ---
    for (const auto& effect : Pawn->effects) {
        EffectTemplate* tmpl = PathTrackerStruct::instance().findEffectTemplate(effect->templateId);
        QString effectName   = tmpl ? tmpl->name : effect->templateId;

        auto* effectItem = new QTreeWidgetItem(ui->EffectList);
        effectItem->setText(0, effectName);

        QFont font = effectItem->font(0);
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        effectItem->setFont(0, font);
        effectItem->setExpanded(true);
        effectItem->setFlags(effectItem->flags() & ~Qt::ItemIsSelectable);
        ui->EffectList->addTopLevelItem(effectItem);

        for (const auto& [prop, val] : effect->getDisplayProperties(tmpl))
            AddPropertyWithValue(effectItem, prop, val);
    }

    // --- Attributes ---
    for (const AttributeValue& attrVal : Pawn->attributes) {
        QString displayName = attrVal.key;
        if (PathTrackerStruct::attributes().hasAttribute(attrVal.key))
            displayName = PathTrackerStruct::attributes().getAttribute(attrVal.key).displayName;

        auto* attrItem = new QTreeWidgetItem(ui->AttributeList);
        attrItem->setText(0, displayName);
        attrItem->setText(1, QString::number(attrVal.currentValue));

        QFont font = attrItem->font(0);
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        attrItem->setFont(0, font);
        attrItem->setFont(1, font);
        attrItem->setExpanded(true);
        attrItem->setFlags(attrItem->flags() & ~Qt::ItemIsSelectable);
        ui->AttributeList->addTopLevelItem(attrItem);

        if (attrVal.baseValue != 0)
            AddPropertyWithValue(attrItem, "Base", QString::number(attrVal.baseValue));

        for (const auto& effect : Pawn->effects) {
            EffectTemplate* fxTmpl = PathTrackerStruct::instance().findEffectTemplate(effect->templateId);
            QString fxName = fxTmpl ? fxTmpl->name : effect->templateId;

            const Stage* stage = effect->currentStage();
            if (!stage) continue;
            for (const AttributeModifier& mod : stage->modifiers) {
                if (mod.attributeKey != attrVal.key) continue;

                QString sign;
                switch (mod.op) {
                    case ModifierOp::Plus:     sign = QString("+%1").arg(mod.value); break;
                    case ModifierOp::Minus:    sign = QString("-%1").arg(mod.value); break;
                    case ModifierOp::Multiply: sign = QString("×%1").arg(mod.value); break;
                }
                QString label = mod.description.isEmpty()
                              ? fxName
                              : QString("%1 (%2)").arg(fxName, mod.description);
                AddPropertyWithValue(attrItem, label, sign);
            }
        }
    }

    // --- Effect-only attributes (modifier targets the pawn doesn't own) ---
    QSet<QString> displayedKeys;
    for (const AttributeValue& attrVal : Pawn->attributes)
        displayedKeys.insert(attrVal.key);

    QMap<QString, QList<QPair<QString, AttributeModifier>>> effectOnlyMods;
    for (const auto& effect : Pawn->effects) {
        EffectTemplate* fxTmpl = PathTrackerStruct::instance().findEffectTemplate(effect->templateId);
        QString fxName = fxTmpl ? fxTmpl->name : effect->templateId;
        const Stage* stage = effect->currentStage();
        if (!stage) continue;
        for (const AttributeModifier& mod : stage->modifiers) {
            if (displayedKeys.contains(mod.attributeKey)) continue;
            effectOnlyMods[mod.attributeKey].append({fxName, mod});
        }
    }

    for (auto it = effectOnlyMods.begin(); it != effectOnlyMods.end(); ++it) {
        const QString& key = it.key();
        const auto& mods = it.value();

        QString displayName = key;
        if (PathTrackerStruct::attributes().hasAttribute(key))
            displayName = PathTrackerStruct::attributes().getAttribute(key).displayName;

        double total = 0.0;
        for (const auto& [fxName, mod] : mods) {
            switch (mod.op) {
                case ModifierOp::Plus:     total += mod.value; break;
                case ModifierOp::Minus:    total -= mod.value; break;
                case ModifierOp::Multiply: total *= mod.value; break;
            }
        }

        auto* attrItem = new QTreeWidgetItem(ui->AttributeList);
        attrItem->setText(0, displayName);
        attrItem->setText(1, QString::number(total));

        QFont font = attrItem->font(0);
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        attrItem->setFont(0, font);
        attrItem->setFont(1, font);
        attrItem->setExpanded(true);
        attrItem->setFlags(attrItem->flags() & ~Qt::ItemIsSelectable);
        ui->AttributeList->addTopLevelItem(attrItem);

        for (const auto& [fxName, mod] : mods) {
            QString sign;
            switch (mod.op) {
                case ModifierOp::Plus:     sign = QString("+%1").arg(mod.value); break;
                case ModifierOp::Minus:    sign = QString("-%1").arg(mod.value); break;
                case ModifierOp::Multiply: sign = QString("×%1").arg(mod.value); break;
            }
            QString label = mod.description.isEmpty()
                          ? fxName
                          : QString("%1 (%2)").arg(fxName, mod.description);
            AddPropertyWithValue(attrItem, label, sign);
        }
    }

    // --- Actions ---
    for (const auto& effect : Pawn->effects) {
        const Stage* stage = effect->currentStage();
        if (!stage) continue;
        for (const auto& action : stage->actions) {
            QWidget* w = action->createRuntimeWidget(
                ui->ActionList,
                Pawn,
                [this]{ RefreshPawnList(); });
            if (!w) continue;
            auto* actionItem = new QListWidgetItem(ui->ActionList);
            actionItem->setSizeHint(QSize(ui->ActionList->viewport()->width(),
                                          w->sizeHint().height()));
            ui->ActionList->setItemWidget(actionItem, w);
        }
    }

    // Grey out the entire action panel when the pawn is not the active combatant
    const bool isActive = (Pawn->state == PawnState::Active);
    if (!isActive) {
        auto* fx = new QGraphicsOpacityEffect(ui->ActionList);
        fx->setOpacity(0.4);
        ui->ActionList->setGraphicsEffect(fx);
        ui->ActionList->setEnabled(false);
    } else {
        ui->ActionList->setGraphicsEffect(nullptr);
        ui->ActionList->setEnabled(true);
    }

    RefreshPawnIcons();
}

void MainWindow::RefreshPawnIcons()
{
    for (int i = 0; i < ui->PawnList->count(); ++i) {
        QListWidgetItem* item = ui->PawnList->item(i);
        if (!item) continue;
        if (PawnRow* row = qobject_cast<PawnRow*>(ui->PawnList->itemWidget(item)))
            row->RefreshIcons();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Turn / encounter management
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_NextTurnButton_clicked()
{
    if (!enc()) return;

    SaveSnapshot();
    enc()->advanceTurn();
    if (!enc()->initiativeOrder.isEmpty() && enc()->initiativeOrder[0].pawn)
        CurrentPawnID = enc()->initiativeOrder[0].pawn->instanceId;
    ui->RoundNumbeLabel->setText(QString::number(enc()->currentRound));
    RefreshPawnList();
    FullRefresh();
    SaveToJson();
}

void MainWindow::UpdateStartStopButtonAppearance()
{
    const AppTheme& t = PathTrackerStruct::instance().theme();
    if (EncounterOngoing) {
        ui->StartStopButton->setStyleSheet(
            "background-color: #cc2222; color: white;");
        ui->StartStopButton->setText("Stop Encounter");
    } else {
        const QString accText = (t.accentColor.lightness() > 128) ? "black" : "white";
        ui->StartStopButton->setStyleSheet(
            QString("background-color: %1; color: %2;")
                .arg(t.accentColor.name(), accText));
        ui->StartStopButton->setText("Start Encounter");
    }
}

void MainWindow::on_StartStopButton_clicked()
{
    if (!enc() || enc()->initiativeOrder.isEmpty()) {
        QMessageBox::information(this, "Error", "The Encounter has no Participants");
        return;
    }

    if (!EncounterOngoing) {
        // --- Start encounter ---
        EncounterOngoing = true;
        UpdateStartStopButtonAppearance();

        enc()->isOngoing = true;

        // Reset all pawn states to Waiting
        for (PawnInstance* p : enc()->allCombatants())
            p->state = PawnState::Waiting;

        enc()->sortInitiative();

        // Mark first pawn Active and fire initial events
        if (!enc()->initiativeOrder.isEmpty() && enc()->initiativeOrder[0].pawn) {
            enc()->initiativeOrder[0].pawn->state = PawnState::Active;
            CurrentPawnID = enc()->initiativeOrder[0].pawn->instanceId;
        }

        enc()->fireRoundStart();
        if (PawnInstance* first = enc()->currentActor())
            first->fireEvent(EventType::OnTurnStart);

        enc()->currentRound = 1;
        m_pastStates.clear();
        m_futureStates.clear();
        SaveSnapshot();
        ui->UndoButton->setEnabled(false);   // first snapshot – nothing to undo yet
        ui->RedoButton->setEnabled(false);
        ui->RoundNumbeLabel->setText("1");
        ui->NextTurnButton->setEnabled(true);
        RefreshPawnList();
        FullRefresh();
        SaveToJson();
        return;
    }

    // --- Stop encounter: show confirmation dialog ---
    QDialog confirmDlg(this);
    confirmDlg.setWindowTitle("Stop Encounter?");
    auto* dlgLayout = new QVBoxLayout(&confirmDlg);
    dlgLayout->addWidget(new QLabel("What would you like to do?", &confirmDlg));

    auto* btnRow = new QHBoxLayout;
    auto* noBtn               = new QPushButton("No",                       &confirmDlg);
    auto* yesBtn              = new QPushButton("Yes (Stop)",                &confirmDlg);
    auto* clearDeadBtn        = new QPushButton("Yes + Clear Dead Pawns",    &confirmDlg);
    auto* clearEncounterBtn   = new QPushButton("Yes + Clear Encounter",     &confirmDlg);
    btnRow->addWidget(noBtn);
    btnRow->addWidget(yesBtn);
    btnRow->addWidget(clearDeadBtn);
    btnRow->addWidget(clearEncounterBtn);
    dlgLayout->addLayout(btnRow);

    int result = 0;   // 0=no, 1=stop, 2=clearDead, 3=clearAll
    connect(noBtn,             &QPushButton::clicked, &confirmDlg, [&]{ result = 0; confirmDlg.accept(); });
    connect(yesBtn,            &QPushButton::clicked, &confirmDlg, [&]{ result = 1; confirmDlg.accept(); });
    connect(clearDeadBtn,      &QPushButton::clicked, &confirmDlg, [&]{ result = 2; confirmDlg.accept(); });
    connect(clearEncounterBtn, &QPushButton::clicked, &confirmDlg, [&]{ result = 3; confirmDlg.accept(); });

    confirmDlg.exec();
    if (result == 0) return;

    // Stop the encounter
    EncounterOngoing = false;
    UpdateStartStopButtonAppearance();
    enc()->isOngoing = false;

    if (result == 2) {
        // Clear dead pawns from initiative order and owned pawns
        enc()->initiativeOrder.erase(
            std::remove_if(enc()->initiativeOrder.begin(), enc()->initiativeOrder.end(),
                [](const InitiativeEntry& e){ return e.pawn && e.pawn->state == PawnState::Dead; }),
            enc()->initiativeOrder.end());
        enc()->ownedPawns.erase(
            std::remove_if(enc()->ownedPawns.begin(), enc()->ownedPawns.end(),
                [](const std::unique_ptr<PawnInstance>& p){ return p->state == PawnState::Dead; }),
            enc()->ownedPawns.end());
    } else if (result == 3) {
        // Clear all pawns
        enc()->initiativeOrder.clear();
        enc()->ownedPawns.clear();
        enc()->activePlayers.clear();
    }

    // Reset remaining pawn states
    for (PawnInstance* p : enc()->allCombatants())
        p->state = PawnState::Waiting;

    m_pastStates.clear();
    m_futureStates.clear();
    ui->UndoButton->setEnabled(false);
    ui->RedoButton->setEnabled(false);
    ui->NextTurnButton->setEnabled(false);
    ui->RoundNumbeLabel->setText("1");
    RefreshPawnList();
    SaveToJson();
}

void MainWindow::on_PawnList_itemDoubleClicked(QListWidgetItem *item)
{
    QString pawnId = item->data(Qt::UserRole).toString();
    PawnInstance* pawn = nullptr;
    for (PawnInstance* p : enc()->allCombatants()) {
        if (p->instanceId == pawnId) { pawn = p; break; }
    }
    if (!pawn) return;

    pawn_editor* editor = new pawn_editor(&PathTrackerStruct::instance(), pawn, this);
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->setWindowTitle("Editing: " + pawn->displayName);
    editor->show();
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI helpers
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::ClearLayout(QLayout* layout)
{
    if (!layout) return;

    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }
}

void MainWindow::AddPropertyWithValue(QTreeWidgetItem* Parent, QString Property, QString Value)
{
    auto* propItem = new QTreeWidgetItem(Parent);
    propItem->setText(0, Property);
    propItem->setText(1, Value);
    propItem->setFlags(propItem->flags() & ~Qt::ItemIsSelectable);
}

void MainWindow::UpdateAttribute(const Attribute&) {}
void MainWindow::UpdateEffect(const EffectTemplate&) {}
void MainWindow::Refresh() {}

// ─────────────────────────────────────────────────────────────────────────────
//  PawnAdder navigation
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_PawnAdderPrev_clicked()
{
    int i = ui->PawnAdder->currentIndex();
    ui->PawnAdder->setCurrentIndex((i - 1 + ui->PawnAdder->count()) % ui->PawnAdder->count());
}

void MainWindow::on_PawnAdderNext_clicked()
{
    int i = ui->PawnAdder->currentIndex();
    ui->PawnAdder->setCurrentIndex((i + 1) % ui->PawnAdder->count());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Populate combos
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::PopulatePawnAdderCombos()
{
    ui->EncounterCombo->clear();
    for (EncounterTemplate* tmpl : PathTrackerStruct::instance().allEncounterTemplates())
        ui->EncounterCombo->addItem(tmpl->name, tmpl->id);

    ui->PawnTemplateCombo->clear();
    for (PawnTemplate* tmpl : PathTrackerStruct::instance().allPawnTemplates())
        ui->PawnTemplateCombo->addItem(tmpl->name, tmpl->id);

    ui->PlayerCombo->clear();
    if (!enc()) return;
    for (Player* player : PlayerRegistry::instance().allPlayers()) {
        bool alreadyActive = false;
        for (Player* active : enc()->activePlayers) {
            if (active->instanceId == player->instanceId) {
                alreadyActive = true;
                break;
            }
        }
        if (!alreadyActive)
            ui->PlayerCombo->addItem(player->displayName, player->instanceId);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Add: Template Pawn (page 3)
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_PawnTemplateAddBtn_clicked()
{
    if (!enc()) return;

    QString id = ui->PawnTemplateCombo->currentData().toString();
    if (id.isEmpty()) return;

    PawnTemplate* tmpl = PathTrackerStruct::instance().findPawnTemplate(id);
    if (!tmpl) return;

    SaveSnapshot();

    int initiative = ui->PawnInitSpin->value();
    enc()->addPawnInstance(*tmpl);
    enc()->setInitiative(enc()->ownedPawns.back()->instanceId, initiative);
    enc()->sortInitiative();

    RefreshPawnList();
    SaveToJson();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Add: Player (page 2)
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_PlayerAddBtn_clicked()
{
    if (!enc()) return;

    QString id = ui->PlayerCombo->currentData().toString();
    if (id.isEmpty()) return;

    Player* player = PlayerRegistry::instance().findPlayer(id);
    if (!player) return;

    SaveSnapshot();

    int initiative = ui->PlayerInitSpin->value();
    enc()->addPlayer(player);
    enc()->setInitiative(player->instanceId, initiative);
    enc()->sortInitiative();

    RefreshPawnList();
    SaveToJson();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Add: Encounter Group (page 0)
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::on_EncounterAddBtn_clicked()
{
    if (!enc()) return;

    QString id = ui->EncounterCombo->currentData().toString();
    if (id.isEmpty()) return;

    EncounterTemplate* tmpl = PathTrackerStruct::instance().findEncounterTemplate(id);
    if (!tmpl) return;

    QDialog dlg(this);
    dlg.setWindowTitle("Set Initiative \u2013 " + tmpl->name);
    QVBoxLayout* dlgLayout = new QVBoxLayout(&dlg);

    QList<QPair<PawnTemplate*, QSpinBox*>> pawnRows;
    for (const QString& pawnId : tmpl->pawnTemplateIds) {
        PawnTemplate* pawnTmpl = PathTrackerStruct::instance().findPawnTemplate(pawnId);
        if (!pawnTmpl) continue;

        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel(pawnTmpl->name));
        QSpinBox* spin = new QSpinBox();
        spin->setRange(1, 30);
        row->addWidget(spin);
        dlgLayout->addLayout(row);
        pawnRows.append({pawnTmpl, spin});
    }

    QDialogButtonBox* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlgLayout->addWidget(btnBox);

    if (dlg.exec() != QDialog::Accepted) return;

    SaveSnapshot();

    for (auto& [pawnTmpl, spin] : pawnRows) {
        enc()->addPawnInstance(*pawnTmpl);
        enc()->setInitiative(enc()->ownedPawns.back()->instanceId, spin->value());
    }
    enc()->sortInitiative();

    RefreshPawnList();
    SaveToJson();
}
