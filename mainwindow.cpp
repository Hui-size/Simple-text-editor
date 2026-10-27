#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
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
#include <QSaveFile>
#include <QStringConverter>
#include <QTextDocument>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), statusLabel(new QLabel(this))
{
    ui->setupUi(this);
    statusBar()->addPermanentWidget(statusLabel, 1);
    setCurrentFile(QString());
    auto *editor = ui->editor;

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, [this] { save(); });
    connect(ui->actionSaveAs, &QAction::triggered, this, [this] { saveAs(); });
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionUndo, &QAction::triggered, editor, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, editor, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, editor, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered, editor, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, editor, &QPlainTextEdit::paste);
    connect(ui->actionSelectAll, &QAction::triggered, editor, &QPlainTextEdit::selectAll);
    connect(ui->actionFind, &QAction::triggered, this, [this] { showSearchDialog(false); });
    connect(ui->actionReplace, &QAction::triggered, this, [this] { showSearchDialog(true); });

    connect(ui->actionFont, &QAction::triggered, this, [this, editor] {
        bool accepted = false;
        const QFont font = QFontDialog::getFont(&accepted, editor->font(), this, tr("字体"));
        if (accepted)
            editor->setFont(font);
    });
    connect(ui->actionTextColor, &QAction::triggered, this, [this, editor] {
        const QColor color = QColorDialog::getColor(editor->palette().color(QPalette::Text),
                                                    this, tr("字体颜色"));
        if (color.isValid()) {
            QPalette palette = editor->palette();
            palette.setColor(QPalette::Text, color);
            editor->setPalette(palette);
        }
    });
    connect(ui->actionBackground, &QAction::triggered, this, [this, editor] {
        const QColor color = QColorDialog::getColor(editor->palette().color(QPalette::Base),
                                                    this, tr("编辑器背景色"));
        if (color.isValid()) {
            QPalette palette = editor->palette();
            palette.setColor(QPalette::Base, color);
            editor->setPalette(palette);
        }
    });
    connect(ui->actionWrap, &QAction::toggled, this, [editor](bool enabled) {
        editor->setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    });
    connect(ui->actionLineNumbers, &QAction::toggled, editor, &CodeEditor::setLineNumbersVisible);
    connect(ui->actionToolbar, &QAction::toggled, ui->toolBar, &QWidget::setVisible);
    connect(ui->toolBar, &QToolBar::visibilityChanged, ui->actionToolbar, &QAction::setChecked);
    connect(ui->actionStatusbar, &QAction::toggled, statusBar(), &QWidget::setVisible);
    connect(ui->actionAbout, &QAction::triggered, this, [this] {
        AboutDialog dialog(this);
        dialog.exec();
    });
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this] {
        updateTitle();
        updateActions();
    });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatus);
    connect(editor, &QPlainTextEdit::textChanged, this, [this] {
        updateStatus();
        updateActions();
    });
    connect(editor, &QPlainTextEdit::copyAvailable, this, &MainWindow::updateActions);
    connect(editor, &QPlainTextEdit::undoAvailable, this, &MainWindow::updateActions);
    connect(editor, &QPlainTextEdit::redoAvailable, this, &MainWindow::updateActions);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateActions);

    editor->setLineWrapMode(ui->actionWrap->isChecked()
                           ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    editor->setLineNumbersVisible(ui->actionLineNumbers->isChecked());
    updateStatus();
    updateActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showSearchDialog(bool replaceMode)
{
    QPointer<SearchDialog> &dialog = replaceMode ? replaceDialog : findDialog;
    if (!dialog)
        dialog = new SearchDialog(ui->editor, replaceMode, this);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    dialog->focusSearchText();
}

void MainWindow::setCurrentFile(const QString &path)
{
    currentFile = path;
    ui->editor->document()->setModified(false);
    updateTitle();
}

void MainWindow::updateTitle()
{
    const QString name = currentFile.isEmpty() ? tr("新建文本文件") : QFileInfo(currentFile).fileName();
    setWindowTitle(name + QStringLiteral("[*]") + tr(" - 简单文本编辑器"));
    setWindowModified(ui->editor->document()->isModified());
}

void MainWindow::updateStatus()
{
    const QTextCursor cursor = ui->editor->textCursor();
    statusLabel->setText(tr("长度：%1    行数：%2    行：%3    列：%4    UTF-8    龙智森")
                        .arg(ui->editor->toPlainText().size())
                        .arg(ui->editor->blockCount())
                        .arg(cursor.blockNumber() + 1)
                        .arg(cursor.positionInBlock() + 1));
}

void MainWindow::updateActions()
{
    const auto *editor = ui->editor;
    ui->actionCut->setEnabled(editor->textCursor().hasSelection() && !editor->isReadOnly());
    ui->actionCopy->setEnabled(editor->textCursor().hasSelection());
    ui->actionPaste->setEnabled(editor->canPaste());
    ui->actionUndo->setEnabled(editor->document()->isUndoAvailable() && !editor->isReadOnly());
    ui->actionRedo->setEnabled(editor->document()->isRedoAvailable() && !editor->isReadOnly());
    ui->actionSelectAll->setEnabled(!editor->document()->isEmpty());
}

bool MainWindow::maybeSave()
{
    if (!ui->editor->document()->isModified())
        return true;
    const auto result = QMessageBox::warning(this, tr("未保存的修改"),
                         tr("文档“%1”已修改，是否保存？")
                             .arg(currentFile.isEmpty() ? tr("新建文本文件") : QFileInfo(currentFile).fileName()),
                         QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                         QMessageBox::Save);
    if (result == QMessageBox::Save)
        return save();
    return result == QMessageBox::Discard;
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;
    ui->editor->clear();
    setCurrentFile(QString());
}

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("打开文件"), currentFile,
                                                     tr("文本文件 (*.txt);;所有文件 (*)"));
    if (!path.isEmpty() && maybeSave())
        loadFile(path);
}

bool MainWindow::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("打开失败"), tr("无法打开文件：%1\n%2").arg(path, file.errorString()));
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        QMessageBox::warning(this, tr("读取失败"), file.errorString());
        return false;
    }

    // 严格校验，避免把乱码或二进制内容读入后覆盖原文件。
    QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
    const QString text = decoder(bytes);
    if (decoder.hasError() || text.contains(QChar::Null)) {
        QMessageBox::warning(this, tr("文件格式不支持"),
                             tr("此文件不是有效的 UTF-8 文本，或包含二进制内容。\n"
                                "请先将文件转换为 UTF-8 文本后再打开；当前文档已保留。"));
        return false;
    }

    ui->editor->setPlainText(text);
    setCurrentFile(QFileInfo(path).absoluteFilePath());
    statusBar()->showMessage(tr("文件已打开"), 2000);
    return true;
}

bool MainWindow::save()
{
    return currentFile.isEmpty() ? saveAs() : saveFile(currentFile);
}

bool MainWindow::saveAs()
{
    QFileDialog dialog(this, tr("另存为"), currentFile, tr("文本文件 (*.txt);;所有文件 (*)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral("txt"));
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
        return false;
    return saveFile(dialog.selectedFiles().first());
}

bool MainWindow::saveFile(const QString &path)
{
    // 先写临时文件，提交成功后才替换目标，写入失败不破坏原文件。
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("保存失败"), tr("无法保存文件：%1\n%2").arg(path, file.errorString()));
        return false;
    }
    const QByteArray data = ui->editor->toPlainText().toUtf8();
    if (file.write(data) != data.size() || !file.commit()) {
        QMessageBox::warning(this, tr("保存失败"), file.errorString());
        return false;
    }
    setCurrentFile(QFileInfo(path).absoluteFilePath());
    statusBar()->showMessage(tr("文件已保存"), 2000);
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}
