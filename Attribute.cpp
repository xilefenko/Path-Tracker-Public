#include "Attribute.h"
#include <QJsonObject>

// ─────────────────────────────────────────────
//  Attribute
// ─────────────────────────────────────────────

QJsonObject Attribute::saveToJson() const
{
    QJsonObject obj;
    obj["key"]          = key;
    obj["displayName"]  = displayName;
    obj["defaultValue"] = defaultValue;
    return obj;
}

void Attribute::loadFromJson(const QJsonObject& obj)
{
    key          = obj["key"].toString();
    displayName  = obj["displayName"].toString();
    defaultValue = obj["defaultValue"].toInt(0);
}

// ─────────────────────────────────────────────
//  AttributeValue
// ─────────────────────────────────────────────

QJsonObject AttributeValue::saveToJson() const
{
    QJsonObject obj;
    obj["key"]          = key;
    obj["baseValue"]    = baseValue;
    obj["currentValue"] = currentValue;
    return obj;
}

void AttributeValue::loadFromJson(const QJsonObject& obj)
{
    key          = obj["key"].toString();
    baseValue    = obj["baseValue"].toInt(0);
    currentValue = obj["currentValue"].toInt(0);
}

// ─────────────────────────────────────────────
//  AttributeModifier
// ─────────────────────────────────────────────

int AttributeModifier::apply(int base) const
{
    switch (op)
    {
    case ModifierOp::Plus:     return base + value;
    case ModifierOp::Minus:    return base - value;
    case ModifierOp::Multiply: return base * value;
    }
    return base;
}

QJsonObject AttributeModifier::saveToJson() const
{
    QJsonObject obj;
    obj["attributeKey"] = attributeKey;
    obj["value"]        = value;

    switch (op)
    {
    case ModifierOp::Plus:     obj["op"] = "Plus";     break;
    case ModifierOp::Minus:    obj["op"] = "Minus";    break;
    case ModifierOp::Multiply: obj["op"] = "Multiply"; break;
    }

    obj["description"] = description;
    return obj;
}

void AttributeModifier::loadFromJson(const QJsonObject& obj)
{
    attributeKey = obj["attributeKey"].toString();
    value        = obj["value"].toInt(0);

    const QString opStr = obj["op"].toString();
    if      (opStr == "Plus")     op = ModifierOp::Plus;
    else if (opStr == "Minus")    op = ModifierOp::Minus;
    else if (opStr == "Multiply") op = ModifierOp::Multiply;
    else                          op = ModifierOp::Plus;   // safe default

    description = obj["description"].toString();
}
