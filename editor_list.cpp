#include "editor_list.h"
#include "ui_editor_list.h"
#include "editor_addeffect.h"
#include "editor_attribute.h"
#include <QMessageBox>
#include <QStack>
#include <QSet>

// ─────────────────────────────────────────────
//  Default Constructor
// ─────────────────────────────────────────────

editor_List::editor_List(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_List)
{
    ui->setupUi(this);
}

// ─────────────────────────────────────────────
//  Constructor for specific stage index
// ─────────────────────────────────────────────

editor_List::editor_List(PathTrackerStruct* data, EffectTemplate* effect, int index, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_List)
    , m_effect(effect)
    , m_data(data)
{
    // Grow stage list if needed
    while (index >= m_effect->stages.size())
        m_effect->addStage();

    m_stage = &m_effect->stages[index];

    ui->setupUi(this);
    InitContextMenus();
    BuildLists();
}

editor_List::~editor_List()
{
    delete ui;
}

// ─────────────────────────────────────────────
//  Context menu setup
// ─────────────────────────────────────────────

void editor_List::InitContextMenus()
{
    ui->ActionList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->ActionList, &QListWidget::customContextMenuRequested,
            this, &editor_List::ShowActionContextMenu);

    ui->AttributeList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AttributeList, &QListWidget::customContextMenuRequested,
            this, &editor_List::ShowAttributeContextMenu);

    ui->AddEffectList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AddEffectList, &QListWidget::customContextMenuRequested,
            this, &editor_List::ShowAddEffectContextMenu);
}

// ─────────────────────────────────────────────
//  Action context menu
// ─────────────────────────────────────────────

void editor_List::ShowActionContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QListWidgetItem *item = ui->ActionList->itemAt(pos);

    // --- REMOVE ACTION ---
    if (item) {
        QAction *removeAction = menu.addAction("Remove Action");
        connect(removeAction, &QAction::triggered, [this, item]() {
            int row = ui->ActionList->row(item);
            if (row >= 0 && row < m_stage->actions.size()) {
                m_stage->actions.removeAt(row);
                delete item;
            }
            emit Save();
        });
    }

    // --- ADD ACTION (one entry per registered type) ---
    for (auto [key, value] : ActionFactory::registeredTypes().asKeyValueRange()) {
        QAction *action = menu.addAction(value);
        connect(action, &QAction::triggered, [this, key]() {
            AddActionToRound(key);
            BuildLists();
        });
    }

    menu.exec(ui->ActionList->mapToGlobal(pos));
}

// ─────────────────────────────────────────────
//  Attribute context menu
// ─────────────────────────────────────────────

void editor_List::ShowAttributeContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QListWidgetItem *item = ui->AttributeList->itemAt(pos);

    // --- REMOVE ATTRIBUTE ---
    if (item) {
        QAction *removeAction = menu.addAction("Remove Attribute");
        connect(removeAction, &QAction::triggered, [this, item]() {
            int row = ui->AttributeList->row(item);
            if (row >= 0 && row < m_stage->modifiers.size()) {
                m_stage->modifiers.removeAt(row);
                delete item;
            }
            emit Save();
        });
    }

    // --- ADD SUBMENU ---
    QMenu *addMenu = menu.addMenu("Add Attribute...");

    QList<Attribute> allAttrs = PathTrackerStruct::attributes().allAttributes();
    for (const Attribute& attr : allAttrs) {

        // Skip attributes already present in this stage
        bool alreadyAdded = false;
        for (const AttributeModifier& modifier : m_stage->modifiers) {
            if (modifier.attributeKey == attr.key) {
                alreadyAdded = true;
                break;
            }
        }
        if (alreadyAdded) continue;

        QAction *act = addMenu->addAction(attr.displayName);
        connect(act, &QAction::triggered, [this, key = attr.key]() {
            AddAttributeToRound(key);
            BuildLists();
        });
    }

    menu.exec(ui->AttributeList->mapToGlobal(pos));
}

// ─────────────────────────────────────────────
//  Add Effect context menu
// ─────────────────────────────────────────────

void editor_List::ShowAddEffectContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QListWidgetItem *item = ui->AddEffectList->itemAt(pos);

    // --- REMOVE EFFECT ---
    if (item) {
        QAction *removeAction = menu.addAction("Remove Effect");
        connect(removeAction, &QAction::triggered, [this, item]() {
            int row = ui->AddEffectList->row(item);
            if (row >= 0 && row < m_stage->childEffects.size()) {
                m_stage->childEffects.removeAt(row);
                delete item;
            }
            emit Save();
        });
    }

    // --- ADD SUBMENU ---
    QMenu *addMenu = menu.addMenu("Add Effect...");

    for (EffectTemplate* tmpl : m_data->allEffectTemplates()) {
        QAction *act = addMenu->addAction(tmpl->name);
        connect(act, &QAction::triggered, [this, id = tmpl->id]() {
            AddEffectToRound(id);
            emit Save();
        });
    }

    menu.exec(ui->AddEffectList->mapToGlobal(pos));
}

// ─────────────────────────────────────────────
//  Data modification helpers
// ─────────────────────────────────────────────

void editor_List::AddActionToRound(QString typeName)
{
    if (!m_stage || !m_data) return;

    auto action = ActionFactory::create(typeName);
    if (action)
        m_stage->actions.append(action);

    emit Save();
}

void editor_List::AddAttributeToRound(QString attributeKey)
{
    if (!m_stage || !m_data) return;

    AttributeModifier modifier;
    modifier.attributeKey = attributeKey;
    modifier.op           = ModifierOp::Plus;
    modifier.value        = 1;
    m_stage->modifiers.append(modifier);

    emit Save();
}

void editor_List::AddEffectToRound(QString id)
{
    if (!m_stage || !m_data) return;

    if (WouldCauseCircularity(id, m_effect->id)) {
        QMessageBox::warning(this, "Cannot Add Effect",
                             "Adding this effect would create a circular dependency.");
        return;
    }

    ChildEffectRef ref;
    ref.effectTemplateId = id;
    ref.binding          = ChildBinding::Bound;
    m_stage->childEffects.append(ref);

    BuildLists();
}

// ─────────────────────────────────────────────
//  List building
// ─────────────────────────────────────────────

void editor_List::ClearLists(QListWidget& list)
{
    list.clear();
}

void editor_List::BuildLists()
{
    Q_ASSERT(m_stage);

    ClearLists(*ui->ActionList);
    ClearLists(*ui->AttributeList);
    ClearLists(*ui->AddEffectList);

    for (auto& actionPtr : m_stage->actions)
        CreateActionWidget(actionPtr.get());

    for (auto& modifier : m_stage->modifiers)
        CreateAttributeWidget(&modifier);

    for (auto& ref : m_stage->childEffects)
        CreateAddEffectWidget(&ref);
}

void editor_List::CreateActionWidget(Action* action)
{
    auto* item   = new QListWidgetItem(ui->ActionList);
    auto* widget = action->createEditorWidget(ui->ActionList, [this](){ emit Save(); });

    item->setSizeHint(widget->sizeHint());
    ui->ActionList->addItem(item);
    ui->ActionList->setItemWidget(item, widget);
}

void editor_List::CreateAttributeWidget(AttributeModifier* modPtr)
{
    QString label = modPtr->attributeKey;
    if (PathTrackerStruct::attributes().hasAttribute(modPtr->attributeKey))
        label = PathTrackerStruct::attributes().getAttribute(modPtr->attributeKey).displayName;

    auto* item   = new QListWidgetItem(ui->AttributeList);
    auto* widget = new editor_attribute(label, modPtr, ui->AttributeList);

    connect(widget, SIGNAL(Save()), this, SIGNAL(Save()));

    item->setData(Qt::UserRole, modPtr->attributeKey);
    item->setSizeHint(widget->sizeHint());
    ui->AttributeList->addItem(item);
    ui->AttributeList->setItemWidget(item, widget);
}

void editor_List::CreateAddEffectWidget(ChildEffectRef* refPtr)
{
    EffectTemplate* tmpl  = m_data->findEffectTemplate(refPtr->effectTemplateId);
    QString         label = tmpl ? tmpl->name
                                 : QString("Effect %1").arg(refPtr->effectTemplateId);

    auto* item   = new QListWidgetItem(ui->AddEffectList);
    auto* widget = new editor_addeffect(label, refPtr, ui->AddEffectList);

    connect(widget, SIGNAL(Save()), this, SIGNAL(Save()));

    item->setData(Qt::UserRole, refPtr->effectTemplateId);
    item->setSizeHint(widget->sizeHint());
    ui->AddEffectList->addItem(item);
    ui->AddEffectList->setItemWidget(item, widget);
}

// ─────────────────────────────────────────────
//  Circularity check
// ─────────────────────────────────────────────

bool editor_List::WouldCauseCircularity(const QString& candidateID, const QString& targetID)
{
    if (candidateID == targetID) return true;

    QSet<QString>   visited;
    QStack<QString> stack;
    stack.push(candidateID);

    while (!stack.isEmpty()) {
        QString cur = stack.pop();

        if (visited.contains(cur)) continue;
        visited.insert(cur);

        EffectTemplate* eff = m_data->findEffectTemplate(cur);
        if (!eff) continue;

        for (const Stage& stage : eff->stages) {
            for (const ChildEffectRef& ref : stage.childEffects) {
                if (ref.effectTemplateId == targetID) return true;
                if (!visited.contains(ref.effectTemplateId))
                    stack.push(ref.effectTemplateId);
            }
        }
    }
    return false;
}
