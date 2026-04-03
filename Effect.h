#pragma once
#include "Attribute.h"
#include "Action.h"
#include <QString>
#include <QList>
#include <QPair>
#include <QJsonObject>
#include <QWidget>
#include <memory>

// ─────────────────────────────────────────────
//  ChildEffectRef
//  Flat reference stored inside a Stage.
//  Binding = Bound means removal follows the parent.
// ─────────────────────────────────────────────
enum class ChildBinding { Bound, Unbound };

struct ChildEffectRef
{
    QString      effectTemplateId;   // UUID string – resolved post-load
    ChildBinding binding = ChildBinding::Bound;
    int          level   = 1;

    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);
};

// ─────────────────────────────────────────────
//  Stage  (plain data struct, owned by EffectTemplate)
//  Three flat arrays – no nested objects.
// ─────────────────────────────────────────────
struct Stage
{
    QList<AttributeModifier>        modifiers;
    QList<ChildEffectRef>           childEffects;
    QList<std::shared_ptr<Action>>  actions;

    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);
};

// ─────────────────────────────────────────────
//  EffectTemplate  (Editor artefact)
//  Defines lifecycle, stages, and flavor text.
//  Never mutated at runtime.
// ─────────────────────────────────────────────
class EffectTemplate
{
public:
    QString      id;            // UUID
    QString      name;
    QString      description;   // flavor text
    QString      onset;         // flavor text
    QString      PictureFilePath; // relative path, e.g. "icons/foo.png"
    QList<Stage> stages;

    // Editor helpers
    Stage&       addStage();
    void         removeStage(int index);

    QJsonObject  saveToJson() const;
    void         loadFromJson(const QJsonObject& obj);
};

// ─────────────────────────────────────────────
//  EffectInstance  (Runtime object)
//  Created from an EffectTemplate by instantiating
//  all Stage actions via clone().
// ─────────────────────────────────────────────
class PawnInstance;   // forward

class EffectInstance
{
public:
    // Construction
    static EffectInstance* fromTemplate(const EffectTemplate& tmpl, PawnInstance* owner);

    // Identity / linkage
    QString   templateId;    // source EffectTemplate::id
    QString   instanceId;    // unique per runtime session
    PawnInstance* owner = nullptr;

    // Runtime state (flat integers – no extra class)
    int level             = 0;
    int currentStageIndex = 0;
    int currentRound      = 0;

    // Cloned stages (actions are deep-copied so they own runtime state)
    QList<Stage> stages;

    // Convenience accessors
    Stage*       currentStage();
    const Stage* currentStage() const;
    bool         advanceStage();   // returns false if already at last stage
    bool         regressStage();   // returns false if already at first stage

    // Event dispatch – iterates currentStage's actions
    void         fireEvent(EventType event);

    // Display properties for the EffectList panel
    QList<QPair<QString,QString>> getDisplayProperties(const EffectTemplate* tmpl = nullptr) const;

    // Runtime UI row (Property -> Value table widget)
    QWidget*     createRuntimeWidget(QWidget* parent = nullptr);

    // Serialization (runtime snapshot)
    QJsonObject  saveToJson() const;
    void         loadFromJson(const QJsonObject& obj);
};
