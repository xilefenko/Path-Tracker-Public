/*
 * Pathfinder DM Cheat Sheet – Core Framework
 * ===========================================
 *
 * Include this single header to pull in the entire framework.
 *
 * CLASS RELATIONSHIPS (flat – no deep nesting)
 * ─────────────────────────────────────────────
 *
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │  SINGLETONS / REGISTRIES                                        │
 *  │  AttributeRegistry   – all Attribute definitions (key → value)  │
 *  │  PlayerRegistry      – "Back Bench" of persistent Player objects │
 *  │  DiceHistoryModel    – session-wide roll log                     │
 *  └─────────────────────────────────────────────────────────────────┘
 *
 *  EDITOR (Templates – immutable at runtime)
 *  ─────────────────────────────────────────
 *  Attribute          key + displayName + defaultValue
 *  AttributeModifier  attributeKey + op(+/-/*) + value  [no instance needed]
 *  ChildEffectRef     effectTemplateId + binding(Bound/Unbound)
 *
 *  Stage              modifiers[]          ← flat AttributeModifier list
 *                     childEffects[]       ← flat ChildEffectRef list
 *                     actions[]            ← flat Action* list
 *
 *  EffectTemplate     id + name + flavor + stages[]
 *  PawnTemplate       id + name + baseAttributes[] + defaultEffects[]
 *  EncounterTemplate  id + name + pawnTemplateIds[]
 *
 *  RUNTIME (Instances – own live state)
 *  ────────────────────────────────────
 *  EffectInstance     templateId + level(int) + currentStageIndex(int)
 *                     + currentRound(int) + stages[] (cloned with actions)
 *
 *  PawnInstance       templateId + attributes[] + effects[]
 *    └─ Player        + playerName  [lives in PlayerRegistry]
 *
 *  EncounterInstance  templateId + ownedPawns[] + activePlayers[]
 *                     + initiativeOrder[] + currentRound(int)
 *
 *  ACTIONS (Instances, polymorphic via Action*)
 *  ─────────────────────────────────────────────
 *  Action                   (abstract)
 *  ├─ LevelModifierAction         direction
 *  ├─ StageModifierAction         direction
 *  ├─ LevelStageModifierAction    target + direction + diceFormula
 *  ├─ DamageThrowAction           damageType + diceFormula
 *  └─ MaxDurationAction           maxRounds + roundsRemaining (runtime)
 *
 *  DATA FLOW  (Editor → Runtime)
 *  ──────────────────────────────
 *  EncounterTemplate  ──fromTemplate()──►  EncounterInstance
 *                                              │ creates
 *                                              ▼
 *  PawnTemplate       ──fromTemplate()──►  PawnInstance
 *                                              │ applyEffect()
 *                                              ▼
 *  EffectTemplate     ──fromTemplate()──►  EffectInstance
 *                                              │ clone()
 *                                              ▼
 *                                          Action* (runtime state owned here)
 *
 *  LOOKUP STRATEGY (kept flat / O(1) or O(n) over small lists)
 *  ─────────────────────────────────────────────────────────────
 *  • AttributeRegistry  → QMap<QString, Attribute>    by key
 *  • PlayerRegistry     → linear scan by instanceId   (small N)
 *  • PawnInstance::attributes → linear scan by key    (small N)
 *  • EncounterInstance::allCombatants() → flat QList<PawnInstance*>
 *  • All template stores in the application (EncounterTemplateStore etc.)
 *    should use QMap<QString, T> keyed by UUID string.
 */

#pragma once

#include "Attribute.h"
#include "AttributeRegistry.h"
#include "Action.h"
#include "Effect.h"
#include "Pawn.h"
#include "Encounter.h"
#include "DiceRoller.h"
