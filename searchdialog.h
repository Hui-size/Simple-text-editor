#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H
#include <QDialog>
class QLineEdit; class QCheckBox; class CodeEditor;
class SearchDialog : public QDialog {
    Q_OBJECT
public:
    SearchDialog(CodeEditor *editor, bool replaceMode, QWidget *parent = nullptr);
private:
    bool findNext(bool notify = true);
    void replaceOne();
    void replaceAll();
    CodeEditor *editor; QLineEdit *findEdit; QLineEdit *replaceEdit; QCheckBox *caseBox; QCheckBox *wholeBox;
};
#endif
