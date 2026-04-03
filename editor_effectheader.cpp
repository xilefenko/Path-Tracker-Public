#include "editor_effectheader.h"
#include "ui_editor_effectheader.h"
#include "PictureHelper.h"
#include <QTimer>
#include <QTextBlock>
#include <QFileDialog>
#include <QPixmap>

editor_EffectHeader::editor_EffectHeader(EffectTemplate* effect, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::editor_EffectHeader)
    , m_effect(effect)
{
    ui->setupUi(this);

    connect(ui->DetailsToggle, &QPushButton::toggled, this, [this](bool checked) {
        ui->DetailsContainer->setVisible(checked);
        ui->DetailsToggle->setText(checked ? "▼ Details" : "▶ Details");
    });
    ui->DetailsToggle->setChecked(true);

    ui->DescriptionInput->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->DescriptionInput->setPlainText(m_effect->description);
    ui->OnsetInput->setText(m_effect->onset);

    if (!m_effect->PictureFilePath.isEmpty())
    {
        QPixmap pixmap(ResolvePicturePath(m_effect->PictureFilePath));
        if (!pixmap.isNull()) ui->IconPreview->setPixmap(pixmap);
    }

    connect(ui->DescriptionInput->document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            this, [this](QSizeF) { AdjustDescriptionHeight(); });
    QTimer::singleShot(0, this, &editor_EffectHeader::AdjustDescriptionHeight);
}

editor_EffectHeader::~editor_EffectHeader()
{
    delete ui;
}

void editor_EffectHeader::AdjustDescriptionHeight()
{
    QPlainTextEdit* edit = ui->DescriptionInput;

    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setLineWrapMode(QPlainTextEdit::WidgetWidth);

    auto updateHeight = [edit]()
    {
        // Force layout update before calculating height
        edit->document()->documentLayout()->update();

        int lines = 0;
        for (QTextBlock block = edit->document()->firstBlock();
             block.isValid();
             block = block.next())
        {
            // Ensure block layout is valid
            if (block.layout())
            {
                lines += block.layout()->lineCount();
            }
        }

        const QFontMetrics fm(edit->font());
        const int padding =
            edit->frameWidth() * 2 +
            edit->contentsMargins().top() +
            edit->contentsMargins().bottom();

        edit->setFixedHeight(30 + lines * fm.lineSpacing() + padding);
    };

    QObject::connect(edit->document(), &QTextDocument::contentsChanged,
                     edit, updateHeight);

    // Defer initial update to allow layout to complete
    QTimer::singleShot(10, edit, updateHeight);


}

void editor_EffectHeader::on_DescriptionInput_textChanged()
{
    if (m_effect) m_effect->description = ui->DescriptionInput->toPlainText();
    emit Save();
}

void editor_EffectHeader::on_OnsetInput_textEdited(const QString& text)
{
    if (m_effect) m_effect->onset = text;
    emit Save();
}

void editor_EffectHeader::on_SetIconButton_clicked()
{
    if (!m_effect) return;
    QString source = QFileDialog::getOpenFileName(
        this, "Select Icon", {}, "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (source.isEmpty()) return;

    QString relativePath = ImportPicture(source, "icons");
    if (relativePath.isEmpty()) return;

    m_effect->PictureFilePath = relativePath;
    QPixmap pixmap(ResolvePicturePath(relativePath));
    if (!pixmap.isNull()) ui->IconPreview->setPixmap(pixmap);
    emit Save();
}


