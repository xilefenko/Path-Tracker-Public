#include "Encounter.h"
#include "PathTrackerStruct.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <algorithm>
#include <QDebug>

// ═══════════════════════════════════════════════════════════════════════════════
//  EncounterTemplate
// ═══════════════════════════════════════════════════════════════════════════════

void EncounterTemplate::addPawnTemplate(const QString& templateId)
{
    //if (!pawnTemplateIds.contains(templateId))
    //
        pawnTemplateIds.append(templateId);
}

void EncounterTemplate::removePawnTemplate(const QString& templateId)
{
    pawnTemplateIds.removeAll(templateId);
}

QJsonObject EncounterTemplate::saveToJson() const
{
    QJsonObject obj;
    obj["id"]   = id;
    obj["name"] = name;

    QJsonArray arr;
    for (const QString& pawnTemplateId : pawnTemplateIds)
        arr.append(pawnTemplateId);
    obj["pawnTemplateIds"] = arr;

    return obj;
}

void EncounterTemplate::loadFromJson(const QJsonObject& obj)
{
    id   = obj["id"].toString();
    name = obj["name"].toString();

    pawnTemplateIds.clear();
    for (const QJsonValue& jsonValue : obj["pawnTemplateIds"].toArray())
        pawnTemplateIds.append(jsonValue.toString());
}

// ═══════════════════════════════════════════════════════════════════════════════
//  EncounterInstance
// ═══════════════════════════════════════════════════════════════════════════════

EncounterInstance* EncounterInstance::fromTemplate(
    const EncounterTemplate& tmpl,
    const QMap<QString, PawnTemplate*>& templateLookup)
{
    auto* inst = new EncounterInstance;
    inst->instanceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    inst->templateId = tmpl.id;
    inst->name       = tmpl.name;

    for (const QString& pawnTemplateId : tmpl.pawnTemplateIds)
    {
        PawnTemplate* pawnTemplate = templateLookup.value(pawnTemplateId, nullptr);
        if (!pawnTemplate)
        {
            qWarning() << "EncounterInstance::fromTemplate – PawnTemplate not found:" << pawnTemplateId;
            continue;
        }

        PawnInstance* pawn = PawnInstance::fromTemplate(*pawnTemplate);

        // Apply default effects defined on the pawn template.
        for (const ChildEffectRef& ref : pawnTemplate->defaultEffects)
        {
            EffectTemplate* effectTemplate =
                PathTrackerStruct::instance().findEffectTemplate(ref.effectTemplateId);
            if (effectTemplate)
                pawn->applyEffect(*effectTemplate);
        }

        inst->ownedPawns.emplace_back(pawn);

        // Populate initiative list with a default of 0; DM sets real values later.
        InitiativeEntry entry;
        entry.initiative = 0;
        entry.pawn       = pawn;
        inst->initiativeOrder.append(entry);
    }

    return inst;
}

// ── Pawn management ───────────────────────────────────────────────────────────

void EncounterInstance::addPawnInstance(const PawnTemplate& tmpl)
{
    PawnInstance* pawn = PawnInstance::fromTemplate(tmpl);

    // Apply default effects defined on the pawn template.
    for (const ChildEffectRef& ref : tmpl.defaultEffects)
    {
        EffectTemplate* effectTemplate =
            PathTrackerStruct::instance().findEffectTemplate(ref.effectTemplateId);
        if (effectTemplate)
            pawn->applyEffect(*effectTemplate);
    }

    ownedPawns.emplace_back(pawn);

    InitiativeEntry entry;
    entry.initiative = 0;
    entry.pawn       = pawn;
    initiativeOrder.append(entry);
}

void EncounterInstance::addPlayer(Player* player)
{
    Q_ASSERT(player);
    if (activePlayers.contains(player))
    {
        qWarning() << "EncounterInstance::addPlayer – player already active.";
        return;
    }
    activePlayers.append(player);

    InitiativeEntry entry;
    entry.initiative = 0;
    entry.pawn       = player;
    initiativeOrder.append(entry);
}

void EncounterInstance::removePlayer(const QString& instId)
{
    for (int i = 0; i < activePlayers.size(); ++i)
    {
        if (activePlayers[i]->instanceId == instId)
        {
            activePlayers.removeAt(i);

            // Also remove from initiative list.
            for (int j = initiativeOrder.size() - 1; j >= 0; --j)
            {
                if (initiativeOrder[j].pawn &&
                    initiativeOrder[j].pawn->instanceId == instId)
                {
                    initiativeOrder.removeAt(j);
                }
            }
            return;
        }
    }
}

QList<PawnInstance*> EncounterInstance::allCombatants() const
{
    QList<PawnInstance*> all;
    all.reserve(ownedPawns.size() + activePlayers.size());

    for (const auto& p : ownedPawns)
        all.append(p.get());

    for (Player* player : activePlayers)
        all.append(player);

    return all;
}

// ── Initiative ────────────────────────────────────────────────────────────────

void EncounterInstance::setInitiative(const QString& pawnInstanceId, int value)
{
    for (InitiativeEntry& entry : initiativeOrder)
    {
        if (entry.pawn && entry.pawn->instanceId == pawnInstanceId)
        {
            entry.initiative = value;
            return;
        }
    }
    qWarning() << "EncounterInstance::setInitiative – pawn not found:" << pawnInstanceId;
}

void EncounterInstance::sortInitiative()
{
    std::stable_sort(initiativeOrder.begin(), initiativeOrder.end());
    currentInitiativeIndex = 0;
}

PawnInstance* EncounterInstance::currentActor() const
{
    if (initiativeOrder.isEmpty()) return nullptr;
    const int idx = qBound(0, currentInitiativeIndex, initiativeOrder.size() - 1);
    return initiativeOrder[idx].pawn;
}

void EncounterInstance::advanceTurn()
{
    if (initiativeOrder.isEmpty()) return;

    // 1. current actor is always index 0
    PawnInstance* current = currentActor();
    if (!current) return;

    // 2. fire OnTurnEnd
    current->fireEvent(EventType::OnTurnEnd);

    // 3. state transition: Active or Unconscious → Spent (had their turn)
    if (current->state == PawnState::Active || current->state == PawnState::Unconscious)
        current->state = PawnState::Spent;

    // 4. re-sort
    std::stable_sort(initiativeOrder.begin(), initiativeOrder.end());

    // 5. check if any Waiting or Unconscious remain
    bool anyWaiting = false;
    for (const InitiativeEntry& initiativeEntry : initiativeOrder)
        if (initiativeEntry.pawn && (initiativeEntry.pawn->state == PawnState::Waiting || initiativeEntry.pawn->state == PawnState::Unconscious)) {
            anyWaiting = true;
            break;
        }

    // 6. no Waiting → new round
    if (!anyWaiting) {
        fireRoundEnd();
        for (InitiativeEntry& initiativeEntry : initiativeOrder)
            if (initiativeEntry.pawn && initiativeEntry.pawn->state == PawnState::Spent)
                initiativeEntry.pawn->state = PawnState::Waiting;
        std::stable_sort(initiativeOrder.begin(), initiativeOrder.end());
        fireRoundStart();
    }

    // 7. promote first pawn to Active if it is Waiting or Unconscious
    if (!initiativeOrder.isEmpty() && initiativeOrder[0].pawn)
        if (initiativeOrder[0].pawn->state == PawnState::Waiting ||
            initiativeOrder[0].pawn->state == PawnState::Unconscious)
            initiativeOrder[0].pawn->state = PawnState::Active;

    // 8. fire OnTurnStart
    if (!initiativeOrder.isEmpty() && initiativeOrder[0].pawn)
        initiativeOrder[0].pawn->fireEvent(EventType::OnTurnStart);

    // 9. index stays 0
    currentInitiativeIndex = 0;
}

void EncounterInstance::markDead(const QString& pawnInstanceId)
{
    int  foundIndex = -1;
    bool wasActive  = false;

    for (int i = 0; i < initiativeOrder.size(); ++i) {
        if (initiativeOrder[i].pawn && initiativeOrder[i].pawn->instanceId == pawnInstanceId) {
            foundIndex = i;
            wasActive  = (initiativeOrder[i].pawn->state == PawnState::Active);
            initiativeOrder[i].pawn->savedInitiative = initiativeOrder[i].initiative;
            initiativeOrder[i].pawn->state           = PawnState::Dead;
            break;
        }
    }

    if (foundIndex == -1) {
        // Not in initiativeOrder – mark in allCombatants anyway
        for (PawnInstance* combatant : allCombatants())
            if (combatant->instanceId == pawnInstanceId) { combatant->state = PawnState::Dead; return; }
        return;
    }

    initiativeOrder.removeAt(foundIndex);
    std::stable_sort(initiativeOrder.begin(), initiativeOrder.end());

    // If the active pawn just died, promote the new front if it is Waiting
    if (wasActive && !initiativeOrder.isEmpty() && initiativeOrder[0].pawn)
        if (initiativeOrder[0].pawn->state == PawnState::Waiting)
            initiativeOrder[0].pawn->state = PawnState::Active;
}

void EncounterInstance::revive(const QString& pawnInstanceId)
{
    PawnInstance* pawn = nullptr;
    for (PawnInstance* combatant : allCombatants())
        if (combatant->instanceId == pawnInstanceId) { pawn = combatant; break; }
    if (!pawn) return;

    pawn->state = PawnState::Waiting;

    InitiativeEntry entry;
    entry.initiative = pawn->savedInitiative;
    entry.pawn       = pawn;
    initiativeOrder.append(entry);
    std::stable_sort(initiativeOrder.begin(), initiativeOrder.end());
}

void EncounterInstance::markUnconscious(const QString& pawnInstanceId)
{
    for (InitiativeEntry& initiativeEntry : initiativeOrder) {
        if (initiativeEntry.pawn && initiativeEntry.pawn->instanceId == pawnInstanceId) {
            if (initiativeEntry.pawn->state == PawnState::Waiting)
                initiativeEntry.pawn->state = PawnState::Unconscious;
            else if (initiativeEntry.pawn->state == PawnState::Unconscious)
                initiativeEntry.pawn->state = PawnState::Waiting;
            return;
        }
    }
}

// ── Round tracking ────────────────────────────────────────────────────────────

void EncounterInstance::fireRoundStart()
{
    for (PawnInstance* pawn : allCombatants())
        pawn->fireEvent(EventType::OnRoundStart);
}

void EncounterInstance::fireRoundEnd()
{
    for (PawnInstance* pawn : allCombatants())
        pawn->fireEvent(EventType::OnRoundEnd);

    currentRound++;
    qDebug() << "Round" << currentRound << "begins.";
}

// ── Serialization ─────────────────────────────────────────────────────────────

QJsonObject EncounterInstance::saveToJson() const
{
    QJsonObject obj;
    obj["instanceId"]              = instanceId;
    obj["templateId"]              = templateId;
    obj["name"]                    = name;
    obj["isOngoing"]               = isOngoing;
    obj["currentRound"]            = currentRound;
    obj["currentInitiativeIndex"]  = currentInitiativeIndex;

    // Save owned pawns.
    QJsonArray ownedArr;
    for (const auto& p : ownedPawns)
        ownedArr.append(p->saveToJson());
    obj["ownedPawns"] = ownedArr;

    // Save active players with full state so undo/redo can restore their effects.
    QJsonArray playerArr;
    for (const Player* player : activePlayers)
        playerArr.append(player->saveToJson());
    obj["activePlayers"] = playerArr;

    // Save initiative order as pairs [instanceId, initiative].
    QJsonArray initArr;
    for (const InitiativeEntry& entry : initiativeOrder)
    {
        QJsonObject initiativeEntry;
        initiativeEntry["pawnInstanceId"] = entry.pawn ? entry.pawn->instanceId : QString{};
        initiativeEntry["initiative"]     = entry.initiative;
        initArr.append(initiativeEntry);
    }
    obj["initiativeOrder"] = initArr;

    return obj;
}

void EncounterInstance::loadFromJson(const QJsonObject& obj)
{
    instanceId             = obj["instanceId"].toString();
    templateId             = obj["templateId"].toString();
    name                   = obj["name"].toString();
    isOngoing              = obj["isOngoing"].toBool(false);
    currentRound           = obj["currentRound"].toInt(1);
    currentInitiativeIndex = obj["currentInitiativeIndex"].toInt(0);

    // Restore owned pawns.
    ownedPawns.clear();
    for (const QJsonValue& jsonValue : obj["ownedPawns"].toArray())
    {
        auto* pawn = new PawnInstance;
        pawn->loadFromJson(jsonValue.toObject());
        ownedPawns.emplace_back(pawn);
    }

    // Restore active players: look them up from PlayerRegistry and restore their state.
    // Supports both old format (array of id strings) and new format (array of full objects).
    activePlayers.clear();
    for (const QJsonValue& jsonValue : obj["activePlayers"].toArray())
    {
        QString playerId;
        QJsonObject playerObj;
        if (jsonValue.isObject())
        {
            playerObj = jsonValue.toObject();
            playerId  = playerObj["instanceId"].toString();
        }
        else
        {
            playerId = jsonValue.toString();
        }

        Player* player = PlayerRegistry::instance().findPlayer(playerId);
        if (player)
        {
            if (!playerObj.isEmpty())
                player->loadFromJson(playerObj);   // restore full state (effects, HP, etc.)
            activePlayers.append(player);
        }
        else
        {
            qWarning() << "EncounterInstance::loadFromJson – player not found:" << playerId;
        }
    }

    // Legacy: old saves used "activePlayerIds" (array of strings).
    if (activePlayers.isEmpty() && obj.contains("activePlayerIds"))
    {
        for (const QJsonValue& jsonValue : obj["activePlayerIds"].toArray())
        {
            const QString legacyId = jsonValue.toString();
            Player* player = PlayerRegistry::instance().findPlayer(legacyId);
            if (player)
                activePlayers.append(player);
        }
    }

    // Restore initiative order. We need to look up PawnInstance* from the
    // owned list or active players by instanceId.
    initiativeOrder.clear();

    // Build a flat instanceId -> PawnInstance* map for quick lookup.
    QMap<QString, PawnInstance*> pawnMap;
    for (const auto& p : ownedPawns)
        pawnMap[p->instanceId] = p.get();
    for (Player* player : activePlayers)
        pawnMap[player->instanceId] = player;

    for (const QJsonValue& jsonValue : obj["initiativeOrder"].toArray())
    {
        const QJsonObject initiativeEntry = jsonValue.toObject();
        InitiativeEntry entry;
        entry.initiative = initiativeEntry["initiative"].toInt(0);
        entry.pawn       = pawnMap.value(initiativeEntry["pawnInstanceId"].toString(), nullptr);
        initiativeOrder.append(entry);
    }
}
