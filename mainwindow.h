#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QLabel;
class QCloseEvent;
class SearchDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    friend class EditorTests;
    bool maybeSave();
    bool save();
    bool saveAs();
    bool saveFile(const QString &path);
    bool loadFile(const QString &path);
    void openFile();
    void newFile();
    void showSearchDialog(bool replaceMode);
    void updateTitle();
    void updateStatus();
    void updateActions();
    void setCurrentFile(const QString &path);

    Ui::MainWindow *ui;
    QString currentFile;
    QLabel *statusLabel;
    QPointer<SearchDialog> findDialog;
    QPointer<SearchDialog> replaceDialog;
};
#endif
