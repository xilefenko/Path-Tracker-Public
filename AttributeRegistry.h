#pragma once
#include "Attribute.h"
#include <QMap>
#include <QJsonArray>

// ─────────────────────────────────────────────
//  AttributeRegistry  (Singleton)
//  Stores all Attribute definitions keyed by their string key.
//  Higher-level objects store only the key and look up here.
// ─────────────────────────────────────────────
class AttributeRegistry
{
public:
    static AttributeRegistry& instance();

    // CRUD
    void      registerAttribute(const Attribute& attr);
    bool      hasAttribute(const QString& key) const;
    Attribute getAttribute(const QString& key) const;   // asserts if missing
    QList<Attribute> allAttributes() const;
    void      removeAttribute(const QString& key);

    // Serialization
    QJsonArray  saveToJson() const;
    void        loadFromJson(const QJsonArray& arr);

private:
    AttributeRegistry() = default;
    QMap<QString, Attribute> m_attributes;
};
