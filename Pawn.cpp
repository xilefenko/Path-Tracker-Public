#include "Pawn.h"
#include "pawnrow.h"
#include "PathTrackerStruct.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QDebug>

// ═══════════════════════════════════════════════════════════════════════════════
//  PawnTemplate
// ═══════════════════════════════════════════════════════════════════════════════

void PawnTemplate::setBaseAttribute(const QString& key, int value)
{
    for (AttributeValue& attributeValue : baseAttributes)
    {
        if (attributeValue.key == key)
        {
            attributeValue.baseValue    = value;
            attributeValue.currentValue = value;
            return;
        }
    }
    // Not found – insert new entry.
    AttributeValue attributeValue;
    attributeValue.key          = key;
    attributeValue.baseValue    = value;
    attributeValue.currentValue = value;
    baseAttributes.append(attributeValue);
}

int PawnTemplate::getBaseAttribute(const QString& key) const
{
    for (const AttributeValue& attributeValue : baseAttributes)
        if (attributeValue.key == key) return attributeValue.baseValue;
    return 0;
}

QJsonObject PawnTemplate::saveToJson() const
{
    QJsonObject obj;
    obj["id"]              = id;
    obj["name"]            = name;
    obj["pictureFilePath"] = PictureFilePath;

    QJsonArray attributeArray;
    for (const AttributeValue& attributeValue : baseAttributes)
        attributeArray.append(attributeValue.saveToJson());
    obj["baseAttributes"] = attributeArray;

    QJsonArray effectArray;
    for (const ChildEffectRef& effectReference : defaultEffects)
        effectArray.append(effectReference.saveToJson());
    obj["defaultEffects"] = effectArray;

    return obj;
}

void PawnTemplate::loadFromJson(const QJsonObject& obj)
{
    id              = obj["id"].toString();
    name            = obj["name"].toString();
    PictureFilePath = obj["pictureFilePath"].toString();

    baseAttributes.clear();
    for (const QJsonValue& jsonValue : obj["baseAttributes"].toArray())
    {
        AttributeValue attributeValue;
        attributeValue.loadFromJson(jsonValue.toObject());
        baseAttributes.append(attributeValue);
    }

    defaultEffects.clear();
    for (const QJsonValue& jsonValue : obj["defaultEffects"].toArray())
    {
        ChildEffectRef effectReference;
        effectReference.loadFromJson(jsonValue.toObject());
        defaultEffects.append(effectReference);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  PawnState helpers
// ═══════════════════════════════════════════════════════════════════════════════

static QString StateToString(PawnState state)
{
    switch (state) {
    case PawnState::Waiting:     return "Waiting";
    case PawnState::Active:      return "Active";
    case PawnState::Spent:       return "Spent";
    case PawnState::Unconscious: return "Unconscious";
    case PawnState::Dead:        return "Dead";
    }
    return "Waiting";
}

static PawnState StateFromString(const QString& s)
{
    if (s == "Active")      return PawnState::Active;
    if (s == "Spent")       return PawnState::Spent;
    if (s == "Unconscious") return PawnState::Unconscious;
    if (s == "Dead")        return PawnState::Dead;
    return PawnState::Waiting;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  PawnInstance
// ═══════════════════════════════════════════════════════════════════════════════

PawnInstance* PawnInstance::fromTemplate(const PawnTemplate& pawnTemplate)
{
    auto* newPawnInstance = new PawnInstance;
    newPawnInstance->instanceId  = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newPawnInstance->templateId  = pawnTemplate.id;
    newPawnInstance->displayName = pawnTemplate.name;
    newPawnInstance->attributes  = pawnTemplate.baseAttributes;   // deep copy of flat structs

    // Ensure mandatory HP and AC attributes are present
    auto ensureAttrValue = [&](const char* key, int defaultValue) {
        for (const AttributeValue& av : newPawnInstance->attributes)
            if (av.key == key) return;
        AttributeValue av;
        av.key          = key;
        av.baseValue    = defaultValue;
        av.currentValue = defaultValue;
        newPawnInstance->attributes.append(av);
    };
    ensureAttrValue(HP_KEY, 10);
    ensureAttrValue(AC_KEY, 10);

    // Default effects are applied by the caller (EncounterInstance) after construction
    // using PathTrackerStruct::findEffectTemplate.

    return newPawnInstance;
}

// ── Attribute access ──────────────────────────────────────────────────────────

int PawnInstance::getAttribute(const QString& key) const
{
    for (const AttributeValue& attributeValue : attributes)
        if (attributeValue.key == key) return attributeValue.currentValue;
    return 0;
}

void PawnInstance::setAttribute(const QString& key, int value)
{
    for (AttributeValue& attributeValue : attributes)
    {
        if (attributeValue.key == key)
        {
            attributeValue.currentValue = value;
            return;
        }
    }
    // Attribute not in list – add it (runtime-only attribute e.g. temp HP).
    AttributeValue attributeValue;
    attributeValue.key          = key;
    attributeValue.baseValue    = value;
    attributeValue.currentValue = value;
    attributes.append(attributeValue);
}

void PawnInstance::resolveAttributes()
{
    // Step 1: reset every attribute's currentValue to its baseValue,
    // except HP which is a tracked resource managed explicitly via setAttribute.
    for (AttributeValue& attributeValue : attributes)
        if (attributeValue.key != HP_KEY)
            attributeValue.currentValue = attributeValue.baseValue;

    // Step 2: apply modifiers from every active effect's current stage.
    for (const auto& effectInstance : effects)
    {
        const Stage* stage = effectInstance->currentStage();
        if (!stage) continue;

        for (const AttributeModifier& attributeModifier : stage->modifiers)
        {
            for (AttributeValue& attributeValue : attributes)
            {
                if (attributeValue.key == attributeModifier.attributeKey)
                {
                    attributeValue.currentValue = attributeModifier.apply(attributeValue.currentValue);
                    break;
                }
            }
        }
    }
}

// ── Effect management ─────────────────────────────────────────────────────────

void PawnInstance::applyEffect(const EffectTemplate& tmpl)
{
    auto* inst = EffectInstance::fromTemplate(tmpl, this);
    effects.emplace_back(inst);

    // Fire the OnApplied event immediately.
    inst->fireEvent(EventType::OnApplied);

    // Auto-apply all child effects defined in the initial stage.
    const Stage* stage = inst->currentStage();
    if (stage)
    {
        for (const ChildEffectRef& child : stage->childEffects)
        {
            EffectTemplate* childTmpl =
                PathTrackerStruct::instance().findEffectTemplate(child.effectTemplateId);
            if (childTmpl)
                applyEffect(*childTmpl); // recursive – also fires OnApplied and resolves
        }
    }

    resolveAttributes();
}

void PawnInstance::removeEffect(const QString& effectInstanceId)
{
    for (int i = 0; i < effects.size(); ++i)
    {
        if (effects[i]->instanceId == effectInstanceId)
        {
            // Fire removal event before erasing.
            effects[i]->fireEvent(EventType::OnRemoved);

            // Also remove any Bound child effects.
            const Stage* stage = effects[i]->currentStage();
            if (stage)
            {
                for (const ChildEffectRef& child : stage->childEffects)
                {
                    if (child.binding == ChildBinding::Bound)
                    {
                        // Find and remove child effect by templateId match.
                        for (int j = effects.size() - 1; j >= 0; --j)
                        {
                            if (effects[j]->templateId == child.effectTemplateId)
                            {
                                effects[j]->fireEvent(EventType::OnRemoved);
                                effects.erase(effects.begin() + j);
                                break;
                            }
                        }
                    }
                }
            }

            effects.erase(effects.begin() + i);
            resolveAttributes();
            return;
        }
    }
    qWarning() << "PawnInstance::removeEffect – instanceId not found:" << effectInstanceId;
}

void PawnInstance::syncBoundChildEffects(EffectInstance* inst, int oldStageIndex)
{
    // Collect bound children from the old stage.
    QList<QString> toRemove;
    if (oldStageIndex >= 0 && oldStageIndex < inst->stages.size())
    {
        for (const ChildEffectRef& child : inst->stages[oldStageIndex].childEffects)
        {
            if (child.binding == ChildBinding::Bound)
                toRemove.append(child.effectTemplateId);
        }
    }

    // Remove them (they were bound to the old stage, not the new one).
    for (const QString& tid : toRemove)
    {
        for (int i = static_cast<int>(effects.size()) - 1; i >= 0; --i)
        {
            if (effects[i]->templateId == tid)
            {
                effects[i]->fireEvent(EventType::OnRemoved);
                effects.erase(effects.begin() + i);
                break;
            }
        }
    }

    // Apply all children declared in the new stage.
    const Stage* newStage = inst->currentStage();
    if (newStage)
    {
        for (const ChildEffectRef& child : newStage->childEffects)
        {
            EffectTemplate* childTmpl =
                PathTrackerStruct::instance().findEffectTemplate(child.effectTemplateId);
            if (childTmpl)
                applyEffect(*childTmpl);
        }
    }

    resolveAttributes();
}

EffectInstance* PawnInstance::findEffect(const QString& effectInstanceId)
{
    for (const auto& effectInstance : effects)
        if (effectInstance->instanceId == effectInstanceId) return effectInstance.get();
    return nullptr;
}

// ── Event forwarding ─────────────────────────────────────────────────────────

void PawnInstance::fireEvent(EventType event)
{
    // Iterate a copy of the list – effects may be removed during iteration
    // (e.g. MaxDurationAction calls removeEffect).
    QList<EffectInstance*> snapshot;
    for (const auto& effectInstance : effects)
        snapshot.append(effectInstance.get());

    for (EffectInstance* effectInstance : snapshot)
    {
        // Guard: effect may have been removed by an earlier action this round.
        if (findEffect(effectInstance->instanceId))
            effectInstance->fireEvent(event);
    }
}

// ── Serialization ─────────────────────────────────────────────────────────────

QJsonObject PawnInstance::saveToJson() const
{
    QJsonObject obj;
    obj["instanceId"]  = instanceId;
    obj["templateId"]  = templateId;
    obj["displayName"] = displayName;

    QJsonArray attributeArray;
    for (const AttributeValue& attributeValue : attributes)
        attributeArray.append(attributeValue.saveToJson());
    obj["attributes"] = attributeArray;

    QJsonArray effectArray;
    for (const auto& effectInstance : effects)
        effectArray.append(effectInstance->saveToJson());
    obj["effects"] = effectArray;

    obj["state"]           = StateToString(state);
    obj["savedInitiative"] = savedInitiative;

    return obj;
}

void PawnInstance::loadFromJson(const QJsonObject& obj)
{
    instanceId  = obj["instanceId"].toString();
    templateId  = obj["templateId"].toString();
    displayName = obj["displayName"].toString();
    state           = StateFromString(obj["state"].toString("Waiting"));
    savedInitiative = obj["savedInitiative"].toInt(0);

    attributes.clear();
    for (const QJsonValue& jsonValue : obj["attributes"].toArray())
    {
        AttributeValue attributeValue;
        attributeValue.loadFromJson(jsonValue.toObject());
        attributes.append(attributeValue);
    }

    effects.clear();
    for (const QJsonValue& jsonValue : obj["effects"].toArray())
    {
        auto* newEffectInstance = new EffectInstance;
        newEffectInstance->loadFromJson(jsonValue.toObject());
        newEffectInstance->owner = this;   // re-link owner pointer
        effects.emplace_back(newEffectInstance);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Player
// ═══════════════════════════════════════════════════════════════════════════════

Player* Player::create(const QString& name)
{
    auto* newPlayer = new Player;
    newPlayer->instanceId  = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newPlayer->templateId  = {};   // Players have no template
    newPlayer->displayName = name;
    newPlayer->playerName  = name;

    // Ensure mandatory HP and AC attributes
    auto ensureAttrValue = [&](const char* key, int defaultValue) {
        AttributeValue av;
        av.key          = key;
        av.baseValue    = defaultValue;
        av.currentValue = defaultValue;
        newPlayer->attributes.append(av);
    };
    ensureAttrValue(HP_KEY, 10);
    ensureAttrValue(AC_KEY, 10);

    return newPlayer;
}

QJsonObject Player::saveToJson() const
{
    QJsonObject obj = PawnInstance::saveToJson();
    obj["playerName"]      = playerName;
    obj["isPlayer"]        = true;
    obj["pictureFilePath"] = PictureFilePath;
    return obj;
}

void Player::loadFromJson(const QJsonObject& obj)
{
    PawnInstance::loadFromJson(obj);
    playerName      = obj["playerName"].toString(displayName);
    PictureFilePath = obj["pictureFilePath"].toString();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  PlayerRegistry
// ═══════════════════════════════════════════════════════════════════════════════

PlayerRegistry& PlayerRegistry::instance()
{
    static PlayerRegistry s_instance;
    return s_instance;
}

void PlayerRegistry::addPlayer(Player* player)
{
    Q_ASSERT(player);

    // Enforce uniqueness by instanceId.
    for (const auto& existingPlayer : m_players)
    {
        if (existingPlayer->instanceId == player->instanceId)
        {
            qWarning() << "PlayerRegistry::addPlayer – duplicate instanceId, ignored.";
            delete player;
            return;
        }
    }
    m_players.emplace_back(player);
}

void PlayerRegistry::removePlayer(const QString& instanceId)
{
    for (int i = 0; i < m_players.size(); ++i)
    {
        if (m_players[i]->instanceId == instanceId)
        {
            m_players.erase(m_players.begin() + i);
            return;
        }
    }
}

Player* PlayerRegistry::findPlayer(const QString& instanceId)
{
    for (const auto& player : m_players)
        if (player->instanceId == instanceId) return player.get();
    return nullptr;
}

QList<Player*> PlayerRegistry::allPlayers()
{
    QList<Player*> result;
    result.reserve(m_players.size());
    for (const auto& player : m_players)
        result.append(player.get());
    return result;
}

QJsonArray PlayerRegistry::saveToJson() const
{
    QJsonArray arr;
    for (const auto& player : m_players)
        arr.append(player->saveToJson());
    return arr;
}

void PlayerRegistry::loadFromJson(const QJsonArray& arr)
{
    m_players.clear();
    for (const QJsonValue& jsonValue : arr)
    {
        auto* player = new Player;
        player->loadFromJson(jsonValue.toObject());
        m_players.emplace_back(player);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  List widget factories
// ═══════════════════════════════════════════════════════════════════════════════

QWidget* PawnInstance::createListWidget(int initiative)
{
    PawnRow* row = new PawnRow();
    row->SetData(this, initiative);
    return row;
}

QWidget* Player::createListWidget(int initiative)
{
    PawnRow* row = new PawnRow();
    row->SetData(this, initiative);
    return row;
}
