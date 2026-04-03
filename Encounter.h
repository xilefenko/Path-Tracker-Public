#pragma once
#include "Pawn.h"
#include <QString>
#include <QList>
#include <QJsonObject>
#include <memory>
#include <vector>

// ─────────────────────────────────────────────
//  InitiativeEntry  (flat struct)
//  Keeps the initiative order inside an EncounterInstance.
// ─────────────────────────────────────────────
struct InitiativeEntry
{
    int           initiative = 0;
    PawnInstance* pawn       = nullptr;   // non-owning

    bool operator<(const InitiativeEntry& other) const {
        auto statePriority = [](PawnState s) -> int {
            if (s == PawnState::Active)                                    return 0;
            if (s == PawnState::Waiting || s == PawnState::Unconscious)    return 1;
            if (s == PawnState::Spent)                                     return 2;
            return 3;
        };
        int leftPriority = pawn       ? statePriority(pawn->state)       : 3;
        int rightPriority = other.pawn ? statePriority(other.pawn->state) : 3;
        if (leftPriority != rightPriority) return leftPriority < rightPriority;
        if (initiative != other.initiative) return initiative > other.initiative;
        QString leftId = pawn       ? pawn->instanceId       : "";
        QString rightId = other.pawn ? other.pawn->instanceId : "";
        return leftId < rightId;
    }
};

// ─────────────────────────────────────────────
//  EncounterTemplate  (Editor artefact)
//  A named list of PawnTemplate references.
// ─────────────────────────────────────────────
class EncounterTemplate
{
public:
    QString         id;   // UUID
    QString         name;

    // Flat list of pawn template IDs (resolved post-load)
    QList<QString>  pawnTemplateIds;

    // Editor helpers
    void addPawnTemplate(const QString& templateId);
    void removePawnTemplate(const QString& templateId);

    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);
};

// ─────────────────────────────────────────────
//  EncounterInstance  (Runtime object)
//  Instantiated from an EncounterTemplate.
//  Owns all PawnInstances for this fight.
//  Players are borrowed (non-owning) from PlayerRegistry.
// ─────────────────────────────────────────────
class EncounterInstance
{
public:
    // Construction
    static EncounterInstance* fromTemplate(const EncounterTemplate& tmpl,
                                           const QMap<QString, PawnTemplate*>& templateLookup);

    QString instanceId;
    QString templateId;
    QString name;

    // ── Pawn management ──────────────────────
    // Enemy pawns owned by this encounter
    std::vector<std::unique_ptr<PawnInstance>> ownedPawns;

    // Player pawns pulled from the Back Bench (non-owning)
    QList<Player*>                       activePlayers;

    // Add an enemy mid-combat (from any PawnTemplate)
    void addPawnInstance(const PawnTemplate& tmpl);

    // Slot a Player from the Back Bench into this encounter
    void addPlayer(Player* player);
    void removePlayer(const QString& instanceId);

    // Flat combined view of all combatants (owned + players)
    QList<PawnInstance*> allCombatants() const;

    // ── Initiative ───────────────────────────
    QList<InitiativeEntry> initiativeOrder;
    int                    currentInitiativeIndex = 0;

    void setInitiative(const QString& pawnInstanceId, int value);
    void sortInitiative();

    PawnInstance* currentActor() const;
    void          advanceTurn();

    void markDead(const QString& pawnInstanceId);
    void revive(const QString& pawnInstanceId);
    void markUnconscious(const QString& pawnInstanceId);

    // ── Ongoing flag ─────────────────────────
    bool isOngoing = false;

    // ── Round tracking ───────────────────────
    int  currentRound = 1;

    void fireRoundStart();   // fires OnRoundStart on every combatant
    void fireRoundEnd();     // fires OnRoundEnd on every combatant, increments round

    // ── Serialization (runtime snapshot) ─────
    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);

private:
    EncounterInstance() = default;

    friend class PathTrackerStruct;   // needs make_unique<EncounterInstance> in parseSaveDocument
};
