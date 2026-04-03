#pragma once
#include "Attribute.h"
#include <QString>
#include <QJsonObject>
#include <QWidget>
#include <functional>

class PawnInstance;   // forward declaration

// ─────────────────────────────────────────────
//  EventType  – hooks fired by the Encounter loop
// ─────────────────────────────────────────────
enum class EventType
{
    OnRoundStart,
    OnRoundEnd,
    OnTurnStart,
    OnTurnEnd,
    OnApplied,
    OnRemoved
};

// ─────────────────────────────────────────────
//  Action  (Abstract Base)
//  Attached to a Stage. Copied from EffectTemplate
//  to EffectInstance verbatim; manages its own
//  runtime state internally.
// ─────────────────────────────────────────────
class Action
{
public:
    virtual ~Action() = default;

    // Factory helper – returns a heap-allocated copy of this action
    virtual Action* clone() const = 0;

    // Editor UI
    virtual QWidget* createEditorWidget(QWidget* parent = nullptr,
                                        std::function<void()> onChanged = nullptr) = 0;

    // Runtime UI (may return nullptr if no user interaction needed).
    // target:      the combatant this action belongs to (may be nullptr)
    // onMutated:   callback to invoke after the action modifies pawn state (e.g. HP change)
    virtual QWidget* createRuntimeWidget(QWidget* parent = nullptr,
                                         PawnInstance* target = nullptr,
                                         std::function<void()> onMutated = nullptr)
    { Q_UNUSED(parent) Q_UNUSED(target) Q_UNUSED(onMutated) return nullptr; }

    // Display properties for the EffectList panel
    virtual QList<QPair<QString,QString>> getDisplayProperties() const { return {}; }

    // True if this action requires a DM click (e.g. saving throw)
    virtual bool requiresDmAction() const { return false; }

    // True if the DM has already performed this action this round
    virtual bool isPerformed()      const { return false; }

    // Core execution – called by EncounterInstance event loop
    virtual void executeEvent(EventType event, PawnInstance* target) = 0;

    // Serialization
    virtual QString     typeName() const = 0;   // for JSON "type" discriminator
    virtual QJsonObject saveToJson() const = 0;
    virtual void        loadFromJson(const QJsonObject& obj) = 0;
};

// ─────────────────────────────────────────────
//  1. LevelModifierAction
//  Increments or decrements EffectInstance::level on OnRoundEnd.
//  No runtime widget.
// ─────────────────────────────────────────────
class LevelModifierAction : public Action
{
public:
    enum class Direction { Increase, Decrease };

    Direction direction = Direction::Increase;

    Action*  clone() const override { return new LevelModifierAction(*this); }
    QWidget* createEditorWidget(QWidget* parent = nullptr, std::function<void()> onChanged = nullptr) override;
    QWidget* createRuntimeWidget(QWidget* parent = nullptr, PawnInstance* target = nullptr, std::function<void()> onMutated = nullptr) override;
    void     executeEvent(EventType event, PawnInstance* target) override;

    QString     typeName() const override { return "LevelModifier"; }
    QJsonObject saveToJson() const override;
    void        loadFromJson(const QJsonObject& obj) override;
};

// ─────────────────────────────────────────────
//  2. StageModifierAction
//  Advances or regresses EffectInstance::currentStageIndex on OnRoundEnd.
//  No runtime widget.
// ─────────────────────────────────────────────
class StageModifierAction : public Action
{
public:
    enum class Direction { Increase, Decrease };

    Direction direction = Direction::Increase;

    Action*  clone() const override { return new StageModifierAction(*this); }
    QWidget* createEditorWidget(QWidget* parent = nullptr, std::function<void()> onChanged = nullptr) override;
    QWidget* createRuntimeWidget(QWidget* parent = nullptr, PawnInstance* target = nullptr, std::function<void()> onMutated = nullptr) override;
    void     executeEvent(EventType event, PawnInstance* target) override;

    QString     typeName() const override { return "StageModifier"; }
    QJsonObject saveToJson() const override;
    void        loadFromJson(const QJsonObject& obj) override;
};

// ─────────────────────────────────────────────
//  3. LevelStageModifierAction  (Saving Throw)
//  Like StageModifier but the DM must confirm via a dice roll first.
//  Runtime widget: clickable dice button that opens DiceRollerPopup.
// ─────────────────────────────────────────────
class LevelStageModifierAction : public Action
{
public:
    enum class Target { Level, Stage };
    enum class Direction { Increase, Decrease };

    Target    target    = Target::Stage;
    Direction direction = Direction::Increase;
    QString   diceFormula;   // e.g. "1D20+4"

    bool performed = false;
    bool succeeded = false;

    Action*  clone() const override { return new LevelStageModifierAction(*this); }
    QWidget* createEditorWidget(QWidget* parent = nullptr, std::function<void()> onChanged = nullptr) override;
    QWidget* createRuntimeWidget(QWidget* parent = nullptr,
                                  PawnInstance* target = nullptr,
                                  std::function<void()> onMutated = nullptr) override;
    void     executeEvent(EventType event, PawnInstance* target) override;

    QList<QPair<QString,QString>> getDisplayProperties() const override { return {}; }
    bool requiresDmAction() const override { return true; }
    bool isPerformed()      const override { return performed; }

    QString     typeName() const override { return "LevelStageModifier"; }
    QJsonObject saveToJson() const override;
    void        loadFromJson(const QJsonObject& obj) override;
};

// ─────────────────────────────────────────────
//  4. DamageThrowAction
//  Rolls damage and applies it to the target's HP attribute.
//  Runtime widget: "Perform Damage Throw" button.
// ─────────────────────────────────────────────
class DamageThrowAction : public Action
{
public:
    QString damageType;    // e.g. "Fire", "Slashing"
    QString diceFormula;   // e.g. "8D6"

    bool performed  = false;
    int  lastResult = 0;

    Action*  clone() const override { return new DamageThrowAction(*this); }
    QWidget* createEditorWidget(QWidget* parent = nullptr, std::function<void()> onChanged = nullptr) override;
    QWidget* createRuntimeWidget(QWidget* parent = nullptr,
                                  PawnInstance* target = nullptr,
                                  std::function<void()> onMutated = nullptr) override;
    void     executeEvent(EventType event, PawnInstance* target) override;

    bool requiresDmAction() const override { return true; }
    bool isPerformed()      const override { return performed; }

    QString     typeName() const override { return "DamageThrow"; }
    QJsonObject saveToJson() const override;
    void        loadFromJson(const QJsonObject& obj) override;
};

// ─────────────────────────────────────────────
//  5. MaxDurationAction
//  Counts remaining rounds. Removes parent effect when counter hits 0.
//  Runtime widget: label "Rounds Remaining: X".
// ─────────────────────────────────────────────
class MaxDurationAction : public Action
{
public:
    int maxRounds      = 1;
    int roundsRemaining = 1;   // runtime state, reset on clone/apply

    Action*  clone() const override;   // resets roundsRemaining to maxRounds
    QWidget* createEditorWidget(QWidget* parent = nullptr, std::function<void()> onChanged = nullptr) override;
    QWidget* createRuntimeWidget(QWidget* parent = nullptr,
                                  PawnInstance* target = nullptr,
                                  std::function<void()> onMutated = nullptr) override;
    void     executeEvent(EventType event, PawnInstance* target) override;

    QList<QPair<QString,QString>> getDisplayProperties() const override;

    QString     typeName() const override { return "MaxDuration"; }
    QJsonObject saveToJson() const override;
    void        loadFromJson(const QJsonObject& obj) override;
};

// ─────────────────────────────────────────────
//  ActionFactory
//  Creates Action objects by type name string.
//  Used by editor menus and Stage::loadFromJson().
// ─────────────────────────────────────────────
#include <QMap>

class ActionFactory {
public:
    // Returns nullptr for unknown typeName
    static std::shared_ptr<Action> create(const QString& typeName);

    // Returns map of typeName → human-readable label for editor menus
    static QMap<QString, QString> registeredTypes();
};
