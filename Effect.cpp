#include "Effect.h"
#include "Pawn.h"
#include "AttributeRegistry.h"
#include "PathTrackerStruct.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPair>
#include <QUuid>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

// ─────────────────────────────────────────────
//  Time helper
//  Pathfinder: 1 round = 6 sec, 1 min = 10 rounds, 1 hr = 60 min = 600 rounds
// ─────────────────────────────────────────────
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

// ─────────────────────────────────────────────
//  ChildEffectRef
// ─────────────────────────────────────────────

QJsonObject ChildEffectRef::saveToJson() const
{
    QJsonObject obj;
    obj["effectTemplateId"] = effectTemplateId;
    obj["binding"]          = (binding == ChildBinding::Bound) ? "Bound" : "Unbound";
    obj["level"]            = level;
    return obj;
}

void ChildEffectRef::loadFromJson(const QJsonObject& obj)
{
    effectTemplateId = obj["effectTemplateId"].toString();
    binding = (obj["binding"].toString() == "Bound")
              ? ChildBinding::Bound : ChildBinding::Unbound;
    level = obj["level"].toInt(1);
}

// ─────────────────────────────────────────────
//  Stage  – helper to (de)serialise the Action* list
//  using the type-discriminator pattern.
// ─────────────────────────────────────────────


QJsonObject Stage::saveToJson() const
{
    QJsonObject obj;

    QJsonArray mods;
    for (const AttributeModifier& modifier : modifiers)
        mods.append(modifier.saveToJson());
    obj["modifiers"] = mods;

    QJsonArray children;
    for (const ChildEffectRef& childEffect : childEffects)
        children.append(childEffect.saveToJson());
    obj["childEffects"] = children;

    QJsonArray acts;
    for (const auto& action : actions)
        acts.append(action->saveToJson());
    obj["actions"] = acts;

    return obj;
}

void Stage::loadFromJson(const QJsonObject& obj)
{
    modifiers.clear();
    for (const QJsonValue& jsonValue : obj["modifiers"].toArray())
    {
        AttributeModifier modifier;
        modifier.loadFromJson(jsonValue.toObject());
        modifiers.append(modifier);
    }

    childEffects.clear();
    for (const QJsonValue& jsonValue : obj["childEffects"].toArray())
    {
        ChildEffectRef childEffect;
        childEffect.loadFromJson(jsonValue.toObject());
        childEffects.append(childEffect);
    }

    actions.clear();
    for (const QJsonValue& jsonValue : obj["actions"].toArray())
    {
        const QJsonObject actionObj = jsonValue.toObject();
        auto newAction = ActionFactory::create(actionObj["type"].toString());
        if (newAction)
        {
            newAction->loadFromJson(actionObj);
            actions.append(std::move(newAction));
        }
        else
        {
            qWarning() << "Stage::loadFromJson: unknown action type:" << actionObj["type"].toString();
        }
    }
}

// ─────────────────────────────────────────────
//  Action factory  (delegates to ActionFactory)
// ─────────────────────────────────────────────
#include "Action.h"

// ─────────────────────────────────────────────
//  EffectTemplate
// ─────────────────────────────────────────────

Stage& EffectTemplate::addStage()
{
    stages.append(Stage{});
    return stages.last();
}

void EffectTemplate::removeStage(int index)
{
    if (index >= 0 && index < stages.size())
        stages.removeAt(index);
}

QJsonObject EffectTemplate::saveToJson() const
{
    QJsonObject obj;
    obj["id"]              = id;
    obj["name"]            = name;
    obj["description"]     = description;
    obj["onset"]           = onset;
    obj["pictureFilePath"] = PictureFilePath;

    QJsonArray stageArray;
    for (const Stage& stage : stages)
        stageArray.append(stage.saveToJson());
    obj["stages"] = stageArray;

    return obj;
}

void EffectTemplate::loadFromJson(const QJsonObject& obj)
{
    id              = obj["id"].toString();
    name            = obj["name"].toString();
    description     = obj["description"].toString();
    onset           = obj["onset"].toString();
    PictureFilePath = obj["pictureFilePath"].toString();

    stages.clear();
    for (const QJsonValue& jsonValue : obj["stages"].toArray())
    {
        Stage stage;
        stage.loadFromJson(jsonValue.toObject());
        stages.append(stage);
    }
}

// ─────────────────────────────────────────────
//  EffectInstance
// ─────────────────────────────────────────────

EffectInstance* EffectInstance::fromTemplate(const EffectTemplate& effectTemplate, PawnInstance* owner)
{
    auto* newEffectInstance = new EffectInstance;
    newEffectInstance->templateId  = effectTemplate.id;
    newEffectInstance->instanceId  = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newEffectInstance->owner       = owner;
    newEffectInstance->level       = 0;
    newEffectInstance->currentStageIndex = 0;
    newEffectInstance->currentRound      = 0;

    // Deep-copy stages, cloning every Action so runtime state is independent.
    for (const Stage& srcStage : effectTemplate.stages)
    {
        Stage dstStage;
        dstStage.modifiers   = srcStage.modifiers;
        dstStage.childEffects = srcStage.childEffects;

        for (const auto& srcAction : srcStage.actions)
        {
            dstStage.actions.append(
                std::shared_ptr<Action>(srcAction->clone()));
        }

        newEffectInstance->stages.append(dstStage);
    }

    return newEffectInstance;
}

Stage* EffectInstance::currentStage()
{
    if (stages.isEmpty()) return nullptr;
    const int stageIndex = qBound(0, currentStageIndex, stages.size() - 1);
    return &stages[stageIndex];
}

const Stage* EffectInstance::currentStage() const
{
    if (stages.isEmpty()) return nullptr;
    const int stageIndex = qBound(0, currentStageIndex, stages.size() - 1);
    return &stages[stageIndex];
}

bool EffectInstance::advanceStage()
{
    if (currentStageIndex >= stages.size() - 1) return false;
    const int oldIndex = currentStageIndex;
    ++currentStageIndex;
    if (owner)
        owner->syncBoundChildEffects(this, oldIndex);
    return true;
}

bool EffectInstance::regressStage()
{
    if (currentStageIndex <= 0) return false;
    const int oldIndex = currentStageIndex;
    --currentStageIndex;
    if (owner)
        owner->syncBoundChildEffects(this, oldIndex);
    return true;
}

void EffectInstance::fireEvent(EventType event)
{
    if (event == EventType::OnRoundEnd)
        currentRound++;

    Stage* stage = currentStage();
    if (!stage) return;

    // Snapshot actions via shared_ptr to keep them alive, and capture instanceId
    // locally — MaxDurationAction may call pawn->removeEffect() which destroys
    // this EffectInstance (and its stages) while we are still iterating.
    const QString myId = instanceId;
    const QList<std::shared_ptr<Action>> actionSnapshot = stage->actions;

    for (const auto& action : actionSnapshot)
    {
        // If this effect was removed by an earlier action in the same event, stop.
        if (!owner || !owner->findEffect(myId)) return;
        action->executeEvent(event, owner);
    }
}

QList<QPair<QString,QString>> EffectInstance::getDisplayProperties(const EffectTemplate* tmpl) const
{
    QList<QPair<QString,QString>> displayProperties;

    if (stages.isEmpty())
        displayProperties.append(qMakePair(QString("Stage"), QString("None")));
    else
        displayProperties.append({"Stage", QString("%1 / %2")
                                    .arg(currentStageIndex + 1)
                                    .arg(stages.size())});

    if (level != 0)
        displayProperties.append({"Level", QString::number(level)});

    if (tmpl && !tmpl->onset.isEmpty())
        displayProperties.append({"Onset", tmpl->onset});

    displayProperties.append({"Active", formatDuration(currentRound)});

    const Stage* stage = currentStage();
    if (stage) {
        for (const auto& action : stage->actions)
            displayProperties.append(action->getDisplayProperties());

        for (const AttributeModifier& mod : stage->modifiers) {
            const AttributeRegistry& reg = PathTrackerStruct::attributes();
            QString attrName = reg.hasAttribute(mod.attributeKey)
                             ? reg.getAttribute(mod.attributeKey).displayName
                             : mod.attributeKey;
            QString sign;
            switch (mod.op) {
                case ModifierOp::Plus:     sign = QString("+%1").arg(mod.value); break;
                case ModifierOp::Minus:    sign = QString("-%1").arg(mod.value); break;
                case ModifierOp::Multiply: sign = QString("×%1").arg(mod.value); break;
            }
            QString label = mod.description.isEmpty()
                          ? attrName
                          : QString("%1 (%2)").arg(attrName, mod.description);
            displayProperties.append({label, sign});
        }
    }

    return displayProperties;
}

QWidget* EffectInstance::createRuntimeWidget(QWidget* parent)
{
    // Returns a simple property->value widget row for display in a table.
    auto* container = new QWidget(parent);
    auto* layout    = new QVBoxLayout(container);
    layout->setContentsMargins(4, 2, 4, 2);

    auto addRow = [&](const QString& prop, const QString& val){
        auto* row = new QHBoxLayout;
        auto* lbl = new QLabel(QString("<b>%1</b>").arg(prop), container);
        lbl->setMinimumWidth(130);
        auto* valLbl = new QLabel(val, container);
        row->addWidget(lbl);
        row->addWidget(valLbl);
        layout->addLayout(row);
    };

    addRow("Effect ID",    templateId);
    addRow("Instance ID",  instanceId);
    addRow("Level",        QString::number(level));
    addRow("Stage",        QString::number(currentStageIndex));
    addRow("Round",        QString::number(currentRound));

    // Also add runtime widgets from current stage's actions.
    if (Stage* stage = currentStage())
    {
        for (auto& action : stage->actions)
        {
            QWidget* actionWidget = action->createRuntimeWidget(container);
            if (actionWidget)
                layout->addWidget(actionWidget);
        }
    }

    return container;
}

QJsonObject EffectInstance::saveToJson() const
{
    QJsonObject obj;
    obj["templateId"]        = templateId;
    obj["instanceId"]        = instanceId;
    obj["level"]             = level;
    obj["currentStageIndex"] = currentStageIndex;
    obj["currentRound"]      = currentRound;

    // We save runtime stage state (action internals like roundsRemaining).
    QJsonArray stageArray;
    for (const Stage& stage : stages)
        stageArray.append(stage.saveToJson());
    obj["stages"] = stageArray;

    return obj;
}

void EffectInstance::loadFromJson(const QJsonObject& obj)
{
    templateId        = obj["templateId"].toString();
    instanceId        = obj["instanceId"].toString();
    level             = obj["level"].toInt(0);
    currentStageIndex = obj["currentStageIndex"].toInt(0);
    currentRound      = obj["currentRound"].toInt(0);
    owner             = nullptr;   // re-linked by PawnInstance::loadFromJson

    stages.clear();
    for (const QJsonValue& jsonValue : obj["stages"].toArray())
    {
        Stage stage;
        stage.loadFromJson(jsonValue.toObject());
        stages.append(stage);
    }
}
