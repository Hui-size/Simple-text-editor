#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QTextDocument>

namespace Ui { class SearchDialog; }
class CodeEditor;

class SearchDialog : public QDialog
{
    Q_OBJECT
public:
    SearchDialog(CodeEditor *editor, bool replaceMode, QWidget *parent = nullptr);
    ~SearchDialog() override;
    void focusSearchText();
private:
    friend class EditorTests;
    QTextDocument::FindFlags findFlags(bool includeDirection = true) const;
    bool findNext();
    void replaceOne();
    void replaceAll();
    void updateButtons();

    Ui::SearchDialog *ui;
    CodeEditor *editor;
};
#endif
