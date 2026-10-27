#include "searchdialog.h"
#include "ui_searchdialog.h"
#include "codeeditor.h"

#include <QTextCursor>

SearchDialog::SearchDialog(CodeEditor *editor, bool replaceMode, QWidget *parent)
    : QDialog(parent), ui(new Ui::SearchDialog), editor(editor)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(replaceMode ? tr("替换") : tr("查找"));
    setWindowIcon(QIcon(replaceMode ? QStringLiteral(":/icons/replace.svg")
                                   : QStringLiteral(":/icons/find.svg")));
    ui->replaceLabel->setVisible(replaceMode);
    ui->replaceEdit->setVisible(replaceMode);
    ui->replaceButton->setVisible(replaceMode);
    ui->replaceAllButton->setVisible(replaceMode);

    connect(ui->findButton, &QPushButton::clicked, this, &SearchDialog::findNext);
    connect(ui->replaceButton, &QPushButton::clicked, this, &SearchDialog::replaceOne);
    connect(ui->replaceAllButton, &QPushButton::clicked, this, &SearchDialog::replaceAll);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->findEdit, &QLineEdit::textChanged, this, &SearchDialog::updateButtons);
    // 默认按钮处理 Enter，避免与 returnPressed 重复触发两次查找。
    ui->findButton->setDefault(true);
    ui->findEdit->setText(editor->textCursor().selectedText());
    updateButtons();
    focusSearchText();
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::focusSearchText()
{
    ui->findEdit->setFocus();
    ui->findEdit->selectAll();
}

void SearchDialog::updateButtons()
{
    const bool available = !ui->findEdit->text().isEmpty();
    ui->findButton->setEnabled(available);
    ui->replaceButton->setEnabled(available);
    ui->replaceAllButton->setEnabled(available);
    ui->messageLabel->clear();
}

QTextDocument::FindFlags SearchDialog::findFlags(bool includeDirection) const
{
    QTextDocument::FindFlags flags;
    if (ui->caseBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (ui->wholeBox->isChecked())
        flags |= QTextDocument::FindWholeWords;
    if (includeDirection && ui->upButton->isChecked())
        flags |= QTextDocument::FindBackward;
    return flags;
}

bool SearchDialog::findNext()
{
    const QString term = ui->findEdit->text();
    if (term.isEmpty()) {
        ui->messageLabel->setText(tr("请输入查找内容。"));
        return false;
    }
    const auto flags = findFlags();
    const QTextCursor saved = editor->textCursor();
    if (editor->find(term, flags)) {
        ui->messageLabel->clear();
        return true;
    }

    // 越过文档边界后循环查找；完全没有匹配时恢复原光标和选区。
    QTextCursor boundary(editor->document());
    boundary.movePosition(flags.testFlag(QTextDocument::FindBackward)
                          ? QTextCursor::End : QTextCursor::Start);
    editor->setTextCursor(boundary);
    if (editor->find(term, flags)) {
        ui->messageLabel->setText(tr("已到文档边界，继续循环查找。"));
        return true;
    }
    editor->setTextCursor(saved);
    ui->messageLabel->setText(tr("未找到“%1”。").arg(term));
    return false;
}

void SearchDialog::replaceOne()
{
    const QString term = ui->findEdit->text();
    if (term.isEmpty()) {
        findNext();
        return;
    }
    QTextCursor cursor = editor->textCursor();
    // 用同一组查找规则验证选区，防止“全字匹配”替换了单词的一部分。
    QTextCursor from(editor->document());
    from.setPosition(cursor.selectionStart());
    const QTextCursor match = editor->document()->find(term, from, findFlags(false));
    const bool selectedMatch = cursor.hasSelection() && !match.isNull()
            && match.selectionStart() == cursor.selectionStart()
            && match.selectionEnd() == cursor.selectionEnd();
    if (!selectedMatch) {
        if (!findNext())
            return;
        cursor = editor->textCursor();
    }
    cursor.insertText(ui->replaceEdit->text());
    editor->setTextCursor(cursor);
    if (findNext())
        ui->messageLabel->setText(tr("已替换 1 处，已选中下一个匹配项。"));
    else
        ui->messageLabel->setText(tr("已替换 1 处，没有更多匹配项。"));
}

void SearchDialog::replaceAll()
{
    const QString term = ui->findEdit->text();
    if (term.isEmpty()) {
        findNext();
        return;
    }
    const auto flags = findFlags(false);
    QTextCursor editBlock(editor->document());
    QTextCursor cursor(editor->document());
    int count = 0;
    editBlock.beginEditBlock();
    while (true) {
        cursor = editor->document()->find(term, cursor, flags);
        if (cursor.isNull())
            break;
        // 从替换后的末尾继续，替换词包含查找词也不会反复处理刚插入的内容。
        cursor.insertText(ui->replaceEdit->text());
        ++count;
    }
    editBlock.endEditBlock();
    ui->messageLabel->setText(count ? tr("已替换 %1 处。").arg(count) : tr("未找到“%1”，替换 0 处。").arg(term));
}
