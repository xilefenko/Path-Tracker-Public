#include "editor_player.h"
#include "ui_editor_player.h"
#include "PictureHelper.h"
#include <QMenu>
#include <QFileDialog>
#include <QPixmap>

editor_Player::editor_Player(QWidget *parent)
    : QWidget(parent), ui(new Ui::editor_Player)
{
    ui->setupUi(this);
}

editor_Player::editor_Player(Player* player, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::editor_Player)
    , m_player(player)
{
    ui->setupUi(this);
    ui->PlayerNameLabel->setText(player->playerName);
    PopulateAttrCombo();
    RefreshAttrList();

    if (!m_player->PictureFilePath.isEmpty())
    {
        QPixmap pixmap(ResolvePicturePath(m_player->PictureFilePath));
        if (!pixmap.isNull()) ui->PortraitPreview->setPixmap(pixmap);
    }

    ui->AttrList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AttrList, &QListWidget::customContextMenuRequested,
            this, &editor_Player::ShowAttrContextMenu);
    connect(ui->AttrList, &QListWidget::itemDoubleClicked,
            this, &editor_Player::OnAttrItemDoubleClicked);
}

editor_Player::~editor_Player()
{
    delete ui;
}

void editor_Player::PopulateAttrCombo()
{
    ui->AttrCombo->clear();
    QList<Attribute> attrs = PathTrackerStruct::attributes().allAttributes();
    for (const Attribute& attr : attrs)
        ui->AttrCombo->addItem(attr.displayName, attr.key);
}

void editor_Player::RefreshAttrList()
{
    ui->AttrList->clear();
    if (!m_player) return;
    // Player attributes are live values (PawnInstance subclass, no PawnTemplate)
    for (const AttributeValue& av : m_player->attributes) {
        QString label = av.key;
        if (PathTrackerStruct::attributes().hasAttribute(av.key))
            label = PathTrackerStruct::attributes().getAttribute(av.key).displayName;
        QListWidgetItem* item = new QListWidgetItem(
            QString("%1: %2").arg(label).arg(av.baseValue), ui->AttrList);
        item->setData(Qt::UserRole, av.key);
    }
}

void editor_Player::on_AttrAddButton_clicked()
{
    if (!m_player) return;
    QString key = ui->AttrCombo->currentData().toString();
    if (key.isEmpty()) return;
    int value = ui->AttrSpin->value();
    // Set both baseValue and currentValue so resolveAttributes() preserves the value
    for (AttributeValue& av : m_player->attributes) {
        if (av.key == key) {
            av.baseValue    = value;
            av.currentValue = value;
            RefreshAttrList();
            emit Save();
            return;
        }
    }
    AttributeValue av;
    av.key          = key;
    av.baseValue    = value;
    av.currentValue = value;
    m_player->attributes.append(av);
    RefreshAttrList();
    emit Save();
}

void editor_Player::on_SetPortraitButton_clicked()
{
    if (!m_player) return;
    QString source = QFileDialog::getOpenFileName(
        this, "Select Portrait", {}, "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (source.isEmpty()) return;

    QString relativePath = ImportPicture(source, "portraits");
    if (relativePath.isEmpty()) return;

    m_player->PictureFilePath = relativePath;
    QPixmap pixmap(ResolvePicturePath(relativePath));
    if (!pixmap.isNull()) ui->PortraitPreview->setPixmap(pixmap);
    emit Save();
}

void editor_Player::ShowAttrContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = ui->AttrList->itemAt(pos);
    if (!item) return;
    QString key = item->data(Qt::UserRole).toString();
    QMenu menu(this);
    QAction* removeAction = menu.addAction("Remove Attribute");
    connect(removeAction, &QAction::triggered, [this, key, item]() {
        m_player->attributes.removeIf([&key](const AttributeValue& av) {
            return av.key == key;
        });
        delete item;
        emit Save();
    });
    menu.exec(ui->AttrList->mapToGlobal(pos));
}

void editor_Player::OnAttrItemDoubleClicked(QListWidgetItem* item)
{
    if (!item || !m_player) return;
    QString key = item->data(Qt::UserRole).toString();
    int idx = ui->AttrCombo->findData(key);
    if (idx != -1) ui->AttrCombo->setCurrentIndex(idx);
    for (const AttributeValue& av : m_player->attributes) {
        if (av.key == key) {
            ui->AttrSpin->setValue(av.baseValue);
            break;
        }
    }
}
