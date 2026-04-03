#pragma once

// ── Mandatory attribute keys shared across the whole application ──────────────
inline constexpr const char* HP_KEY = "HitPoints";
inline constexpr const char* AC_KEY = "ArmorClass";

#include "AttributeRegistry.h"
#include "Effect.h"
#include "Pawn.h"
#include "Encounter.h"

#include <QString>
#include <QMap>
#include <QList>
#include <map>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QColor>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
//  AppTheme
//
//  All configurable visual parameters. toStylesheet() generates QSS from them.
//  Stored as "appSettings.theme" in Save.json.
// ─────────────────────────────────────────────────────────────────────────────

// Which colour mode is active.
// Dark/Light store fully customisable colours; System auto-follows the OS palette.
enum class ThemeMode { Dark = 0, Light = 1, System = 2 };

struct AppTheme
{
    ThemeMode themeMode = ThemeMode::Dark;

    QColor windowBackground  = QColor("#2b2b2b");
    QColor textColor         = QColor("#dddddd");
    QColor accentColor       = QColor("#2ecc71");
    QColor buttonBackground  = QColor("#3c3c3c");
    QColor buttonTextColor   = QColor("#ffffff");
    int    buttonBorderRadius = 4;   // px, range 0–20
    int    buttonPadding      = 4;   // px, range 0–20
    int    fontSize           = 10;  // pt, range 6–24

    // Input field backgrounds (QSpinBox, QComboBox, QLineEdit, QPlainTextEdit, QTextEdit)
    QColor inputBackground    = QColor("#404040");

    // Per-state pawn row background colors
    QColor pawnWaitingColor     = QColor("#2b2b2b");
    QColor pawnActiveColor      = QColor("#1a5c1a");
    QColor pawnSpentColor       = QColor("#252525");
    QColor pawnUnconsciousColor = QColor("#3d0000");
    QColor pawnDeadColor        = QColor("#111111");

    // Built-in presets — returned by value so callers can customise further
    static AppTheme darkPreset();
    static AppTheme lightPreset();

    QString     toStylesheet() const;
    QJsonObject saveToJson()   const;
    void        loadFromJson(const QJsonObject&);
};

// ─────────────────────────────────────────────────────────────────────────────
//  PathTrackerStruct
//
//  Single owner of all persistent application state. Divided into two clear
//  layers that mirror the Template / Instance split:
//
//  EDITOR LAYER  (templates – survive between sessions, never mutated at runtime)
//  ├─ AttributeRegistry::instance()   – Attribute definitions (singleton, not owned here)
//  ├─ effectTemplates                 – EffectTemplate definitions keyed by id
//  ├─ pawnTemplates                   – PawnTemplate definitions keyed by id
//  └─ encounterTemplates              – EncounterTemplate definitions keyed by id
//
//  RUNTIME LAYER  (instances – current session state)
//  ├─ PlayerRegistry::instance()      – Back Bench Players (singleton, not owned here)
//  └─ activeEncounter                 – The running EncounterInstance (may be nullptr)
//
//  SAVE / LOAD
//  └─ save(path) / load(path)         – Single JSON file, all layers in one document
//
//  Usage:
//      PathTrackerStruct& app = PathTrackerStruct::instance();
//      app.load();          // on startup
//      app.save();          // on close / Ctrl+S
// ─────────────────────────────────────────────────────────────────────────────

class PathTrackerStruct
{
public:
    // ── Singleton access ─────────────────────────────────────────────────────
    static PathTrackerStruct& instance();

    // Non-copyable, non-movable (singleton)
    PathTrackerStruct(const PathTrackerStruct&)            = delete;
    PathTrackerStruct& operator=(const PathTrackerStruct&) = delete;

    // ── Save / Load ──────────────────────────────────────────────────────────

    // Serialises all editor templates + runtime state to a single JSON file.
    // Default path is "Save.json" in the working directory.
    void save(const QString& filePath = "Save.json") const;

    // Deserialises the file back into all registries and runtime state.
    // Clears existing data first. Returns false if the file could not be read.
    bool load(const QString& filePath = "Save.json");

    // One-shot migration: reads OldSave.json schema, populates registries.
    // Call save() afterward to produce a valid Save.json.
    bool loadLegacy(const QString& filePath = "OldSave.json");

    // ── Editor layer: Effect templates ───────────────────────────────────────
    // Full CRUD for EffectTemplates. Keyed by EffectTemplate::id.

    void            addEffectTemplate(std::unique_ptr<EffectTemplate> tmpl);
    void            removeEffectTemplate(const QString& id);
    EffectTemplate* findEffectTemplate(const QString& id) const;
    QList<EffectTemplate*> allEffectTemplates() const;

    // ── Editor layer: Pawn templates ─────────────────────────────────────────
    // Full CRUD for PawnTemplates. Keyed by PawnTemplate::id.

    void           addPawnTemplate(std::unique_ptr<PawnTemplate> tmpl);
    void           removePawnTemplate(const QString& id);
    PawnTemplate*  findPawnTemplate(const QString& id) const;
    QList<PawnTemplate*> allPawnTemplates() const;

    // Convenience: build the id->ptr lookup map needed by EncounterInstance::fromTemplate
    QMap<QString, PawnTemplate*> pawnTemplateLookup() const;

    // ── Editor layer: Encounter templates ────────────────────────────────────
    // Full CRUD for EncounterTemplates. Keyed by EncounterTemplate::id.

    void               addEncounterTemplate(std::unique_ptr<EncounterTemplate> tmpl);
    void               removeEncounterTemplate(const QString& id);
    EncounterTemplate* findEncounterTemplate(const QString& id) const;
    QList<EncounterTemplate*> allEncounterTemplates() const;

    // ── Runtime layer: active encounter ──────────────────────────────────────

    // Start a new encounter from a template (clears any existing active encounter).
    void startEncounter(const QString& encounterTemplateId);

    // Create a blank encounter not tied to any template (used on first launch).
    void startEmptyEncounter(const QString& name = "Active Encounter");

    // Replace the active encounter directly (e.g. when loading a saved session).
    void setActiveEncounter(std::unique_ptr<EncounterInstance> encounter);

    // Abandon the current encounter without saving its state.
    void clearActiveEncounter();

    // Raw access – may return nullptr if no encounter is running.
    EncounterInstance*       activeEncounter();
    const EncounterInstance* activeEncounter() const;

    // ── App settings ─────────────────────────────────────────────────────────

    QString         stylesheet() const { return m_stylesheet; }
    void            setStylesheet(const QString& qss) { m_stylesheet = qss; save(); }

    const AppTheme& theme() const { return m_theme; }
    void            setTheme(const AppTheme& theme);

    // ── Convenience accessors to singletons ──────────────────────────────────
    // These forward to the relevant singleton so callers need only one include.

    static AttributeRegistry& attributes() { return AttributeRegistry::instance(); }
    static PlayerRegistry&    players()    { return PlayerRegistry::instance();    }

private:
    PathTrackerStruct() = default;

    // App settings
    QString  m_stylesheet;
    AppTheme m_theme;

    // Editor stores – ordered maps so JSON output is deterministic
    // (std::map avoids Qt COW incompatibility with move-only value types)
    std::map<QString, std::unique_ptr<EffectTemplate>>    m_effectTemplates;
    std::map<QString, std::unique_ptr<PawnTemplate>>      m_pawnTemplates;
    std::map<QString, std::unique_ptr<EncounterTemplate>> m_encounterTemplates;

    // Runtime state
    std::unique_ptr<EncounterInstance> m_activeEncounter;

    // ── Internal helpers ─────────────────────────────────────────────────────
    QJsonObject buildSaveDocument() const;
    bool        parseSaveDocument(const QJsonObject& root);


};


