#include "PathTrackerStruct.h"

#include <QJsonDocument>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QUuid>

// ─────────────────────────────────────────────────────────────────────────────
//  JSON document layout
//
//  {
//    "savedAt": "2026-02-23 14:05:00",
//
//    "editorLayer": {
//      "attributes":         [ ...AttributeRegistry entries... ],
//      "effectTemplates":    [ ...EffectTemplate objects... ],
//      "pawnTemplates":      [ ...PawnTemplate objects... ],
//      "encounterTemplates": [ ...EncounterTemplate objects... ]
//    },
//
//    "runtimeLayer": {
//      "players":         [ ...Player objects from PlayerRegistry... ],
//      "activeEncounter": { ...EncounterInstance snapshot... }   <- omitted if null
//    }
//  }
// ─────────────────────────────────────────────────────────────────────────────

// ═══════════════════════════════════════════════════════════════════════════════
//  AppTheme — presets
// ═══════════════════════════════════════════════════════════════════════════════

AppTheme AppTheme::darkPreset()
{
    // Default-constructed AppTheme is already the dark preset
    AppTheme t;
    t.themeMode = ThemeMode::Dark;
    return t;
}

AppTheme AppTheme::lightPreset()
{
    AppTheme t;
    t.themeMode           = ThemeMode::Light;
    t.windowBackground    = QColor("#f0f0f0");
    t.textColor           = QColor("#1a1a1a");
    t.accentColor         = QColor("#1a8a40");
    t.buttonBackground    = QColor("#d0d0d0");
    t.buttonTextColor     = QColor("#1a1a1a");
    t.inputBackground     = QColor("#ffffff");
    t.pawnWaitingColor    = QColor("#f0f0f0");
    t.pawnActiveColor     = QColor("#c8f0c8");
    t.pawnSpentColor      = QColor("#e0e0e0");
    t.pawnUnconsciousColor= QColor("#ffd0d0");
    t.pawnDeadColor       = QColor("#c0c0c0");
    return t;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  AppTheme
// ═══════════════════════════════════════════════════════════════════════════════

QString AppTheme::toStylesheet() const
{
    // Derived colors
    const QColor hoverBg      = buttonBackground.lighter(120);
    const QColor inputBorder  = inputBackground.lighter(140);
    const QColor hoverItem    = buttonBackground;
    const QColor pbGroove     = windowBackground.darker(130);
    const QColor scrollTrack  = windowBackground.darker(115);
    const QColor scrollHandle = buttonBackground.lighter(110);
    const QColor groupBorder  = inputBackground.lighter(130);

    const QString selText = (accentColor.lightness() > 128) ? "#111111" : "#eeeeee";

    const QString win   = windowBackground.name();
    const QString txt   = textColor.name();
    const QString btnBg = buttonBackground.name();
    const QString btnTx = buttonTextColor.name();
    const QString inp   = inputBackground.name();
    const QString bord  = inputBorder.name();
    const QString acc   = accentColor.name();

    QString ss;

    ss += QString(
        "QMainWindow, QDialog, QWidget { background-color: %1; color: %2; font-size: %3pt; }"
        "QPushButton { background-color: %4; color: %5; border-radius: %6px; padding: %7px; font-size: %3pt; }"
        "QPushButton:hover { background-color: %8; }"
        "QPushButton:disabled { background-color: %11; color: %12; }"
        "QLabel { color: %2; }"
        "QListWidget, QTreeWidget { background-color: %1; color: %2; }"
        "QSpinBox, QDoubleSpinBox, QComboBox, QLineEdit, QPlainTextEdit, QTextEdit {"
        "    background-color: %9; color: %2; border: 1px solid %10; }"
        "QComboBox QAbstractItemView { background-color: %9; color: %2; }"
    )
    .arg(win)           // %1
    .arg(txt)           // %2
    .arg(fontSize)      // %3
    .arg(btnBg)         // %4
    .arg(btnTx)         // %5
    .arg(buttonBorderRadius) // %6
    .arg(buttonPadding) // %7
    .arg(hoverBg.name()) // %8
    .arg(inp)           // %9
    .arg(bord)          // %10
    .arg(windowBackground.darker(110).name())  // %11 disabled button bg
    .arg(textColor.darker(160).name());        // %12 disabled button text

    // Selection highlights
    ss += QString(
        "QListWidget::item:selected, QTreeWidget::item:selected,"
        "QTableWidget::item:selected, QComboBox QAbstractItemView::item:selected {"
        "    background-color: %1; color: %2; }"
    ).arg(acc, selText);

    // QTreeView alternating rows and selection (QTreeWidget inherits QTreeView)
    ss += QString(
        "QTreeView { alternate-background-color: %1; }"
        "QTreeView::item { background-color: transparent; }"
        "QTreeView::item:selected { background-color: %2; color: %3; }"
    ).arg(windowBackground.darker(115).name(), acc, selText);

    // Focus borders for all input widgets
    ss += QString(
        "QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus,"
        "QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus {"
        "    border: 1px solid %1; }"
    ).arg(acc);

    // Item hover
    ss += QString(
        "QListWidget::item:hover, QTreeWidget::item:hover { background-color: %1; }"
    ).arg(hoverItem.name());

    // QTableWidget
    ss += QString(
        "QTableWidget { background-color: %1; color: %2; }"
        "QTableWidget QTableCornerButton::section { background-color: %1; }"
        "QHeaderView::section { background-color: %3; color: %2; border: 1px solid %4; }"
    ).arg(win, txt, inp, bord);

    // QGroupBox
    ss += QString(
        "QGroupBox { border: 1px solid %1; border-radius: 4px; margin-top: 6px; }"
        "QGroupBox::title { color: %2; subcontrol-origin: margin; left: 8px; }"
    ).arg(groupBorder.name(), txt);

    // QProgressBar (HP bar chunk uses accent color)
    ss += QString(
        "QProgressBar { background-color: %1; color: %2; border: 1px solid %3; border-radius: 3px; text-align: center; }"
        "QProgressBar::chunk { background-color: %4; border-radius: 3px; }"
    ).arg(pbGroove.name(), txt, bord, acc);

    // QCheckBox
    ss += QString(
        "QCheckBox { color: %1; }"
        "QCheckBox::indicator { width: 14px; height: 14px; background-color: %2; border: 1px solid %3; }"
        "QCheckBox::indicator:checked { background-color: %4; }"
    ).arg(txt, inp, bord, acc);

    // QScrollBar (vertical)
    ss += QString(
        "QScrollBar:vertical { background: %1; width: 12px; }"
        "QScrollBar::handle:vertical { background: %2; min-height: 20px; border-radius: 4px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    ).arg(scrollTrack.name(), scrollHandle.name());

    // QScrollBar (horizontal)
    ss += QString(
        "QScrollBar:horizontal { background: %1; height: 12px; }"
        "QScrollBar::handle:horizontal { background: %2; min-width: 20px; border-radius: 4px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
    ).arg(scrollTrack.name(), scrollHandle.name());

    // QSlider — sub-page and add-page must be styled explicitly; without them
    // Windows' hit-testing for the groove breaks and the slider becomes unresponsive.
    ss += QString(
        "QSlider::groove:horizontal { background: %1; height: 4px; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: %3; height: 4px; border-radius: 2px; }"
        "QSlider::add-page:horizontal { background: %1; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: %2; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }"
    ).arg(inp, btnBg, acc);

    // QMenuBar / QMenu / QStatusBar
    ss += QString(
        "QMenuBar { background-color: %1; color: %2; }"
        "QMenuBar::item:selected { background-color: %3; color: %4; }"
        "QMenu { background-color: %5; color: %2; }"
        "QMenu::item:selected { background-color: %3; color: %4; }"
        "QStatusBar { background-color: %1; color: %2; }"
    ).arg(win, txt, acc, selText, inp);

    // QTabWidget / QTabBar
    ss += QString(
        "QTabWidget::pane { background-color: %1; border: 1px solid %2; }"
        "QTabBar::tab { background-color: %3; color: %4; padding: 4px 10px; border: 1px solid %2; }"
        "QTabBar::tab:selected { background-color: %5; color: %6; }"
    ).arg(win, bord, inp, txt, acc, selText);

    // Editor portrait/icon preview borders
    ss += QString(
        "QLabel#PortraitPreview, QLabel#IconPreview {"
        "    border: 2px solid %1; border-radius: 2px; }"
    ).arg(acc);

    return ss;
}

QJsonObject AppTheme::saveToJson() const
{
    QJsonObject obj;
    obj["themeMode"]          = static_cast<int>(themeMode);
    obj["windowBackground"]   = windowBackground.name();
    obj["textColor"]          = textColor.name();
    obj["accentColor"]        = accentColor.name();
    obj["buttonBackground"]   = buttonBackground.name();
    obj["buttonTextColor"]    = buttonTextColor.name();
    obj["buttonBorderRadius"] = buttonBorderRadius;
    obj["buttonPadding"]      = buttonPadding;
    obj["fontSize"]           = fontSize;
    obj["inputBackground"]    = inputBackground.name();
    obj["pawnWaitingColor"]     = pawnWaitingColor.name();
    obj["pawnActiveColor"]      = pawnActiveColor.name();
    obj["pawnSpentColor"]       = pawnSpentColor.name();
    obj["pawnUnconsciousColor"] = pawnUnconsciousColor.name();
    obj["pawnDeadColor"]        = pawnDeadColor.name();
    return obj;
}

void AppTheme::loadFromJson(const QJsonObject& obj)
{
    if (obj.contains("themeMode"))
        themeMode = static_cast<ThemeMode>(obj["themeMode"].toInt(0));
    if (obj.contains("windowBackground"))
        windowBackground = QColor(obj["windowBackground"].toString());
    if (obj.contains("textColor"))
        textColor = QColor(obj["textColor"].toString());
    if (obj.contains("accentColor"))
        accentColor = QColor(obj["accentColor"].toString());
    if (obj.contains("buttonBackground"))
        buttonBackground = QColor(obj["buttonBackground"].toString());
    if (obj.contains("buttonTextColor"))
        buttonTextColor = QColor(obj["buttonTextColor"].toString());
    if (obj.contains("buttonBorderRadius"))
        buttonBorderRadius = obj["buttonBorderRadius"].toInt();
    if (obj.contains("buttonPadding"))
        buttonPadding = obj["buttonPadding"].toInt();
    if (obj.contains("fontSize"))
        fontSize = obj["fontSize"].toInt();
    if (obj.contains("inputBackground"))
        inputBackground = QColor(obj["inputBackground"].toString());
    if (obj.contains("pawnWaitingColor"))
        pawnWaitingColor = QColor(obj["pawnWaitingColor"].toString());
    if (obj.contains("pawnActiveColor"))
        pawnActiveColor = QColor(obj["pawnActiveColor"].toString());
    if (obj.contains("pawnSpentColor"))
        pawnSpentColor = QColor(obj["pawnSpentColor"].toString());
    if (obj.contains("pawnUnconsciousColor"))
        pawnUnconsciousColor = QColor(obj["pawnUnconsciousColor"].toString());
    if (obj.contains("pawnDeadColor"))
        pawnDeadColor = QColor(obj["pawnDeadColor"].toString());
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Singleton
// ═══════════════════════════════════════════════════════════════════════════════

PathTrackerStruct& PathTrackerStruct::instance()
{
    static PathTrackerStruct s_instance;
    return s_instance;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Save
// ═══════════════════════════════════════════════════════════════════════════════

void PathTrackerStruct::save(const QString& filePath) const
{
    qDebug() << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "]"
             << "PathTrackerStruct::save ->" << filePath;

    const QJsonObject root = buildSaveDocument();

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "PathTrackerStruct::save – could not open file for writing:" << filePath;
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "PathTrackerStruct::save – done.";
}

QJsonObject PathTrackerStruct::buildSaveDocument() const
{
    QJsonObject root;
    root["savedAt"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // ── App settings ─────────────────────────────────────────────────────────
    QJsonObject settings;
    settings["stylesheet"] = m_stylesheet;
    settings["theme"]      = m_theme.saveToJson();
    root["appSettings"] = settings;

    // ── Editor layer ─────────────────────────────────────────────────────────
    QJsonObject editor;

    // 1. Attribute definitions (from singleton registry)
    editor["attributes"] = AttributeRegistry::instance().saveToJson();

    // 2. Effect templates
    QJsonArray effectArr;
    for (const auto& [id, tmpl] : m_effectTemplates)
        effectArr.append(tmpl->saveToJson());
    editor["effectTemplates"] = effectArr;

    // 3. Pawn templates
    QJsonArray pawnArr;
    for (const auto& [id, tmpl] : m_pawnTemplates)
        pawnArr.append(tmpl->saveToJson());
    editor["pawnTemplates"] = pawnArr;

    // 4. Encounter templates
    QJsonArray encTmplArr;
    for (const auto& [id, tmpl] : m_encounterTemplates)
        encTmplArr.append(tmpl->saveToJson());
    editor["encounterTemplates"] = encTmplArr;

    root["editorLayer"] = editor;

    // ── Runtime layer ────────────────────────────────────────────────────────
    QJsonObject runtime;

    // 5. Players (from singleton registry)
    runtime["players"] = PlayerRegistry::instance().saveToJson();

    // 6. Active encounter (optional – only written when one exists)
    if (m_activeEncounter)
        runtime["activeEncounter"] = m_activeEncounter->saveToJson();

    root["runtimeLayer"] = runtime;

    return root;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Load
// ═══════════════════════════════════════════════════════════════════════════════

bool PathTrackerStruct::load(const QString& filePath)
{
    qDebug() << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "]"
             << "PathTrackerStruct::load <-" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "PathTrackerStruct::load – file not found or unreadable:" << filePath;
        return false;
    }

    const QByteArray   bytes = file.readAll();
    file.close();

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (doc.isNull() || !doc.isObject())
    {
        qWarning() << "PathTrackerStruct::load – invalid JSON in" << filePath;
        return false;
    }

    return parseSaveDocument(doc.object());
}

bool PathTrackerStruct::parseSaveDocument(const QJsonObject& root)
{
    // ── Clear existing state first ────────────────────────────────────────────
    m_effectTemplates.clear();
    m_pawnTemplates.clear();
    m_encounterTemplates.clear();
    m_activeEncounter.reset();
    m_stylesheet.clear();
    m_theme = AppTheme{};

    // ── App settings ─────────────────────────────────────────────────────────
    if (root.contains("appSettings"))
    {
        const QJsonObject settings = root["appSettings"].toObject();
        if (settings.contains("theme"))
        {
            // Structured theme: regenerate QSS from parameters
            m_theme.loadFromJson(settings["theme"].toObject());
            m_stylesheet = m_theme.toStylesheet();
        }
        else
        {
            // Legacy: raw QSS only, theme stays at defaults
            m_stylesheet = settings["stylesheet"].toString();
        }
    }

    // ── Editor layer ─────────────────────────────────────────────────────────
    const QJsonObject editor = root["editorLayer"].toObject();

    // 1. Attribute definitions → singleton registry
    AttributeRegistry::instance().loadFromJson(editor["attributes"].toArray());

    // 2. Effect templates
    for (const QJsonValue& v : editor["effectTemplates"].toArray())
    {
        auto tmpl = std::make_unique<EffectTemplate>();
        tmpl->loadFromJson(v.toObject());
        const QString id = tmpl->id;
        m_effectTemplates[id] = std::move(tmpl);
    }

    // 3. Pawn templates
    for (const QJsonValue& v : editor["pawnTemplates"].toArray())
    {
        auto tmpl = std::make_unique<PawnTemplate>();
        tmpl->loadFromJson(v.toObject());
        const QString id = tmpl->id;
        m_pawnTemplates[id] = std::move(tmpl);
    }

    // 4. Encounter templates
    for (const QJsonValue& v : editor["encounterTemplates"].toArray())
    {
        auto tmpl = std::make_unique<EncounterTemplate>();
        tmpl->loadFromJson(v.toObject());
        const QString id = tmpl->id;
        m_encounterTemplates[id] = std::move(tmpl);
    }

    // ── Runtime layer ────────────────────────────────────────────────────────
    const QJsonObject runtime = root["runtimeLayer"].toObject();

    // 5. Players → singleton registry
    //    Must be loaded BEFORE the active encounter so that player
    //    instanceId pointers can be resolved inside EncounterInstance::loadFromJson.
    PlayerRegistry::instance().loadFromJson(runtime["players"].toArray());

    // 6. Active encounter (optional)
    if (runtime.contains("activeEncounter"))
    {
        auto encounter = std::unique_ptr<EncounterInstance>(new EncounterInstance());
        encounter->loadFromJson(runtime["activeEncounter"].toObject());
        m_activeEncounter = std::move(encounter);
    }

    // Ensure mandatory attributes always exist in the registry
    auto ensureAttr = [](const char* key, const char* displayName, int defaultValue) {
        if (!AttributeRegistry::instance().hasAttribute(key)) {
            Attribute attr;
            attr.key          = key;
            attr.displayName  = displayName;
            attr.defaultValue = defaultValue;
            AttributeRegistry::instance().registerAttribute(attr);
        }
    };
    ensureAttr(HP_KEY, "Hit Points",  10);
    ensureAttr(AC_KEY, "Armor Class", 10);

    qDebug() << "PathTrackerStruct::load – done."
             << "Attributes:"        << AttributeRegistry::instance().allAttributes().size()
             << "EffectTemplates:"   << m_effectTemplates.size()
             << "PawnTemplates:"     << m_pawnTemplates.size()
             << "EncounterTemplates:" << m_encounterTemplates.size()
             << "Players:"           << PlayerRegistry::instance().allPlayers().size()
             << "ActiveEncounter:"   << (m_activeEncounter ? m_activeEncounter->instanceId : "(none)");

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Legacy migration
// ═══════════════════════════════════════════════════════════════════════════════

bool PathTrackerStruct::loadLegacy(const QString& filePath)
{
    qDebug() << "PathTrackerStruct::loadLegacy <-" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "PathTrackerStruct::loadLegacy – file not found:" << filePath;
        return false;
    }

    const QByteArray   bytes = file.readAll();
    file.close();

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (doc.isNull() || !doc.isObject())
    {
        qWarning() << "PathTrackerStruct::loadLegacy – invalid JSON in" << filePath;
        return false;
    }

    const QJsonObject root = doc.object();

    auto stripBraces = [](const QString& id) -> QString {
        QString s = id;
        if (s.startsWith('{')) s = s.mid(1);
        if (s.endsWith('}'))   s.chop(1);
        return s;
    };

    // Clear existing state
    m_effectTemplates.clear();
    m_pawnTemplates.clear();
    m_encounterTemplates.clear();
    m_activeEncounter.reset();
    AttributeRegistry::instance().loadFromJson(QJsonArray());

    // ── Attributes ───────────────────────────────────────────────────────────
    for (const QJsonValue& v : root["AttributeData"].toArray())
    {
        const QJsonObject obj = v.toObject();
        Attribute attr;
        attr.key          = stripBraces(obj["ID"].toString());
        attr.displayName  = obj["Name"].toString();
        attr.defaultValue = 0;
        AttributeRegistry::instance().registerAttribute(attr);
    }

    // ── Effect templates ─────────────────────────────────────────────────────
    for (const QJsonValue& ev : root["EffectData"].toArray())
    {
        const QJsonObject eff = ev.toObject();

        auto tmpl       = std::make_unique<EffectTemplate>();
        tmpl->id          = stripBraces(eff["ID"].toString());
        tmpl->name        = eff["Name"].toString();
        tmpl->description = eff["Description"].toString();
        tmpl->onset       = eff["Onset"].toString();

        for (const QJsonValue& rv : eff["Rounds"].toArray())
        {
            const QJsonObject round = rv.toObject();
            Stage stage;

            for (const QJsonValue& av : round["Attributes"].toArray())
            {
                const QJsonObject aobj = av.toObject();
                AttributeModifier mod;
                mod.attributeKey = stripBraces(aobj["ID"].toString());
                mod.op    = (aobj["Operrand"].toInt() == 1) ? ModifierOp::Minus : ModifierOp::Plus;
                mod.value = aobj["Value"].toInt();
                stage.modifiers.append(mod);
            }

            for (const QJsonValue& ce : round["AdditionalEffects"].toArray())
            {
                const QJsonObject ceobj = ce.toObject();
                ChildEffectRef ref;
                ref.effectTemplateId = stripBraces(ceobj["ID"].toString());
                ref.binding = ceobj["Bound"].toBool() ? ChildBinding::Bound : ChildBinding::Unbound;
                stage.childEffects.append(ref);
            }

            for (const QJsonValue& acv : round["Actions"].toArray())
            {
                const QJsonObject acobj = acv.toObject();
                if (acobj["ActionType"].toInt() == 0)
                {
                    auto action = std::make_shared<DamageThrowAction>();
                    action->diceFormula = acobj["Dice"].toString();
                    action->damageType  = acobj["Definition"].toString();
                    stage.actions.append(action);
                }
                // ActionType 1 (saving-throw reminders) dropped – no new-arch equivalent
            }

            tmpl->stages.append(stage);
        }

        const QString id = tmpl->id;
        m_effectTemplates[id] = std::move(tmpl);
    }

    qDebug() << "PathTrackerStruct::loadLegacy – done."
             << "Attributes:"      << AttributeRegistry::instance().allAttributes().size()
             << "EffectTemplates:" << m_effectTemplates.size();

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  App settings
// ═══════════════════════════════════════════════════════════════════════════════

void PathTrackerStruct::setTheme(const AppTheme& theme)
{
    m_theme      = theme;
    m_stylesheet = m_theme.toStylesheet();
    save();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Effect template CRUD
// ═══════════════════════════════════════════════════════════════════════════════

void PathTrackerStruct::addEffectTemplate(std::unique_ptr<EffectTemplate> tmpl)
{
    Q_ASSERT(tmpl);
    if (tmpl->id.isEmpty())
        tmpl->id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QString id = tmpl->id;
    m_effectTemplates[id] = std::move(tmpl);
}

void PathTrackerStruct::removeEffectTemplate(const QString& id)
{
    m_effectTemplates.erase(id);
}

EffectTemplate* PathTrackerStruct::findEffectTemplate(const QString& id) const
{
    auto it = m_effectTemplates.find(id);
    return (it != m_effectTemplates.end()) ? it->second.get() : nullptr;
}

QList<EffectTemplate*> PathTrackerStruct::allEffectTemplates() const
{
    QList<EffectTemplate*> result;
    result.reserve(m_effectTemplates.size());
    for (const auto& [id, tmpl] : m_effectTemplates)
        result.append(tmpl.get());
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Pawn template CRUD
// ═══════════════════════════════════════════════════════════════════════════════

void PathTrackerStruct::addPawnTemplate(std::unique_ptr<PawnTemplate> tmpl)
{
    Q_ASSERT(tmpl);
    if (tmpl->id.isEmpty())
        tmpl->id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QString id = tmpl->id;
    m_pawnTemplates[id] = std::move(tmpl);
}

void PathTrackerStruct::removePawnTemplate(const QString& id)
{
    m_pawnTemplates.erase(id);
}

PawnTemplate* PathTrackerStruct::findPawnTemplate(const QString& id) const
{
    auto it = m_pawnTemplates.find(id);
    return (it != m_pawnTemplates.end()) ? it->second.get() : nullptr;
}

QList<PawnTemplate*> PathTrackerStruct::allPawnTemplates() const
{
    QList<PawnTemplate*> result;
    result.reserve(m_pawnTemplates.size());
    for (const auto& [id, tmpl] : m_pawnTemplates)
        result.append(tmpl.get());
    return result;
}

QMap<QString, PawnTemplate*> PathTrackerStruct::pawnTemplateLookup() const
{
    QMap<QString, PawnTemplate*> map;
    for (const auto& [id, tmpl] : m_pawnTemplates)
        map[id] = tmpl.get();
    return map;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Encounter template CRUD
// ═══════════════════════════════════════════════════════════════════════════════

void PathTrackerStruct::addEncounterTemplate(std::unique_ptr<EncounterTemplate> tmpl)
{
    Q_ASSERT(tmpl);
    if (tmpl->id.isEmpty())
        tmpl->id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QString id = tmpl->id;
    m_encounterTemplates[id] = std::move(tmpl);
}

void PathTrackerStruct::removeEncounterTemplate(const QString& id)
{
    m_encounterTemplates.erase(id);
}

EncounterTemplate* PathTrackerStruct::findEncounterTemplate(const QString& id) const
{
    auto it = m_encounterTemplates.find(id);
    return (it != m_encounterTemplates.end()) ? it->second.get() : nullptr;
}

QList<EncounterTemplate*> PathTrackerStruct::allEncounterTemplates() const
{
    QList<EncounterTemplate*> result;
    result.reserve(m_encounterTemplates.size());
    for (const auto& [id, tmpl] : m_encounterTemplates)
        result.append(tmpl.get());
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Runtime: active encounter
// ═══════════════════════════════════════════════════════════════════════════════

void PathTrackerStruct::startEncounter(const QString& encounterTemplateId)
{
    EncounterTemplate* tmpl = findEncounterTemplate(encounterTemplateId);
    if (!tmpl)
    {
        qWarning() << "PathTrackerStruct::startEncounter – template not found:"
                   << encounterTemplateId;
        return;
    }

    // Build pawn lookup and instantiate.
    m_activeEncounter.reset(
        EncounterInstance::fromTemplate(*tmpl, pawnTemplateLookup()));

    qDebug() << "PathTrackerStruct::startEncounter – started encounter:"
             << tmpl->name << "instance:" << m_activeEncounter->instanceId;
}

void PathTrackerStruct::startEmptyEncounter(const QString& name)
{
    auto inst        = std::unique_ptr<EncounterInstance>(new EncounterInstance());
    inst->instanceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    inst->name       = name;
    m_activeEncounter = std::move(inst);
}

void PathTrackerStruct::setActiveEncounter(std::unique_ptr<EncounterInstance> encounter)
{
    m_activeEncounter = std::move(encounter);
}

void PathTrackerStruct::clearActiveEncounter()
{
    m_activeEncounter.reset();
}

EncounterInstance* PathTrackerStruct::activeEncounter()
{
    return m_activeEncounter.get();
}

const EncounterInstance* PathTrackerStruct::activeEncounter() const
{
    return m_activeEncounter.get();
}
