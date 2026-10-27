#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class QLabel; class QCloseEvent;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    bool maybeSave(); bool save(); bool saveAs(); bool saveFile(const QString &path); void openFile(); void newFile();
    void updateTitle(); void updateStatus(); void updateActions(); void setCurrentFile(const QString &path);
    Ui::MainWindow *ui; QString currentFile; QLabel *statusLabel;
};
#endif
