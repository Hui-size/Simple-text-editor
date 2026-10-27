#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "searchdialog.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QSaveFile>
#include <QTextDocument>
#include <QTextStream>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), statusLabel(new QLabel(this)) {
    ui->setupUi(this); statusBar()->addPermanentWidget(statusLabel); setCurrentFile(QString());
    auto e = ui->editor;
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, [this] { save(); });
    connect(ui->actionSaveAs, &QAction::triggered, this, [this] { saveAs(); });
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionUndo, &QAction::triggered, e, &QPlainTextEdit::undo); connect(ui->actionRedo, &QAction::triggered, e, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, e, &QPlainTextEdit::cut); connect(ui->actionCopy, &QAction::triggered, e, &QPlainTextEdit::copy); connect(ui->actionPaste, &QAction::triggered, e, &QPlainTextEdit::paste); connect(ui->actionSelectAll, &QAction::triggered, e, &QPlainTextEdit::selectAll);
    connect(ui->actionFind, &QAction::triggered, this, [this,e] { (new SearchDialog(e, false, this))->show(); });
    connect(ui->actionReplace, &QAction::triggered, this, [this,e] { (new SearchDialog(e, true, this))->show(); });
    connect(ui->actionFont, &QAction::triggered, this, [this,e] { bool ok = false; QFont f = QFontDialog::getFont(&ok, e->font(), this); if (ok) e->setFont(f); });
    connect(ui->actionTextColor, &QAction::triggered, this, [this,e] { QColor c = QColorDialog::getColor(e->palette().color(QPalette::Text), this, tr("文字颜色")); if (c.isValid()) { QPalette p = e->palette(); p.setColor(QPalette::Text, c); e->setPalette(p); } });
    connect(ui->actionBackground, &QAction::triggered, this, [this,e] { QColor c = QColorDialog::getColor(e->palette().color(QPalette::Base), this, tr("背景颜色")); if (c.isValid()) { QPalette p = e->palette(); p.setColor(QPalette::Base, c); e->setPalette(p); } });
    connect(ui->actionWrap, &QAction::toggled, this, [e](bool on) { e->setLineWrapMode(on ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap); });
    connect(ui->actionLineNumbers, &QAction::toggled, e, &CodeEditor::setLineNumbersVisible);
    connect(ui->actionToolbar, &QAction::toggled, ui->toolBar, &QWidget::setVisible); connect(ui->toolBar, &QToolBar::visibilityChanged, ui->actionToolbar, &QAction::setChecked);
    connect(ui->actionStatusbar, &QAction::toggled, statusBar(), &QWidget::setVisible);
    connect(ui->actionAbout, &QAction::triggered, this, [this] { QMessageBox::about(this, tr("关于"), tr("简单文本编辑器\n基于 Qt Widgets 开发。")); });
    connect(e->document(), &QTextDocument::modificationChanged, this, [this] { updateTitle(); updateActions(); });
    connect(e, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatus); connect(e, &QPlainTextEdit::textChanged, this, [this] { updateStatus(); updateActions(); });
    connect(e, &QPlainTextEdit::copyAvailable, this, [this] { updateActions(); }); connect(e, &QPlainTextEdit::undoAvailable, this, [this] { updateActions(); }); connect(e, &QPlainTextEdit::redoAvailable, this, [this] { updateActions(); });
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateActions); updateStatus(); updateActions();
}
MainWindow::~MainWindow() { delete ui; }
void MainWindow::setCurrentFile(const QString &path) { currentFile = path; ui->editor->document()->setModified(false); updateTitle(); }
void MainWindow::updateTitle() { QString name = currentFile.isEmpty() ? tr("新建文本文件") : QFileInfo(currentFile).fileName(); setWindowTitle((ui->editor->document()->isModified() ? QStringLiteral("* ") : QString()) + name + tr(" - 编辑器")); }
void MainWindow::updateStatus() { auto c = ui->editor->textCursor(); statusLabel->setText(tr("长度: %1    行数: %2    行: %3    列: %4").arg(ui->editor->toPlainText().size()).arg(ui->editor->blockCount()).arg(c.blockNumber()+1).arg(c.positionInBlock()+1)); }
void MainWindow::updateActions() { auto e = ui->editor; ui->actionCut->setEnabled(e->textCursor().hasSelection()); ui->actionCopy->setEnabled(e->textCursor().hasSelection()); ui->actionPaste->setEnabled(QApplication::clipboard()->mimeData()->hasText()); ui->actionUndo->setEnabled(e->document()->isUndoAvailable()); ui->actionRedo->setEnabled(e->document()->isRedoAvailable()); }
bool MainWindow::maybeSave() { if (!ui->editor->document()->isModified()) return true; auto result = QMessageBox::warning(this, tr("未保存的修改"), tr("文档已修改，是否保存？"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save); if (result == QMessageBox::Save) return save(); return result == QMessageBox::Discard; }
void MainWindow::newFile() { if (!maybeSave()) return; ui->editor->clear(); setCurrentFile(QString()); }
void MainWindow::openFile() { if (!maybeSave()) return; QString path = QFileDialog::getOpenFileName(this, tr("打开文件"), QString(), tr("文本文件 (*.txt);;所有文件 (*)")); if (path.isEmpty()) return; QFile file(path); if (!file.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, tr("打开失败"), tr("无法打开文件：%1\n%2").arg(path, file.errorString())); return; } QByteArray bytes = file.readAll(); if (file.error() != QFileDevice::NoError) { QMessageBox::warning(this, tr("读取失败"), file.errorString()); return; } QString text = QString::fromUtf8(bytes); ui->editor->setPlainText(text); setCurrentFile(path); }
bool MainWindow::save() { return currentFile.isEmpty() ? saveAs() : saveFile(currentFile); }
bool MainWindow::saveAs() { QString path = QFileDialog::getSaveFileName(this, tr("另存为"), currentFile, tr("文本文件 (*.txt);;所有文件 (*)")); return !path.isEmpty() && saveFile(path); }
bool MainWindow::saveFile(const QString &path) { QSaveFile file(path); if (!file.open(QIODevice::WriteOnly)) { QMessageBox::warning(this, tr("保存失败"), file.errorString()); return false; } QByteArray data = ui->editor->toPlainText().toUtf8(); if (file.write(data) != data.size() || !file.commit()) { QMessageBox::warning(this, tr("保存失败"), file.errorString()); return false; } setCurrentFile(path); statusBar()->showMessage(tr("文件已保存"), 2000); return true; }
void MainWindow::closeEvent(QCloseEvent *event) { if (maybeSave()) event->accept(); else event->ignore(); }
