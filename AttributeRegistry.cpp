#include "AttributeRegistry.h"
#include <QJsonArray>
#include <QDebug>

// ─────────────────────────────────────────────
//  AttributeRegistry  (Singleton)
// ─────────────────────────────────────────────

AttributeRegistry& AttributeRegistry::instance()
{
    static AttributeRegistry s_instance;
    return s_instance;
}

void AttributeRegistry::registerAttribute(const Attribute& attr)
{
    if (attr.key.isEmpty())
    {
        qWarning() << "AttributeRegistry: attempted to register attribute with empty key – ignored.";
        return;
    }
    m_attributes[attr.key] = attr;
}

bool AttributeRegistry::hasAttribute(const QString& key) const
{
    return m_attributes.contains(key);
}

Attribute AttributeRegistry::getAttribute(const QString& key) const
{
    Q_ASSERT_X(m_attributes.contains(key), "AttributeRegistry::getAttribute",
               qPrintable("Unknown attribute key: " + key));
    return m_attributes.value(key);
}

QList<Attribute> AttributeRegistry::allAttributes() const
{
    return m_attributes.values();
}

void AttributeRegistry::removeAttribute(const QString& key)
{
    m_attributes.remove(key);
}

QJsonArray AttributeRegistry::saveToJson() const
{
    QJsonArray arr;
    for (const Attribute& attr : m_attributes)
        arr.append(attr.saveToJson());
    return arr;
}

void AttributeRegistry::loadFromJson(const QJsonArray& arr)
{
    m_attributes.clear();
    for (const QJsonValue& val : arr)
    {
        Attribute attr;
        attr.loadFromJson(val.toObject());
        if (!attr.key.isEmpty())
            m_attributes[attr.key] = attr;
    }
}
