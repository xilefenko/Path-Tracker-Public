#include "editor.h"
#include "ui_editor.h"
#include "editor_effectheader.h"
#include "editor_list.h"
#include "editor_stages.h"
#include "editor_pawntemplate.h"
#include "editor_encountertemplate.h"
#include "editor_player.h"
#include "QUuid"
#include "QMessageBox"
#include "QLabel"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QDialogButtonBox>
#include <algorithm>

// Returns baseName if not in existingNames, otherwise baseName + " 2", " 3", …
static QString UniqueName(const QString& baseName, const QStringList& existingNames)
{
    if (!existingNames.contains(baseName))
        return baseName;
    int n = 2;
    while (existingNames.contains(QString("%1 %2").arg(baseName).arg(n)))
        ++n;
    return QString("%1 %2").arg(baseName).arg(n);
}

Editor::Editor(QWidget *parent): QDialog(parent), ui(new Ui::Editor)
{
    ui->setupUi(this);
}

Editor::~Editor()
{
    delete ui;
}

Editor::Editor(PathTrackerStruct* Data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Editor)
    , DataPtr(Data)
{
    ui->setupUi(this);
    RequestRefresh();

    ui->EffectList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->EffectList, &QListWidget::customContextMenuRequested, this, &Editor::ShowEffectContextMenu);

    ui->AttributeList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AttributeList, &QListWidget::customContextMenuRequested, this, &Editor::ShowActionContextMenu);

    ui->PawnList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->PawnList, &QListWidget::customContextMenuRequested, this, &Editor::ShowPawnContextMenu);

    ui->EncounterList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->EncounterList, &QListWidget::customContextMenuRequested, this, &Editor::ShowEncounterContextMenu);

    ui->PlayerList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->PlayerList, &QListWidget::customContextMenuRequested, this, &Editor::ShowPlayerContextMenu);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Context menus
// ─────────────────────────────────────────────────────────────────────────────

void Editor::ShowEffectContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QListWidgetItem *item = ui->EffectList->itemAt(pos);

    if (item) {
        QString effectID = item->data(Qt::UserRole).toString();

        QAction *removeAction = menu.addAction("Remove Effect");
        connect(removeAction, &QAction::triggered, [this, item, effectID]() {
            // Find pawn templates that reference this effect
            QStringList usedByPawns;
            for (PawnTemplate* pawn : DataPtr->allPawnTemplates())
                for (const ChildEffectRef& ref : pawn->defaultEffects)
                    if (ref.effectTemplateId == effectID && !usedByPawns.contains(pawn->name))
                        usedByPawns.append(pawn->name);

            QString msg = "Are you sure you want to remove this effect?";
            if (!usedByPawns.isEmpty()) {
                msg += QString("\n\nPawns with this default effect: %1").arg(usedByPawns.join(", "));
                msg += "\n\nThe default effect reference will be removed from those pawns.";
            }

            QMessageBox::StandardButton reply =
                QMessageBox::question(this, "Confirm Removal", msg, QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                // Cascade: remove ChildEffectRef from all pawn templates
                for (PawnTemplate* pawn : DataPtr->allPawnTemplates())
                    pawn->defaultEffects.removeIf([&](const ChildEffectRef& ref){ return ref.effectTemplateId == effectID; });
                DataPtr->removeEffectTemplate(effectID);
                delete item;
                if (CurrentEffectID == effectID) {
                    CurrentEffectID.clear();
                    ClearDetailsPanel();
                }
                SaveSlot();
            }
        });

        QAction *renameAction = menu.addAction("Rename Effect");
        connect(renameAction, &QAction::triggered, [this, effectID]() {
            EffectTemplate* tmpl = DataPtr->findEffectTemplate(effectID);
            if (tmpl) {
                ui->EffectLine->setText(tmpl->name);
                ui->EffectButton->setText("Edit Effect");
                CurrentEffectID = effectID;
                rename = true;
            }
        });
    }

    menu.exec(ui->EffectList->mapToGlobal(pos));
}

void Editor::ShowActionContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QListWidgetItem *item = ui->AttributeList->itemAt(pos);

    if (item) {
        QString attributeKey = item->data(Qt::UserRole).toString();

        QAction *removeAction = menu.addAction("Remove Attribute");
        connect(removeAction, &QAction::triggered, [this, item, attributeKey]() {
            // Find all effects and pawns that use this attribute
            QStringList usedByEffects, usedByPawns;
            for (EffectTemplate* eff : DataPtr->allEffectTemplates())
                for (const Stage& s : eff->stages)
                    for (const AttributeModifier& m : s.modifiers)
                        if (m.attributeKey == attributeKey && !usedByEffects.contains(eff->name))
                            usedByEffects.append(eff->name);
            for (PawnTemplate* pawn : DataPtr->allPawnTemplates())
                for (const AttributeValue& av : pawn->baseAttributes)
                    if (av.key == attributeKey && !usedByPawns.contains(pawn->name))
                        usedByPawns.append(pawn->name);

            // Build replacement options (all attributes except the one being deleted)
            QList<Attribute> otherAttrs;
            for (const Attribute& a : PathTrackerStruct::attributes().allAttributes())
                if (a.key != attributeKey)
                    otherAttrs.append(a);

            // Find players that carry this attribute AND are currently in combat
            QStringList affectedActivePlayers;
            if (DataPtr->activeEncounter()) {
                for (Player* player : DataPtr->activeEncounter()->activePlayers)
                    for (const AttributeValue& av : player->attributes)
                        if (av.key == attributeKey && !affectedActivePlayers.contains(player->playerName))
                            affectedActivePlayers.append(player->playerName);
            }

            bool isUsed = !usedByEffects.isEmpty() || !usedByPawns.isEmpty();

            // Build confirmation dialog
            QDialog confirmDlg(this);
            confirmDlg.setWindowTitle("Confirm Removal");
            auto* layout = new QVBoxLayout(&confirmDlg);

            QString msg = "Are you sure you want to remove this attribute?";
            if (!usedByEffects.isEmpty())
                msg += QString("\n\nEffects affected: %1").arg(usedByEffects.join(", "));
            if (!usedByPawns.isEmpty())
                msg += QString("\nPawns affected: %1").arg(usedByPawns.join(", "));
            if (DataPtr->activeEncounter()) {
                if (!affectedActivePlayers.isEmpty())
                    msg += QString("\n\nWarning: the following players are currently in combat and "
                                   "will be affected immediately: %1").arg(affectedActivePlayers.join(", "));
                else
                    msg += "\n\nAn encounter is currently active. Owned combatants already in "
                           "combat will not be affected until the encounter ends.";
            }
            layout->addWidget(new QLabel(msg, &confirmDlg));

            // Replacement combobox — only shown when the attribute is actually in use
            QComboBox* replacementCombo = nullptr;
            if (isUsed && !otherAttrs.isEmpty()) {
                auto* replaceLabel = new QLabel("Replace references with:", &confirmDlg);
                replacementCombo = new QComboBox(&confirmDlg);
                replacementCombo->addItem("(none — remove references)", QString());
                for (const Attribute& a : otherAttrs)
                    replacementCombo->addItem(a.displayName, a.key);
                layout->addWidget(replaceLabel);
                layout->addWidget(replacementCombo);
            } else if (isUsed) {
                layout->addWidget(new QLabel("No other attributes exist — references will be removed.", &confirmDlg));
            }

            auto* buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No, &confirmDlg);
            connect(buttons, &QDialogButtonBox::accepted, &confirmDlg, &QDialog::accept);
            connect(buttons, &QDialogButtonBox::rejected, &confirmDlg, &QDialog::reject);
            layout->addWidget(buttons);

            if (confirmDlg.exec() == QDialog::Accepted) {
                QString replacementKey = replacementCombo
                    ? replacementCombo->currentData().toString()
                    : QString();

                if (!replacementKey.isEmpty()) {
                    // Swap: replace old key with replacement key in effect modifiers
                    for (EffectTemplate* eff : DataPtr->allEffectTemplates())
                        for (Stage& s : eff->stages)
                            for (AttributeModifier& m : s.modifiers)
                                if (m.attributeKey == attributeKey)
                                    m.attributeKey = replacementKey;
                    // Swap: replace old key in pawn base attributes
                    for (PawnTemplate* pawn : DataPtr->allPawnTemplates())
                        for (AttributeValue& av : pawn->baseAttributes)
                            if (av.key == attributeKey)
                                av.key = replacementKey;
                    // Swap: replace old key in player live attributes
                    for (Player* player : PlayerRegistry::instance().allPlayers()) {
                        bool changed = false;
                        for (AttributeValue& av : player->attributes)
                            if (av.key == attributeKey) { av.key = replacementKey; changed = true; }
                        if (changed)
                            player->resolveAttributes();
                    }
                } else {
                    // No replacement — remove all references
                    for (EffectTemplate* eff : DataPtr->allEffectTemplates())
                        for (Stage& s : eff->stages)
                            s.modifiers.removeIf([&](const AttributeModifier& m){ return m.attributeKey == attributeKey; });
                    for (PawnTemplate* pawn : DataPtr->allPawnTemplates())
                        pawn->baseAttributes.removeIf([&](const AttributeValue& av){ return av.key == attributeKey; });
                    for (Player* player : PlayerRegistry::instance().allPlayers()) {
                        bool changed = false;
                        for (const AttributeValue& av : player->attributes)
                            if (av.key == attributeKey) { changed = true; break; }
                        player->attributes.removeIf([&](const AttributeValue& av){ return av.key == attributeKey; });
                        if (changed)
                            player->resolveAttributes();
                    }
                }

                PathTrackerStruct::attributes().removeAttribute(attributeKey);
                delete item;
                SaveSlot();
                RefreshDetailPanel();
            }
        });

        QAction *renameAction = menu.addAction("Rename Attribute");
        connect(renameAction, &QAction::triggered, [this, attributeKey]() {
            if (PathTrackerStruct::attributes().hasAttribute(attributeKey)) {
                Attribute attribute = PathTrackerStruct::attributes().getAttribute(attributeKey);
                ui->AttributeLine->setText(attribute.displayName);
                ui->ComittAttribute->setText("Edit Attribute");
                CurrentEditID = attributeKey;
                rename = true;
            }
        });
    }

    menu.exec(ui->AttributeList->mapToGlobal(pos));
}

void Editor::ShowPawnContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QListWidgetItem *item = ui->PawnList->itemAt(pos);

    if (item) {
        QString pawnID = item->data(Qt::UserRole).toString();

        QAction *removeAction = menu.addAction("Remove Pawn");
        connect(removeAction, &QAction::triggered, [this, item, pawnID]() {
            // Find encounter templates that reference this pawn
            QStringList usedByEncounters;
            for (EncounterTemplate* enc : DataPtr->allEncounterTemplates())
                for (const QString& id : enc->pawnTemplateIds)
                    if (id == pawnID && !usedByEncounters.contains(enc->name))
                        usedByEncounters.append(enc->name);

            QString msg = "Are you sure you want to remove this pawn?";
            if (!usedByEncounters.isEmpty()) {
                msg += QString("\n\nEncounters containing this pawn: %1").arg(usedByEncounters.join(", "));
                msg += "\n\nThe pawn will be removed from those encounters.";
            }

            QMessageBox::StandardButton reply =
                QMessageBox::question(this, "Confirm Removal", msg, QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                // Cascade: remove pawn from all encounter templates
                for (EncounterTemplate* enc : DataPtr->allEncounterTemplates())
                    enc->pawnTemplateIds.removeAll(pawnID);
                DataPtr->removePawnTemplate(pawnID);
                delete item;
                if (CurrentPawnID == pawnID) {
                    CurrentPawnID.clear();
                    ClearPanel(ui->PawnDetailsBody);
                }
                SaveSlot();
            }
        });

        QAction *renameAction = menu.addAction("Rename Pawn");
        connect(renameAction, &QAction::triggered, [this, pawnID]() {
            PawnTemplate* tmpl = DataPtr->findPawnTemplate(pawnID);
            if (tmpl) {
                ui->PawnLine->setText(tmpl->name);
                ui->PawnButton->setText("Edit Pawn");
                CurrentPawnID = pawnID;
                rename = true;
            }
        });
    }

    menu.exec(ui->PawnList->mapToGlobal(pos));
}

void Editor::ShowEncounterContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QListWidgetItem *item = ui->EncounterList->itemAt(pos);

    if (item) {
        QString encID = item->data(Qt::UserRole).toString();

        QAction *removeAction = menu.addAction("Remove Encounter");
        connect(removeAction, &QAction::triggered, [this, item, encID]() {
            QMessageBox::StandardButton reply =
                QMessageBox::question(this, "Confirm Action",
                                      "Are you sure you want to do this?",
                                      QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                DataPtr->removeEncounterTemplate(encID);
                delete item;
                if (CurrentEncounterID == encID) {
                    CurrentEncounterID.clear();
                    ClearPanel(ui->EncounterDetailsBody);
                }
                SaveSlot();
            }
        });

        QAction *renameAction = menu.addAction("Rename Encounter");
        connect(renameAction, &QAction::triggered, [this, encID]() {
            EncounterTemplate* tmpl = DataPtr->findEncounterTemplate(encID);
            if (tmpl) {
                ui->EncounterLine->setText(tmpl->name);
                ui->EncounterButton->setText("Edit Encounter");
                CurrentEncounterID = encID;
                rename = true;
            }
        });
    }

    menu.exec(ui->EncounterList->mapToGlobal(pos));
}

void Editor::ShowPlayerContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QListWidgetItem *item = ui->PlayerList->itemAt(pos);

    if (item) {
        QString playerID = item->data(Qt::UserRole).toString();

        QAction *removeAction = menu.addAction("Remove Player");
        connect(removeAction, &QAction::triggered, [this, item, playerID]() {
            // Block deletion if the player is currently in the active encounter —
            // EncounterInstance holds a raw non-owning pointer; destroying the
            // Player while it is borrowed would leave a dangling pointer.
            EncounterInstance* enc = DataPtr->activeEncounter();
            if (enc) {
                bool inEncounter = std::any_of(enc->activePlayers.begin(), enc->activePlayers.end(),
                    [&playerID](const Player* p) { return p->instanceId == playerID; });
                if (inEncounter) {
                    QMessageBox::warning(this, "Cannot Remove Player",
                        "This player is part of the active encounter.\n"
                        "End the encounter before removing them.");
                    return;
                }
            }

            QMessageBox::StandardButton reply =
                QMessageBox::question(this, "Confirm Action",
                                      "Are you sure you want to do this?",
                                      QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                PathTrackerStruct::players().removePlayer(playerID);
                delete item;
                if (CurrentPlayerID == playerID) {
                    CurrentPlayerID.clear();
                    ClearPanel(ui->PlayerDetailsBody);
                }
                SaveSlot();
            }
        });

        QAction *renameAction = menu.addAction("Rename Player");
        connect(renameAction, &QAction::triggered, [this, playerID]() {
            Player* player = PathTrackerStruct::players().findPlayer(playerID);
            if (player) {
                ui->PlayerLine->setText(player->playerName);
                ui->PlayerButton->setText("Edit Player");
                CurrentPlayerID = playerID;
                rename = true;
            }
        });
    }

    menu.exec(ui->PlayerList->mapToGlobal(pos));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Refresh
// ─────────────────────────────────────────────────────────────────────────────

void Editor::RequestRefresh()
{
    // Attributes – sorted by displayName
    QList<Attribute> attrs = PathTrackerStruct::attributes().allAttributes();
    std::sort(attrs.begin(), attrs.end(), [](const Attribute& a, const Attribute& b){
        return a.displayName < b.displayName;
    });

    ui->AttributeList->clear();
    for (const Attribute& attribute : attrs) {
        QListWidgetItem *widgetItem = new QListWidgetItem(attribute.displayName, ui->AttributeList);
        widgetItem->setData(Qt::UserRole, attribute.key);
    }

    // Effects – sorted by name
    QList<EffectTemplate*> effects = DataPtr->allEffectTemplates();
    std::sort(effects.begin(), effects.end(), [](const EffectTemplate* a, const EffectTemplate* b){
        return QString::localeAwareCompare(a->name, b->name) < 0;
    });

    ui->EffectList->clear();
    for (EffectTemplate* tmpl : effects) {
        QListWidgetItem *widgetItem = new QListWidgetItem(tmpl->name, ui->EffectList);
        widgetItem->setData(Qt::UserRole, tmpl->id);
    }

    // Pawns – sorted by name
    QList<PawnTemplate*> pawns = DataPtr->allPawnTemplates();
    std::sort(pawns.begin(), pawns.end(), [](const PawnTemplate* a, const PawnTemplate* b){
        return QString::localeAwareCompare(a->name, b->name) < 0;
    });

    ui->PawnList->clear();
    for (PawnTemplate* tmpl : pawns) {
        QListWidgetItem *widgetItem = new QListWidgetItem(tmpl->name, ui->PawnList);
        widgetItem->setData(Qt::UserRole, tmpl->id);
    }

    // Encounters – sorted by name
    QList<EncounterTemplate*> encounters = DataPtr->allEncounterTemplates();
    std::sort(encounters.begin(), encounters.end(), [](const EncounterTemplate* a, const EncounterTemplate* b){
        return QString::localeAwareCompare(a->name, b->name) < 0;
    });

    ui->EncounterList->clear();
    for (EncounterTemplate* tmpl : encounters) {
        QListWidgetItem *widgetItem = new QListWidgetItem(tmpl->name, ui->EncounterList);
        widgetItem->setData(Qt::UserRole, tmpl->id);
    }

    // Players – sorted by playerName
    QList<Player*> players = PathTrackerStruct::players().allPlayers();
    std::sort(players.begin(), players.end(), [](const Player* a, const Player* b){
        return QString::localeAwareCompare(a->playerName, b->playerName) < 0;
    });

    ui->PlayerList->clear();
    for (Player* p : players) {
        QListWidgetItem *widgetItem = new QListWidgetItem(p->playerName, ui->PlayerList);
        widgetItem->setData(Qt::UserRole, p->instanceId);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Attribute CRUD
// ─────────────────────────────────────────────────────────────────────────────

void Editor::on_ComittAttribute_clicked()
{
    QString nameText = ui->AttributeLine->text();
    if (nameText.isEmpty()) return;

    QStringList attributeNames;
    for (const Attribute& attr : PathTrackerStruct::attributes().allAttributes())
        if (attr.key != CurrentEditID) attributeNames << attr.displayName;

    if (!CurrentEditID.isEmpty() && rename) {
        if (PathTrackerStruct::attributes().hasAttribute(CurrentEditID)) {
            Attribute attribute = PathTrackerStruct::attributes().getAttribute(CurrentEditID);
            attribute.displayName = UniqueName(nameText, attributeNames);
            PathTrackerStruct::attributes().registerAttribute(attribute);
        }
        rename = false;
    } else {
        Attribute attribute;
        attribute.key          = QUuid::createUuid().toString(QUuid::WithoutBraces);
        attribute.displayName  = UniqueName(nameText, attributeNames);
        attribute.defaultValue = 0;
        PathTrackerStruct::attributes().registerAttribute(attribute);
    }

    SaveSlot();
    RequestRefresh();

    CurrentEditID.clear();
    ui->AttributeLine->clear();
    ui->ComittAttribute->setText("Add Attribute");
}

void Editor::on_AttributeList_itemClicked(QListWidgetItem *item)
{
    if (!rename)
        CurrentEditID = item->data(Qt::UserRole).toString();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Effect CRUD
// ─────────────────────────────────────────────────────────────────────────────

void Editor::on_EffectButton_clicked()
{
    QString nameText = ui->EffectLine->text();
    if (nameText.isEmpty()) return;

    QStringList effectNames;
    for (EffectTemplate* tmpl : DataPtr->allEffectTemplates())
        if (tmpl->id != CurrentEffectID) effectNames << tmpl->name;

    if (!CurrentEffectID.isEmpty() && rename) {
        EffectTemplate* tmpl = DataPtr->findEffectTemplate(CurrentEffectID);
        if (tmpl) {
            tmpl->name = UniqueName(nameText, effectNames);
            rename = false;
        }
    } else {
        auto newEffect   = std::make_unique<EffectTemplate>();
        newEffect->id    = QUuid::createUuid().toString(QUuid::WithoutBraces);
        newEffect->name  = UniqueName(nameText, effectNames);
        DataPtr->addEffectTemplate(std::move(newEffect));
    }

    SaveSlot();
    RequestRefresh();

    CurrentEffectID.clear();
    ui->EffectLine->clear();
    ui->EffectButton->setText("Add Effect");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pawn CRUD
// ─────────────────────────────────────────────────────────────────────────────

void Editor::on_PawnButton_clicked()
{
    QString nameText = ui->PawnLine->text();
    if (nameText.isEmpty()) return;

    QStringList pawnNames;
    for (PawnTemplate* tmpl : DataPtr->allPawnTemplates())
        if (tmpl->id != CurrentPawnID) pawnNames << tmpl->name;

    if (!CurrentPawnID.isEmpty() && rename) {
        PawnTemplate* tmpl = DataPtr->findPawnTemplate(CurrentPawnID);
        if (tmpl) {
            tmpl->name = UniqueName(nameText, pawnNames);
            rename = false;
        }
    } else {
        auto newPawn  = std::make_unique<PawnTemplate>();
        newPawn->id   = QUuid::createUuid().toString(QUuid::WithoutBraces);
        newPawn->name = UniqueName(nameText, pawnNames);
        DataPtr->addPawnTemplate(std::move(newPawn));
    }

    SaveSlot();
    RequestRefresh();

    CurrentPawnID.clear();
    ui->PawnLine->clear();
    ui->PawnButton->setText("Add Pawn");
}

void Editor::on_PawnList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!current) return;
    if (!rename) {
        CurrentPawnID = current->data(Qt::UserRole).toString();
        RefreshPawnPanel();
    }
}

void Editor::on_PawnLine_textChanged(const QString &arg1)
{
    if (!CurrentPawnID.isEmpty() && arg1.isEmpty()) {
        CurrentPawnID.clear();
        ui->PawnButton->setText("Add Pawn");
    }
}

void Editor::RefreshPawnPanel()
{
    ClearPanel(ui->PawnDetailsBody);
    PawnTemplate* pawn = DataPtr->findPawnTemplate(CurrentPawnID);
    if (!pawn) return;
    auto* widget = AddToPanel<editor_PawnTemplate>(ui->PawnDetailsBody, DataPtr, pawn);
    connect(widget, SIGNAL(Save()), this, SLOT(SaveSlot()));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Encounter CRUD
// ─────────────────────────────────────────────────────────────────────────────

void Editor::on_EncounterButton_clicked()
{
    QString nameText = ui->EncounterLine->text();
    if (nameText.isEmpty()) return;

    QStringList encounterNames;
    for (EncounterTemplate* tmpl : DataPtr->allEncounterTemplates())
        if (tmpl->id != CurrentEncounterID) encounterNames << tmpl->name;

    if (!CurrentEncounterID.isEmpty() && rename) {
        EncounterTemplate* tmpl = DataPtr->findEncounterTemplate(CurrentEncounterID);
        if (tmpl) {
            tmpl->name = UniqueName(nameText, encounterNames);
            rename = false;
        }
    } else {
        auto newEnc  = std::make_unique<EncounterTemplate>();
        newEnc->id   = QUuid::createUuid().toString(QUuid::WithoutBraces);
        newEnc->name = UniqueName(nameText, encounterNames);
        DataPtr->addEncounterTemplate(std::move(newEnc));
    }

    SaveSlot();
    RequestRefresh();

    CurrentEncounterID.clear();
    ui->EncounterLine->clear();
    ui->EncounterButton->setText("Add Encounter");
}

void Editor::on_EncounterList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!current) return;
    if (!rename) {
        CurrentEncounterID = current->data(Qt::UserRole).toString();
        RefreshEncounterPanel();
    }
}

void Editor::on_EncounterLine_textChanged(const QString &arg1)
{
    if (!CurrentEncounterID.isEmpty() && arg1.isEmpty()) {
        CurrentEncounterID.clear();
        ui->EncounterButton->setText("Add Encounter");
    }
}

void Editor::RefreshEncounterPanel()
{
    ClearPanel(ui->EncounterDetailsBody);
    EncounterTemplate* enc = DataPtr->findEncounterTemplate(CurrentEncounterID);
    if (!enc) return;
    auto* widget = AddToPanel<editor_EncounterTemplate>(ui->EncounterDetailsBody, DataPtr, enc);
    connect(widget, SIGNAL(Save()), this, SLOT(SaveSlot()));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Player CRUD
// ─────────────────────────────────────────────────────────────────────────────

void Editor::on_PlayerButton_clicked()
{
    QString nameText = ui->PlayerLine->text();
    if (nameText.isEmpty()) return;

    QStringList playerNames;
    for (Player* player : PathTrackerStruct::players().allPlayers())
        if (player->instanceId != CurrentPlayerID) playerNames << player->playerName;

    if (!CurrentPlayerID.isEmpty() && rename) {
        Player* player = PathTrackerStruct::players().findPlayer(CurrentPlayerID);
        if (player) {
            const QString uniqueName   = UniqueName(nameText, playerNames);
            player->playerName  = uniqueName;
            player->displayName = uniqueName;
            rename = false;
        }
    } else {
        Player* newPlayer = Player::create(UniqueName(nameText, playerNames));
        PathTrackerStruct::players().addPlayer(newPlayer);
    }

    SaveSlot();
    RequestRefresh();

    CurrentPlayerID.clear();
    ui->PlayerLine->clear();
    ui->PlayerButton->setText("Add Player");
}

void Editor::on_PlayerList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!current) return;
    if (!rename) {
        CurrentPlayerID = current->data(Qt::UserRole).toString();
        RefreshPlayerPanel();
    }
}

void Editor::on_PlayerLine_textChanged(const QString &arg1)
{
    if (!CurrentPlayerID.isEmpty() && arg1.isEmpty()) {
        CurrentPlayerID.clear();
        ui->PlayerButton->setText("Add Player");
    }
}

void Editor::RefreshPlayerPanel()
{
    ClearPanel(ui->PlayerDetailsBody);
    Player* player = PathTrackerStruct::players().findPlayer(CurrentPlayerID);
    if (!player) return;

    // If this player is borrowed by the active encounter, show a warning and
    // disable editing — mutating a borrowed Player mid-combat violates the
    // template/instance duality and produces inconsistent combat state.
    EncounterInstance* enc = DataPtr->activeEncounter();
    bool inEncounter = enc && std::any_of(enc->activePlayers.begin(), enc->activePlayers.end(),
        [&](const Player* p) { return p->instanceId == CurrentPlayerID; });

    if (inEncounter) {
        auto* warning = new QLabel(
            "This player is part of the active encounter.\n"
            "Editing is disabled until the encounter ends.",
            ui->PlayerDetailsBody);
        warning->setWordWrap(true);
        warning->setStyleSheet("color: orange; font-style: italic;");
        if (!ui->PlayerDetailsBody->layout())
            ui->PlayerDetailsBody->setLayout(new QVBoxLayout(ui->PlayerDetailsBody));
        ui->PlayerDetailsBody->layout()->addWidget(warning);
    }

    auto* widget = AddToPanel<editor_Player>(ui->PlayerDetailsBody, player);
    connect(widget, SIGNAL(Save()), this, SLOT(SaveSlot()));

    if (inEncounter)
        widget->setEnabled(false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Detail panel helpers
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename... Args>
T* Editor::AddToDetailPanel(Args&&... args) {
    QWidget* Container = ui->EffectDetailsBody;
    if (!Container) return nullptr;

    T* NewWidget = new T(std::forward<Args>(args)..., Container);

    if (!Container->layout())
        Container->setLayout(new QVBoxLayout(Container));

    Container->layout()->addWidget(NewWidget);
    return NewWidget;
}

template <typename T, typename... Args>
T* Editor::AddToPanel(QWidget* body, Args&&... args) {
    if (!body) return nullptr;

    T* NewWidget = new T(std::forward<Args>(args)..., body);

    if (!body->layout())
        body->setLayout(new QVBoxLayout(body));

    body->layout()->addWidget(NewWidget);
    return NewWidget;
}

void Editor::ClearDetailsPanel()
{
    QLayout* layout = ui->EffectDetailsBody->layout();
    if (!layout) return;

    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }
}

void Editor::ClearPanel(QWidget* body)
{
    if (!body) return;
    QLayout* layout = body->layout();
    if (!layout) return;

    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }
}

void Editor::RefreshDetailPanel()
{
    ClearDetailsPanel();

    EffectTemplate* currentEffect = DataPtr->findEffectTemplate(CurrentEffectID);
    if (!currentEffect)
        return;

    auto* header = AddToDetailPanel<editor_EffectHeader>(currentEffect);
    connect(header, SIGNAL(Save()), this, SLOT(SaveSlot()));

    auto* stages = AddToDetailPanel<editor_Stages>(DataPtr, currentEffect);
    connect(stages, SIGNAL(Save()), this, SLOT(SaveSlot()));
}

void Editor::on_EffectLine_textChanged(const QString &arg1)
{
    if (!CurrentEffectID.isEmpty() && arg1.isEmpty()) {
        CurrentEffectID.clear();
        ui->EffectButton->setText("Add Effect");
    }
}

void Editor::on_EffectList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!current)
        return;

    if (!rename) {
        CurrentEffectID = current->data(Qt::UserRole).toString();
        RefreshDetailPanel();
    }
}

void Editor::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index)
    if (rename) {
        rename = false;
        ui->EffectLine->setText("");
        ui->EffectButton->setText("Add Effect");
        ui->AttributeLine->setText("");
        ui->ComittAttribute->setText("Add Attribute");
        ui->PawnLine->setText("");
        ui->PawnButton->setText("Add Pawn");
        ui->EncounterLine->setText("");
        ui->EncounterButton->setText("Add Encounter");
        ui->PlayerLine->setText("");
        ui->PlayerButton->setText("Add Player");
    }
}
