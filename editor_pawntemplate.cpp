#include "editor_pawntemplate.h"
#include "ui_editor_pawntemplate.h"
#include "PictureHelper.h"
#include <QMenu>
#include <QFileDialog>
#include <QPixmap>

editor_PawnTemplate::editor_PawnTemplate(QWidget *parent)
    : QWidget(parent), ui(new Ui::editor_PawnTemplate)
{
    ui->setupUi(this);
}

editor_PawnTemplate::editor_PawnTemplate(PathTrackerStruct* data, PawnTemplate* pawn, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_PawnTemplate)
    , m_data(data)
    , m_pawn(pawn)
{
    ui->setupUi(this);
    PopulateAttrCombo();
    PopulateEffectCombo();
    RefreshAttrList();
    RefreshEffectList();

    if (!m_pawn->PictureFilePath.isEmpty())
    {
        QPixmap pixmap(ResolvePicturePath(m_pawn->PictureFilePath));
        if (!pixmap.isNull()) ui->PortraitPreview->setPixmap(pixmap);
    }

    ui->AttrList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AttrList, &QListWidget::customContextMenuRequested,
            this, &editor_PawnTemplate::ShowAttrContextMenu);
    connect(ui->AttrList, &QListWidget::itemDoubleClicked,
            this, &editor_PawnTemplate::OnAttrItemDoubleClicked);

    ui->EffectList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->EffectList, &QListWidget::customContextMenuRequested,
            this, &editor_PawnTemplate::ShowEffectContextMenu);
}

editor_PawnTemplate::~editor_PawnTemplate()
{
    delete ui;
}

void editor_PawnTemplate::PopulateAttrCombo()
{
    ui->AttrCombo->clear();
    QList<Attribute> attrs = PathTrackerStruct::attributes().allAttributes();
    for (const Attribute& attr : attrs)
        ui->AttrCombo->addItem(attr.displayName, attr.key);
}

void editor_PawnTemplate::PopulateEffectCombo()
{
    ui->EffectCombo->clear();
    for (EffectTemplate* tmpl : m_data->allEffectTemplates())
        ui->EffectCombo->addItem(tmpl->name, tmpl->id);
}

void editor_PawnTemplate::RefreshAttrList()
{
    ui->AttrList->clear();
    if (!m_pawn) return;
    for (const AttributeValue& av : m_pawn->baseAttributes) {
        QString label = av.key;
        if (PathTrackerStruct::attributes().hasAttribute(av.key))
            label = PathTrackerStruct::attributes().getAttribute(av.key).displayName;
        QListWidgetItem* item = new QListWidgetItem(
            QString("%1: %2").arg(label).arg(av.baseValue), ui->AttrList);
        item->setData(Qt::UserRole, av.key);
    }
}

void editor_PawnTemplate::RefreshEffectList()
{
    ui->EffectList->clear();
    if (!m_pawn) return;
    for (const ChildEffectRef& ref : m_pawn->defaultEffects) {
        EffectTemplate* tmpl = m_data->findEffectTemplate(ref.effectTemplateId);
        QString label = tmpl ? tmpl->name : ref.effectTemplateId;
        QListWidgetItem* item = new QListWidgetItem(label, ui->EffectList);
        item->setData(Qt::UserRole, ref.effectTemplateId);
    }
}

void editor_PawnTemplate::on_AttrAddButton_clicked()
{
    if (!m_pawn) return;
    QString key = ui->AttrCombo->currentData().toString();
    if (key.isEmpty()) return;
    m_pawn->setBaseAttribute(key, ui->AttrSpin->value());
    RefreshAttrList();
    emit Save();
}

void editor_PawnTemplate::on_EffectAddButton_clicked()
{
    if (!m_pawn) return;
    QString id = ui->EffectCombo->currentData().toString();
    if (id.isEmpty()) return;
    // Prevent duplicate default effects
    for (const ChildEffectRef& ref : m_pawn->defaultEffects)
        if (ref.effectTemplateId == id) return;
    ChildEffectRef ref;
    ref.effectTemplateId = id;
    m_pawn->defaultEffects.append(ref);
    RefreshEffectList();
    emit Save();
}

void editor_PawnTemplate::on_SetPortraitButton_clicked()
{
    if (!m_pawn) return;
    QString source = QFileDialog::getOpenFileName(
        this, "Select Portrait", {}, "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (source.isEmpty()) return;

    QString relativePath = ImportPicture(source, "portraits");
    if (relativePath.isEmpty()) return;

    m_pawn->PictureFilePath = relativePath;
    QPixmap pixmap(ResolvePicturePath(relativePath));
    if (!pixmap.isNull()) ui->PortraitPreview->setPixmap(pixmap);
    emit Save();
}

void editor_PawnTemplate::ShowAttrContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = ui->AttrList->itemAt(pos);
    if (!item) return;
    QString key = item->data(Qt::UserRole).toString();
    QMenu menu(this);
    QAction* removeAction = menu.addAction("Remove Attribute");
    connect(removeAction, &QAction::triggered, [this, key, item]() {
        m_pawn->baseAttributes.removeIf([&key](const AttributeValue& av) {
            return av.key == key;
        });
        delete item;
        emit Save();
    });
    menu.exec(ui->AttrList->mapToGlobal(pos));
}

void editor_PawnTemplate::ShowEffectContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = ui->EffectList->itemAt(pos);
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    QMenu menu(this);
    QAction* removeAction = menu.addAction("Remove Effect");
    connect(removeAction, &QAction::triggered, [this, id, item]() {
        // Remove first occurrence to support future duplicates
        for (int i = 0; i < m_pawn->defaultEffects.size(); ++i) {
            if (m_pawn->defaultEffects[i].effectTemplateId == id) {
                m_pawn->defaultEffects.removeAt(i);
                break;
            }
        }
        delete item;
        emit Save();
    });
    menu.exec(ui->EffectList->mapToGlobal(pos));
}

void editor_PawnTemplate::OnAttrItemDoubleClicked(QListWidgetItem* item)
{
    if (!item || !m_pawn) return;
    QString key = item->data(Qt::UserRole).toString();
    // Select the matching attribute in the combo box
    int idx = ui->AttrCombo->findData(key);
    if (idx != -1) ui->AttrCombo->setCurrentIndex(idx);
    // Pre-fill the spin box with the current base value
    for (const AttributeValue& av : m_pawn->baseAttributes) {
        if (av.key == key) {
            ui->AttrSpin->setValue(av.baseValue);
            break;
        }
    }
}
