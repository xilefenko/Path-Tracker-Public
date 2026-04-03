#pragma once
#include <QString>
#include <QJsonObject>
#include <QUuid>

// ─────────────────────────────────────────────
//  Attribute  (flat key-value definition)
//  Lives in AttributeRegistry. Referenced by key.
// ─────────────────────────────────────────────
struct Attribute
{
    QString key;          // e.g. "Strength", "AC", "HP"
    QString displayName;
    int     defaultValue = 0;

    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);
};

// ─────────────────────────────────────────────
//  AttributeValue  (key + live integer)
//  Used inside PawnInstance to track current stats.
// ─────────────────────────────────────────────
struct AttributeValue
{
    QString key;
    int     baseValue    = 0;
    int     currentValue = 0;   // written by ResolveAttributes()

    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);
};

// ─────────────────────────────────────────────
//  AttributeModifier
//  Created in Editor. Immutable at runtime.
//  Referenced by Effect Stages and PawnTemplates.
// ─────────────────────────────────────────────
enum class ModifierOp { Plus, Minus, Multiply };

struct AttributeModifier
{
    QString     attributeKey;
    ModifierOp  op    = ModifierOp::Plus;
    int         value = 0;
    QString     description;   // context label, e.g. "Frightened"

    // Returns the modified value given a base
    int apply(int base) const;

    QJsonObject saveToJson() const;
    void        loadFromJson(const QJsonObject& obj);
};
