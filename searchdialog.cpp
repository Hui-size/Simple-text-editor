#include "searchdialog.h"
#include "codeeditor.h"
#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>
SearchDialog::SearchDialog(CodeEditor *e, bool replaceMode, QWidget *parent) : QDialog(parent), editor(e) {
    setWindowTitle(replaceMode ? tr("替换") : tr("查找")); setAttribute(Qt::WA_DeleteOnClose);
    findEdit = new QLineEdit(this); replaceEdit = new QLineEdit(this); caseBox = new QCheckBox(tr("区分大小写"), this); wholeBox = new QCheckBox(tr("全字匹配"), this);
    auto *form = new QFormLayout; form->addRow(tr("查找内容："), findEdit); if (replaceMode) form->addRow(tr("替换为："), replaceEdit);
    auto *options = new QHBoxLayout; options->addWidget(caseBox); options->addWidget(wholeBox); options->addStretch();
    auto *buttons = new QHBoxLayout; auto *findButton = new QPushButton(tr("查找下一个"), this); buttons->addWidget(findButton);
    connect(findButton, &QPushButton::clicked, this, [this] { findNext(); }); connect(findEdit, &QLineEdit::returnPressed, this, [this] { findNext(); });
    if (replaceMode) { auto *one = new QPushButton(tr("替换"), this); auto *all = new QPushButton(tr("全部替换"), this); buttons->addWidget(one); buttons->addWidget(all); connect(one, &QPushButton::clicked, this, &SearchDialog::replaceOne); connect(all, &QPushButton::clicked, this, &SearchDialog::replaceAll); }
    auto *close = new QPushButton(tr("关闭"), this); buttons->addWidget(close); connect(close, &QPushButton::clicked, this, &QDialog::close);
    auto *layout = new QVBoxLayout(this); layout->addLayout(form); layout->addLayout(options); layout->addLayout(buttons);
    findEdit->setText(editor->textCursor().selectedText()); findEdit->selectAll(); findEdit->setFocus();
}
bool SearchDialog::findNext(bool notify) {
    QString term = findEdit->text(); if (term.isEmpty()) { if (notify) QMessageBox::information(this, tr("查找"), tr("请输入查找内容。")); return false; }
    QTextDocument::FindFlags flags; if (caseBox->isChecked()) flags |= QTextDocument::FindCaseSensitively; if (wholeBox->isChecked()) flags |= QTextDocument::FindWholeWords;
    if (editor->find(term, flags)) return true;
    QTextCursor saved = editor->textCursor(), start = saved; start.movePosition(QTextCursor::Start); editor->setTextCursor(start);
    if (editor->find(term, flags)) return true;
    editor->setTextCursor(saved); if (notify) QMessageBox::information(this, tr("查找"), tr("未找到“%1”。").arg(term)); return false;
}
void SearchDialog::replaceOne() {
    QString term = findEdit->text(); if (term.isEmpty()) { findNext(); return; }
    auto cursor = editor->textCursor(); Qt::CaseSensitivity cs = caseBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    bool selectedMatch = cursor.hasSelection() && cursor.selectedText().compare(term, cs) == 0;
    if (selectedMatch && wholeBox->isChecked()) { QTextCursor start(editor->document()); start.setPosition(cursor.selectionStart()); QTextDocument::FindFlags flags = QTextDocument::FindWholeWords; if (caseBox->isChecked()) flags |= QTextDocument::FindCaseSensitively; QTextCursor match = editor->document()->find(term, start, flags); selectedMatch = !match.isNull() && match.selectionStart() == cursor.selectionStart() && match.selectionEnd() == cursor.selectionEnd(); }
    if (!selectedMatch) { if (!findNext()) return; cursor = editor->textCursor(); }
    cursor.insertText(replaceEdit->text()); editor->setTextCursor(cursor); findNext(false);
}
void SearchDialog::replaceAll() {
    QString term = findEdit->text(); if (term.isEmpty()) { findNext(); return; }
    QTextDocument::FindFlags flags; if (caseBox->isChecked()) flags |= QTextDocument::FindCaseSensitively; if (wholeBox->isChecked()) flags |= QTextDocument::FindWholeWords;
    QTextCursor cursor(editor->document()); cursor.beginEditBlock(); int count = 0;
    while (!(cursor = editor->document()->find(term, cursor, flags)).isNull()) { cursor.insertText(replaceEdit->text()); ++count; }
    QTextCursor end(editor->document()); end.endEditBlock(); QMessageBox::information(this, tr("替换"), tr("已替换 %1 处。").arg(count));
}
