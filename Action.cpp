#include "Action.h"
#include "Effect.h"    // EffectInstance – needed for level/stage mutation
#include "Pawn.h"      // PawnInstance   – needed for attribute mutation
#include "DiceRoller.h"
#include "PathTrackerStruct.h"

#include <QGraphicsOpacityEffect>
#include <QSizePolicy>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QJsonObject>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: retrieve the EffectInstance that owns a given Action.
//  Actions are stored inside Stage::actions, which lives in EffectInstance::stages.
//  The PawnInstance holds the EffectInstances.
//  We pass context through executeEvent via the PawnInstance pointer; the
//  Action needs to reach back to the EffectInstance. We do this by iterating
//  the pawn's effects. This is a deliberate O(n) walk – n is always tiny.
// ─────────────────────────────────────────────────────────────────────────────
static EffectInstance* findEffectOwningAction(PawnInstance* pawn, Action* action)
{
    for (auto& effectInstance : pawn->effects)
    {
        for (Stage& stage : effectInstance->stages)
        {
            for (auto& stagedAction : stage.actions)
            {
                if (stagedAction.get() == action)
                    return effectInstance.get();
            }
        }
    }
    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. LevelModifierAction
// ═══════════════════════════════════════════════════════════════════════════════

QWidget* LevelModifierAction::createEditorWidget(QWidget* parent, std::function<void()> onChanged)
{
    auto* box    = new QGroupBox("Level Modifier", parent);
    auto* layout = new QHBoxLayout(box);

    auto* group         = new QButtonGroup(box);
    auto* increaseButton = new QRadioButton("Increase", box);
    auto* decreaseButton = new QRadioButton("Decrease", box);
    group->addButton(increaseButton, 0);
    group->addButton(decreaseButton, 1);

    (direction == Direction::Increase ? increaseButton : decreaseButton)->setChecked(true);

    layout->addWidget(increaseButton);
    layout->addWidget(decreaseButton);

    QObject::connect(group, &QButtonGroup::idClicked, box, [this, onChanged](int id){
        direction = (id == 0) ? Direction::Increase : Direction::Decrease;
        if (onChanged) onChanged();
    });

    return box;
}

QWidget* LevelModifierAction::createRuntimeWidget(QWidget* parent,
                                                    PawnInstance* /*target*/,
                                                    std::function<void()> /*onMutated*/)
{
    const QString label = (direction == Direction::Increase)
        ? "Level will Increase next Round"
        : "Level will Decrease next Round";
    auto* btn = new QPushButton(label, parent);
    btn->setEnabled(false);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return btn;
}

void LevelModifierAction::executeEvent(EventType event, PawnInstance* pawn)
{
    if (event != EventType::OnRoundEnd) return;

    EffectInstance* effectInstance = findEffectOwningAction(pawn, this);
    if (!effectInstance) return;

    if (direction == Direction::Increase)
        effectInstance->level++;
    else
        effectInstance->level = qMax(0, effectInstance->level - 1);
}

QJsonObject LevelModifierAction::saveToJson() const
{
    QJsonObject obj;
    obj["type"]      = typeName();
    obj["direction"] = (direction == Direction::Increase) ? "Increase" : "Decrease";
    return obj;
}

void LevelModifierAction::loadFromJson(const QJsonObject& obj)
{
    direction = (obj["direction"].toString() == "Increase")
                ? Direction::Increase : Direction::Decrease;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. StageModifierAction
// ═══════════════════════════════════════════════════════════════════════════════

QWidget* StageModifierAction::createEditorWidget(QWidget* parent, std::function<void()> onChanged)
{
    auto* box    = new QGroupBox("Stage Modifier", parent);
    auto* layout = new QHBoxLayout(box);

    auto* group         = new QButtonGroup(box);
    auto* increaseButton = new QRadioButton("Advance Stage", box);
    auto* decreaseButton = new QRadioButton("Regress Stage", box);
    group->addButton(increaseButton, 0);
    group->addButton(decreaseButton, 1);

    (direction == Direction::Increase ? increaseButton : decreaseButton)->setChecked(true);

    layout->addWidget(increaseButton);
    layout->addWidget(decreaseButton);

    QObject::connect(group, &QButtonGroup::idClicked, box, [this, onChanged](int id){
        direction = (id == 0) ? Direction::Increase : Direction::Decrease;
        if (onChanged) onChanged();
    });

    return box;
}

QWidget* StageModifierAction::createRuntimeWidget(QWidget* parent,
                                                    PawnInstance* /*target*/,
                                                    std::function<void()> /*onMutated*/)
{
    const QString label = (direction == Direction::Increase)
        ? "Stage will Advance next Round"
        : "Stage will Regress next Round";
    auto* btn = new QPushButton(label, parent);
    btn->setEnabled(false);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return btn;
}

void StageModifierAction::executeEvent(EventType event, PawnInstance* pawn)
{
    if (event != EventType::OnRoundEnd) return;

    EffectInstance* effectInstance = findEffectOwningAction(pawn, this);
    if (!effectInstance) return;

    if (direction == Direction::Increase)
        effectInstance->advanceStage();
    else
        effectInstance->regressStage();

    pawn->resolveAttributes();
}

QJsonObject StageModifierAction::saveToJson() const
{
    QJsonObject obj;
    obj["type"]      = typeName();
    obj["direction"] = (direction == Direction::Increase) ? "Increase" : "Decrease";
    return obj;
}

void StageModifierAction::loadFromJson(const QJsonObject& obj)
{
    direction = (obj["direction"].toString() == "Increase")
                ? Direction::Increase : Direction::Decrease;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. LevelStageModifierAction  (Saving Throw)
// ═══════════════════════════════════════════════════════════════════════════════

QWidget* LevelStageModifierAction::createEditorWidget(QWidget* parent, std::function<void()> onChanged)
{
    auto* box    = new QGroupBox("Saving Throw Modifier", parent);
    auto* layout = new QVBoxLayout(box);

    // Target: Level or Stage
    auto* targetLayout = new QHBoxLayout;
    auto* targetGroup  = new QButtonGroup(box);
    auto* levelBtn     = new QRadioButton("Level",  box);
    auto* stageBtn     = new QRadioButton("Stage",  box);
    targetGroup->addButton(levelBtn, 0);
    targetGroup->addButton(stageBtn, 1);
    (target == Target::Level ? levelBtn : stageBtn)->setChecked(true);
    targetLayout->addWidget(new QLabel("Target:", box));
    targetLayout->addWidget(levelBtn);
    targetLayout->addWidget(stageBtn);
    layout->addLayout(targetLayout);

    // Direction
    auto* dirLayout      = new QHBoxLayout;
    auto* dirGroup       = new QButtonGroup(box);
    auto* increaseButton = new QRadioButton("Increase", box);
    auto* decreaseButton = new QRadioButton("Decrease", box);
    dirGroup->addButton(increaseButton, 0);
    dirGroup->addButton(decreaseButton, 1);
    (direction == Direction::Increase ? increaseButton : decreaseButton)->setChecked(true);
    dirLayout->addWidget(new QLabel("Direction:", box));
    dirLayout->addWidget(increaseButton);
    dirLayout->addWidget(decreaseButton);
    layout->addLayout(dirLayout);

    // Dice formula
    auto* diceLayout = new QHBoxLayout;
    diceLayout->addWidget(new QLabel("Dice:", box));
    auto* diceEdit = new QLineEdit(diceFormula, box);
    diceEdit->setPlaceholderText("e.g. 1D20+4");
    diceLayout->addWidget(diceEdit);
    layout->addLayout(diceLayout);

    // Connections
    QObject::connect(targetGroup, &QButtonGroup::idClicked, box, [this, onChanged](int id){
        target = (id == 0) ? Target::Level : Target::Stage;
        if (onChanged) onChanged();
    });
    QObject::connect(dirGroup, &QButtonGroup::idClicked, box, [this, onChanged](int id){
        direction = (id == 0) ? Direction::Increase : Direction::Decrease;
        if (onChanged) onChanged();
    });
    QObject::connect(diceEdit, &QLineEdit::textChanged, box, [this, onChanged](const QString& text){
        diceFormula = text;
        if (onChanged) onChanged();
    });

    return box;
}

QWidget* LevelStageModifierAction::createRuntimeWidget(QWidget* parent,
                                                        PawnInstance* /*target*/,
                                                        std::function<void()> /*onMutated*/)
{
    auto* container = new QWidget(parent);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* layout    = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->setSizeConstraint(QLayout::SetMinimumSize);

    const QString rollLabel = QString("[Save vs %1]  %2  %3")
        .arg(diceFormula)
        .arg(target == Target::Level ? "Level" : "Stage")
        .arg(direction == Direction::Increase ? "▲" : "▼");

    auto* rollBtn         = new QPushButton(rollLabel, container);
    auto* resultLabel     = new QLabel(container);
    auto* succeededButton = new QPushButton("Succeeded", container);
    auto* failedButton    = new QPushButton("Failed",    container);
    auto* btnRow          = new QHBoxLayout;
    btnRow->addWidget(succeededButton);
    btnRow->addWidget(failedButton);

    layout->addWidget(rollBtn);
    layout->addWidget(resultLabel);
    layout->addLayout(btnRow);

    // Buttons are always visible so the list item measures the correct height upfront.
    // We toggle enabled/disabled instead of show/hide.
    succeededButton->setEnabled(false);
    failedButton->setEnabled(false);

    if (performed) {
        rollBtn->setEnabled(false);
        resultLabel->setText(succeeded ? "Result: Succeeded" : "Result: Failed");
        auto* fx = new QGraphicsOpacityEffect(container);
        fx->setOpacity(0.4);
        container->setGraphicsEffect(fx);
        container->setEnabled(false);
    }

    QObject::connect(rollBtn, &QPushButton::clicked, container, [this, rollBtn, resultLabel, succeededButton, failedButton](){
        auto* popup = new DiceRollerPopup(diceFormula, rollBtn);
        if (popup->exec() == QDialog::Accepted) {
            resultLabel->setText(QString("Result: %1").arg(popup->lastResult()));
            succeededButton->setEnabled(true);
            failedButton->setEnabled(true);
            rollBtn->setEnabled(false);
        }
        popup->deleteLater();
    });

    auto onOutcome = [this, container, succeededButton, failedButton](bool didSucceed) {
        performed = true;
        succeeded = didSucceed;
        succeededButton->setEnabled(false);
        failedButton->setEnabled(false);
        auto* fx = new QGraphicsOpacityEffect(container);
        fx->setOpacity(0.4);
        container->setGraphicsEffect(fx);
        container->setEnabled(false);
        PathTrackerStruct::instance().save();
    };

    QObject::connect(succeededButton, &QPushButton::clicked, container, [onOutcome](){ onOutcome(true);  });
    QObject::connect(failedButton,    &QPushButton::clicked, container, [onOutcome](){ onOutcome(false); });

    return container;
}

void LevelStageModifierAction::executeEvent(EventType event, PawnInstance* pawn)
{
    // Reset at the start of each round so the widget rebuilds as fresh.
    if (event == EventType::OnRoundStart) {
        performed = false;
        succeeded = false;
        return;
    }

    // This action is DM-driven (button click), not auto-fired per round.
    if (event != EventType::OnRoundEnd) return;

    EffectInstance* effectInstance = findEffectOwningAction(pawn, this);
    if (!effectInstance) return;

    const bool increase = (direction == Direction::Increase);

    if (target == Target::Level)
        effectInstance->level = increase ? effectInstance->level + 1 : qMax(0, effectInstance->level - 1);
    else
        increase ? effectInstance->advanceStage() : effectInstance->regressStage();

    pawn->resolveAttributes();
}

QJsonObject LevelStageModifierAction::saveToJson() const
{
    QJsonObject obj;
    obj["type"]        = typeName();
    obj["target"]      = (target == Target::Level) ? "Level" : "Stage";
    obj["direction"]   = (direction == Direction::Increase) ? "Increase" : "Decrease";
    obj["diceFormula"] = diceFormula;
    obj["performed"]   = performed;
    obj["succeeded"]   = succeeded;
    return obj;
}

void LevelStageModifierAction::loadFromJson(const QJsonObject& obj)
{
    target      = (obj["target"].toString() == "Level") ? Target::Level : Target::Stage;
    direction   = (obj["direction"].toString() == "Increase") ? Direction::Increase : Direction::Decrease;
    diceFormula = obj["diceFormula"].toString();
    performed   = obj["performed"].toBool(false);
    succeeded   = obj["succeeded"].toBool(false);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  4. DamageThrowAction
// ═══════════════════════════════════════════════════════════════════════════════

QWidget* DamageThrowAction::createEditorWidget(QWidget* parent, std::function<void()> onChanged)
{
    auto* box    = new QGroupBox("Damage Throw", parent);
    auto* layout = new QVBoxLayout(box);

    // Damage type combo
    auto* typeLayout = new QHBoxLayout;
    typeLayout->addWidget(new QLabel("Type:", box));
    auto* typeCombo = new QComboBox(box);
    const QStringList types = { "Acid", "Bludgeoning", "Cold", "Fire",
                                "Force", "Lightning", "Necrotic", "Piercing",
                                "Poison", "Psychic", "Radiant", "Slashing", "Thunder" };
    typeCombo->addItems(types);
    const int foundIndex = types.indexOf(damageType);
    if (foundIndex >= 0) typeCombo->setCurrentIndex(foundIndex);
    typeLayout->addWidget(typeCombo);
    layout->addLayout(typeLayout);

    // Dice formula
    auto* diceLayout = new QHBoxLayout;
    diceLayout->addWidget(new QLabel("Dice:", box));
    auto* diceEdit = new QLineEdit(diceFormula, box);
    diceEdit->setPlaceholderText("e.g. 8D6");
    diceLayout->addWidget(diceEdit);
    layout->addLayout(diceLayout);

    // Connections
    QObject::connect(typeCombo, &QComboBox::currentTextChanged, box, [this, onChanged](const QString& text){
        damageType = text;
        if (onChanged) onChanged();
    });
    QObject::connect(diceEdit, &QLineEdit::textChanged, box, [this, onChanged](const QString& text){
        diceFormula = text;
        if (onChanged) onChanged();
    });

    return box;
}

QWidget* DamageThrowAction::createRuntimeWidget(QWidget* parent,
                                                  PawnInstance* target,
                                                  std::function<void()> onMutated)
{
    auto* container = new QWidget(parent);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* layout    = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->setSizeConstraint(QLayout::SetMinimumSize);

    const QString label = QString("Damage: %1 (%2)").arg(diceFormula, damageType);
    auto* rollBtn     = new QPushButton(label, container);
    auto* resultLabel = new QLabel(container);
    resultLabel->hide();

    layout->addWidget(rollBtn);
    layout->addWidget(resultLabel);

    if (performed) {
        rollBtn->setEnabled(false);
        resultLabel->setText(QString("Result: %1").arg(lastResult));
        resultLabel->show();
        auto* fx = new QGraphicsOpacityEffect(container);
        fx->setOpacity(0.4);
        container->setGraphicsEffect(fx);
        container->setEnabled(false);
    }

    QObject::connect(rollBtn, &QPushButton::clicked, container, [this, container, rollBtn, resultLabel, target, onMutated](){
        auto* popup = new DiceRollerPopup(diceFormula, rollBtn);
        if (popup->exec() == QDialog::Accepted) {
            lastResult = popup->lastResult();
            performed  = true;
            resultLabel->setText(QString("Result: %1").arg(lastResult));
            resultLabel->show();
            rollBtn->setEnabled(false);
            // Subtract damage from target's HP
            if (target) {
                int current = target->getAttribute(HP_KEY);
                target->setAttribute(HP_KEY, current - lastResult);
                target->resolveAttributes();
                PathTrackerStruct::instance().save();
                if (onMutated) onMutated();
            } else {
                PathTrackerStruct::instance().save();
            }
            // Grey out to indicate this action is done
            auto* fx = new QGraphicsOpacityEffect(container);
            fx->setOpacity(0.4);
            container->setGraphicsEffect(fx);
            container->setEnabled(false);
        }
        popup->deleteLater();
    });

    return container;
}

void DamageThrowAction::executeEvent(EventType event, PawnInstance* /*pawn*/)
{
    // Reset at the start of each round so the button becomes active again.
    if (event == EventType::OnRoundStart) {
        performed  = false;
        lastResult = 0;
    }
    // DamageThrowAction is manual-only: damage is applied exclusively via the
    // runtime button. No automatic firing on OnRoundEnd or any other event.
}

QJsonObject DamageThrowAction::saveToJson() const
{
    QJsonObject obj;
    obj["type"]        = typeName();
    obj["damageType"]  = damageType;
    obj["diceFormula"] = diceFormula;
    obj["performed"]   = performed;
    obj["lastResult"]  = lastResult;
    return obj;
}

void DamageThrowAction::loadFromJson(const QJsonObject& obj)
{
    damageType  = obj["damageType"].toString();
    diceFormula = obj["diceFormula"].toString();
    performed   = obj["performed"].toBool(false);
    lastResult  = obj["lastResult"].toInt(0);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. MaxDurationAction – time helpers
// ═══════════════════════════════════════════════════════════════════════════════
//  Pathfinder time units: 1 round = 6 sec, 1 min = 10 rounds, 1 hr = 60 min = 600 rounds

enum UnitIndex { UNIT_SECONDS = 0, UNIT_ROUNDS = 1, UNIT_MINUTES = 2, UNIT_HOURS = 3 };
static const char* const kUnitLabels[] = { "Seconds", "Rounds", "Minutes", "Hours" };

static double roundsToUnit(int rounds, int unit)
{
    switch (unit) {
        case UNIT_SECONDS: return rounds * 6.0;
        case UNIT_ROUNDS:  return double(rounds);
        case UNIT_MINUTES: return rounds / 10.0;
        case UNIT_HOURS:   return rounds / 600.0;
    }
    return double(rounds);
}

static int unitToRounds(double value, int unit)
{
    double r = 0;
    switch (unit) {
        case UNIT_SECONDS: r = value / 6.0;   break;
        case UNIT_ROUNDS:  r = value;          break;
        case UNIT_MINUTES: r = value * 10.0;  break;
        case UNIT_HOURS:   r = value * 600.0; break;
    }
    return qMax(1, qRound(r));
}

// Returns e.g. "20 rounds (2 min)" – always shows rounds + best human unit.
static QString formatDuration(int rounds)
{
    if (rounds < 10)
        return QString("%1 rounds (%2 sec)").arg(rounds).arg(rounds * 6);
    if (rounds < 600) {
        double min = rounds / 10.0;
        return QString("%1 rounds (%2 min)").arg(rounds)
                   .arg(min, 0, 'f', rounds % 10 == 0 ? 0 : 1);
    }
    double hr = rounds / 600.0;
    return QString("%1 rounds (%2 hr)").arg(rounds)
               .arg(hr, 0, 'f', rounds % 600 == 0 ? 0 : 2);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  5. MaxDurationAction
// ═══════════════════════════════════════════════════════════════════════════════

QList<QPair<QString,QString>> MaxDurationAction::getDisplayProperties() const
{
    return {{ "Max Duration",
              QString("%1 / %2").arg(roundsRemaining).arg(formatDuration(maxRounds)) }};
}

Action* MaxDurationAction::clone() const
{
    auto* copy = new MaxDurationAction(*this);
    copy->roundsRemaining = copy->maxRounds;   // reset runtime counter on clone
    return copy;
}

QWidget* MaxDurationAction::createEditorWidget(QWidget* parent, std::function<void()> onChanged)
{
    auto* box    = new QGroupBox("Max Duration", parent);
    auto* layout = new QHBoxLayout(box);

    auto* spin    = new QDoubleSpinBox(box);
    auto* unitBtn = new QPushButton(kUnitLabels[UNIT_ROUNDS], box);

    spin->setDecimals(0);
    spin->setRange(1.0, 99999.0);
    spin->setValue(double(maxRounds));   // start in Rounds

    unitBtn->setProperty("unitIndex", int(UNIT_ROUNDS));
    unitBtn->setToolTip("Click to cycle units: Seconds → Rounds → Minutes → Hours");

    layout->addWidget(spin);
    layout->addWidget(unitBtn);

    // Spin changed → convert displayed value back to rounds and store
    QObject::connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged), box,
        [this, unitBtn, onChanged](double v) {
            int unit  = unitBtn->property("unitIndex").toInt();
            maxRounds = unitToRounds(v, unit);
            if (onChanged) onChanged();
        });

    // Unit button clicked → cycle unit and re-display the stored maxRounds
    QObject::connect(unitBtn, &QPushButton::clicked, box,
        [this, spin, unitBtn]() {
            int nextUnit = (unitBtn->property("unitIndex").toInt() + 1) % 4;
            unitBtn->setProperty("unitIndex", nextUnit);
            unitBtn->setText(kUnitLabels[nextUnit]);

            // Adjust decimals and range for the new unit
            switch (nextUnit) {
                case UNIT_SECONDS: spin->setDecimals(0); spin->setRange(1.0,   599940.0); break;
                case UNIT_ROUNDS:  spin->setDecimals(0); spin->setRange(1.0,    99999.0); break;
                case UNIT_MINUTES: spin->setDecimals(1); spin->setRange(0.1,    9999.9);  break;
                case UNIT_HOURS:   spin->setDecimals(2); spin->setRange(0.01,    166.0);  break;
            }

            // Update display without triggering valueChanged → store loop
            spin->blockSignals(true);
            spin->setValue(roundsToUnit(maxRounds, nextUnit));
            spin->blockSignals(false);
        });

    return box;
}

QWidget* MaxDurationAction::createRuntimeWidget(QWidget* parent,
                                                  PawnInstance* /*target*/,
                                                  std::function<void()> /*onMutated*/)
{
    const QString label = QString("Remaining: %1 / %2")
        .arg(roundsRemaining)
        .arg(formatDuration(maxRounds));
    auto* btn = new QPushButton(label, parent);
    btn->setEnabled(false);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return btn;
}

void MaxDurationAction::executeEvent(EventType event, PawnInstance* pawn)
{
    if (event != EventType::OnRoundEnd) return;

    roundsRemaining = qMax(0, roundsRemaining - 1);

    if (roundsRemaining == 0)
    {
        EffectInstance* effectInstance = findEffectOwningAction(pawn, this);
        if (effectInstance)
            pawn->removeEffect(effectInstance->instanceId);
    }
}

QJsonObject MaxDurationAction::saveToJson() const
{
    QJsonObject obj;
    obj["type"]            = typeName();
    obj["maxRounds"]       = maxRounds;
    obj["roundsRemaining"] = roundsRemaining;
    return obj;
}

void MaxDurationAction::loadFromJson(const QJsonObject& obj)
{
    maxRounds       = obj["maxRounds"].toInt(1);
    roundsRemaining = obj["roundsRemaining"].toInt(maxRounds);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  ActionFactory
// ═══════════════════════════════════════════════════════════════════════════════

std::shared_ptr<Action> ActionFactory::create(const QString& typeName)
{
    if (typeName == "LevelModifier")      return std::make_shared<LevelModifierAction>();
    if (typeName == "StageModifier")      return std::make_shared<StageModifierAction>();
    if (typeName == "LevelStageModifier") return std::make_shared<LevelStageModifierAction>();
    if (typeName == "DamageThrow")        return std::make_shared<DamageThrowAction>();
    if (typeName == "MaxDuration")        return std::make_shared<MaxDurationAction>();
    return nullptr;
}

QMap<QString, QString> ActionFactory::registeredTypes()
{
    return {
        { "LevelModifier",      "Change Level" },
        { "StageModifier",      "Change Stage" },
        { "LevelStageModifier", "Saving Throw (Level/Stage)" },
        { "DamageThrow",        "Damage Throw" },
        { "MaxDuration",        "Max Duration (Rounds)" }
    };
}
