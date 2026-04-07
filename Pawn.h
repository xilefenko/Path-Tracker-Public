#pragma once
#include "Attribute.h"
#include "Effect.h"
#include <QString>
#include <QList>
#include <QJsonObject>
#include <memory>
#include <vector>

class QWidget;

enum class PawnState { Waiting, Active, Spent, Unconscious, Dead };

// ─────────────────────────────────────────────
//  PawnTemplate  (Editor artefact)
//  Defines base stats and pre-attached effects.
//  Never mutated at runtime.
// ─────────────────────────────────────────────
class PawnTemplate
{
public:
    QString id;     // UUID
    QString name;
    QString PictureFilePath; // relative path, e.g. "portraits/goblin.png"

    // Flat list of base attribute values (key + base integer)
    QList<AttributeValue>   baseAttributes;

    // Effects pre-attached to this pawn type
    QList<ChildEffectRef>   defaultEffects;

    // Editor helpers
    void setBaseAttribute(const QString& key, int value);
    int  getBaseAttribute(const QString& key) const;

    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);
};

// ─────────────────────────────────────────────
//  PawnInstance  (Runtime object)
//  Generated from a PawnTemplate by the Encounter.
//  Holds live attribute values and active effects.
// ─────────────────────────────────────────────
class PawnInstance
{
public:
    // Construction
    static PawnInstance* fromTemplate(const PawnTemplate& tmpl);

    // Identity
    QString instanceId;    // UUID, unique per session
    QString templateId;    // source PawnTemplate::id
    QString displayName;

    // Combat state
    PawnState state           = PawnState::Waiting;
    int       savedInitiative = 0;   // stored on markDead, used on revive

    // DM notes (runtime-only, not derived from template)
    QString note;

    // Flat live attribute table (key -> current value)
    QList<AttributeValue>                     attributes;

    // Active effects (owned)
    std::vector<std::unique_ptr<EffectInstance>>    effects;

    // ── Attribute access ──────────────────────
    int  getAttribute(const QString& key) const;
    void setAttribute(const QString& key, int value);

    // Recalculates currentValue for every attribute by summing
    // base values + modifiers from all active effects/stages.
    // Call after adding/removing any effect.
    void resolveAttributes();

    // ── Effect management ─────────────────────
    void         applyEffect(const EffectTemplate& tmpl);
    void         removeEffect(const QString& instanceId);
    EffectInstance* findEffect(const QString& instanceId);

    // Called by EffectInstance::advanceStage / regressStage to keep bound
    // child effects in sync with the new stage.
    void         syncBoundChildEffects(EffectInstance* inst, int oldStageIndex);

    // ── Event forwarding (called by EncounterInstance) ──
    void fireEvent(EventType event);

    // Serialization (runtime snapshot)
    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);

    virtual QWidget* createListWidget(int initiative);

protected:
    PawnInstance() = default;

    friend class EncounterInstance;   // needs new PawnInstance in loadFromJson
};

// ─────────────────────────────────────────────
//  Player  (Inherits PawnInstance)
//  Persistent across encounters. Lives in the global
//  PlayerRegistry "Back Bench".
//  Only one instance per player may exist at a time.
// ─────────────────────────────────────────────
class Player : public PawnInstance
{
public:
    // Players have no PawnTemplate – created directly.
    static Player* create(const QString& name);

    // Extra persistent data
    QString playerName;
    QString PictureFilePath; // relative path, e.g. "portraits/hero.png"

    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);

    QWidget* createListWidget(int initiative) override;
};

// ─────────────────────────────────────────────
//  PlayerRegistry  (Singleton "Back Bench")
//  Holds all Player objects. Enforces uniqueness.
//  EncounterInstance pulls Players from here at runtime.
// ─────────────────────────────────────────────
class PlayerRegistry
{
public:
    static PlayerRegistry& instance();

    void    addPlayer(Player* player);       // takes ownership
    void    removePlayer(const QString& instanceId);
    Player* findPlayer(const QString& instanceId);
    QList<Player*> allPlayers();

    // Serialization
    QJsonArray  saveToJson() const;
    void        loadFromJson(const QJsonArray& arr);

private:
    PlayerRegistry() = default;
    std::vector<std::unique_ptr<Player>> m_players;
};
