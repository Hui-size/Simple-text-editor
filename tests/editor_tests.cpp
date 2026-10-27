#include <QtTest>
#include <QtWidgets>
#include "mainwindow.h"
#include "codeeditor.h"
#include "searchdialog.h"
#include "aboutdialog.h"
#include "ui_mainwindow.h"
#include "ui_searchdialog.h"

class EditorTests : public QObject
{
    Q_OBJECT
private:
    QString tempRoot;
    QString imageRoot;
    QMap<QString, QByteArray> clipboardBackup;

    static void answerMessage(QMessageBox::StandardButton answer)
    {
        QTimer::singleShot(0, [answer] {
            auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (message && message->button(answer))
                message->button(answer)->click();
        });
    }

    static void cancelFileDialog()
    {
        QTimer::singleShot(0, [] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget()))
                dialog->reject();
        });
    }

    static void chooseSavePath(const QString &path)
    {
        QTimer::singleShot(0, [path] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                dialog->selectFile(path);
                QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
            }
        });
    }

    static bool writeBytes(const QString &path, const QByteArray &bytes)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }

private slots:
    void initTestCase()
    {
        QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        QApplication::setQuitOnLastWindowClosed(false);
        tempRoot = qEnvironmentVariable("EDITOR_TEST_TEMP");
        QVERIFY2(!tempRoot.isEmpty(), "Run scripts/build.ps1 -Test to keep test data on F:.");
        QVERIFY(QDir().mkpath(tempRoot));
        imageRoot = QDir(tempRoot).filePath("screenshots");
        QVERIFY(QDir().mkpath(imageRoot));
        const QMimeData *mime = QApplication::clipboard()->mimeData();
        if (mime) {
            for (const QString &format : mime->formats())
                clipboardBackup.insert(format, mime->data(format));
        }
    }

    void initialStateAndResources()
    {
        MainWindow window;
        QVERIFY(!window.isWindowModified());
        QVERIFY(!window.ui->actionUndo->isEnabled());
        QVERIFY(!window.ui->actionRedo->isEnabled());
        QVERIFY(!window.ui->actionCut->isEnabled());
        QVERIFY(!window.ui->actionCopy->isEnabled());
        QVERIFY(!window.ui->actionSelectAll->isEnabled());
        QCOMPARE(window.ui->editor->lineWrapMode(), QPlainTextEdit::WidgetWidth);
        QVERIFY(window.ui->actionLineNumbers->isChecked());
        const auto actions = window.ui->toolBar->actions();
        for (QAction *action : actions) {
            if (!action->isSeparator()) {
                QVERIFY2(!action->icon().isNull(), qPrintable(action->objectName()));
                QVERIFY2(!action->icon().pixmap(24).isNull(), qPrintable(action->objectName()));
            }
        }
        QCOMPARE(window.ui->actionRedo->shortcut(), QKeySequence("Ctrl+Shift+Z"));
    }

    void editActionStatesAndUndoRedo()
    {
        MainWindow window;
        auto *editor = window.ui->editor;
        editor->insertPlainText(QStringLiteral("hello world"));
        QVERIFY(window.isWindowModified());
        QVERIFY(window.ui->actionUndo->isEnabled());
        editor->selectAll();
        QVERIFY(window.ui->actionCopy->isEnabled());
        QVERIFY(window.ui->actionCut->isEnabled());
        window.ui->actionCut->trigger();
        QCOMPARE(editor->toPlainText(), QString());
        QVERIFY(!window.ui->actionCopy->isEnabled());
        QVERIFY(window.ui->actionPaste->isEnabled());
        window.ui->actionUndo->trigger();
        QCOMPARE(editor->toPlainText(), QStringLiteral("hello world"));
        QVERIFY(window.ui->actionRedo->isEnabled());
        window.ui->actionRedo->trigger();
        QCOMPARE(editor->toPlainText(), QString());
        window.ui->actionPaste->trigger();
        QCOMPARE(editor->toPlainText(), QStringLiteral("hello world"));
    }

    void pasteTracksClipboard()
    {
        MainWindow window;
        QApplication::clipboard()->clear();
        QCoreApplication::processEvents();
        QVERIFY(!window.ui->actionPaste->isEnabled());
        QApplication::clipboard()->setText(QStringLiteral("剪贴板"));
        QCoreApplication::processEvents();
        QVERIFY(window.ui->actionPaste->isEnabled());
        // Windows 剪贴板可能被其他程序短暂占用，重试写入再验证 Action 状态。
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
            auto *mime = new QMimeData;
            mime->setData("application/octet-stream", QByteArray("binary"));
            QApplication::clipboard()->setMimeData(mime);
            QCoreApplication::processEvents();
            const auto *current = QApplication::clipboard()->mimeData();
            return current && current->hasFormat("application/octet-stream") && !current->hasText();
        })(), 2000);
        QVERIFY(!window.ui->actionPaste->isEnabled());
    }

    void utf8SaveOpenAndSaveAgain()
    {
        QTemporaryDir dir(tempRoot + "/save-XXXXXX");
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("中文文件名.txt"));
        const QString text = QString::fromUtf8("龙智森 😀\n第二行\n");
        MainWindow window;
        window.ui->editor->insertPlainText(text);
        QVERIFY(window.saveFile(path));
        QVERIFY(!window.isWindowModified());
        QCOMPARE(window.currentFile, path);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), text.toUtf8());
        file.close();
        window.ui->editor->insertPlainText(QStringLiteral("新增"));
        QVERIFY(window.save());
        MainWindow reopened;
        QVERIFY(reopened.loadFile(path));
        QCOMPARE(reopened.ui->editor->toPlainText(), text + QStringLiteral("新增"));
        QVERIFY(!reopened.isWindowModified());
        QVERIFY(!reopened.ui->actionUndo->isEnabled());
    }

    void emptyAndBomFiles()
    {
        QTemporaryDir dir(tempRoot + "/empty-XXXXXX");
        QVERIFY(dir.isValid());
        MainWindow window;
        const QString emptyPath = dir.filePath("empty.txt");
        QVERIFY(window.saveFile(emptyPath));
        QCOMPARE(QFileInfo(emptyPath).size(), 0);
        QVERIFY(window.loadFile(emptyPath));
        QVERIFY(window.ui->editor->toPlainText().isEmpty());
        const QString bomPath = dir.filePath("bom.txt");
        QVERIFY(writeBytes(bomPath, QByteArray::fromHex("efbbbf") + QByteArray("hello\r\nworld")));
        QVERIFY(window.loadFile(bomPath));
        QCOMPARE(window.ui->editor->toPlainText(), QStringLiteral("hello\nworld"));
    }

    void openFailurePreservesDocument_data()
    {
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<bool>("createFile");
        QTest::newRow("missing") << QByteArray() << false;
        QTest::newRow("invalid-utf8") << QByteArray::fromHex("fffefd") << true;
        QTest::newRow("truncated-utf8") << QByteArray::fromHex("e4b8") << true;
        QTest::newRow("binary") << QByteArray("abc\0def", 7) << true;
    }

    void openFailurePreservesDocument()
    {
        QFETCH(QByteArray, bytes);
        QFETCH(bool, createFile);
        QTemporaryDir dir(tempRoot + "/bad-open-XXXXXX");
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("bad.txt");
        if (createFile)
            QVERIFY(writeBytes(path, bytes));
        MainWindow window;
        window.ui->editor->insertPlainText(QStringLiteral("未保存的原文"));
        answerMessage(QMessageBox::Ok);
        QVERIFY(!window.loadFile(path));
        QCOMPARE(window.ui->editor->toPlainText(), QStringLiteral("未保存的原文"));
        QVERIFY(window.isWindowModified());
        QVERIFY(window.currentFile.isEmpty());
    }

    void saveFailureKeepsNameAndModifiedState()
    {
        QTemporaryDir dir(tempRoot + "/bad-save-XXXXXX");
        QVERIFY(dir.isValid());
        MainWindow window;
        window.ui->editor->insertPlainText(QStringLiteral("原文"));
        const QString original = dir.filePath("original.txt");
        QVERIFY(window.saveFile(original));
        window.ui->editor->insertPlainText(QStringLiteral("修改"));
        answerMessage(QMessageBox::Ok);
        QVERIFY(!window.saveFile(dir.filePath("missing/subdir.txt")));
        QCOMPARE(window.currentFile, original);
        QVERIFY(window.isWindowModified());
        QFile file(original);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QStringLiteral("原文").toUtf8());
    }

    void saveAsCancelAndDefaultExtension()
    {
        QTemporaryDir dir(tempRoot + "/save-as-XXXXXX");
        QVERIFY(dir.isValid());
        MainWindow window;
        window.ui->editor->insertPlainText(QStringLiteral("另存为"));
        cancelFileDialog();
        QVERIFY(!window.saveAs());
        QVERIFY(window.currentFile.isEmpty());
        QVERIFY(window.isWindowModified());
        chooseSavePath(dir.filePath("new-file"));
        QVERIFY(window.saveAs());
        QCOMPARE(window.currentFile, dir.filePath("new-file.txt"));
        QVERIFY(QFileInfo::exists(window.currentFile));
        const QString original = window.currentFile;
        window.ui->editor->insertPlainText(QStringLiteral("2"));
        chooseSavePath(dir.filePath("second.txt"));
        QVERIFY(window.saveAs());
        QCOMPARE(window.currentFile, dir.filePath("second.txt"));
        QVERIFY(QFileInfo::exists(original));
        QVERIFY(!window.isWindowModified());
    }

    void closeCancelDiscardAndSave()
    {
        QTemporaryDir dir(tempRoot + "/close-XXXXXX");
        QVERIFY(dir.isValid());
        MainWindow window;
        window.ui->editor->insertPlainText(QStringLiteral("保留内容"));
        QCloseEvent cancel;
        answerMessage(QMessageBox::Cancel);
        window.closeEvent(&cancel);
        QVERIFY(!cancel.isAccepted());
        QCOMPARE(window.ui->editor->toPlainText(), QStringLiteral("保留内容"));
        QCloseEvent discard;
        answerMessage(QMessageBox::Discard);
        window.closeEvent(&discard);
        QVERIFY(discard.isAccepted());

        const QString path = dir.filePath("saved-on-close.txt");
        QVERIFY(window.saveFile(path));
        window.ui->editor->insertPlainText(QStringLiteral("修改"));
        QCloseEvent save;
        answerMessage(QMessageBox::Save);
        window.closeEvent(&save);
        QVERIFY(save.isAccepted());
        QVERIFY(!window.isWindowModified());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QStringLiteral("保留内容修改").toUtf8());
    }

    void closeSaveAsCancelDoesNotClose()
    {
        MainWindow window;
        window.ui->editor->insertPlainText(QStringLiteral("新文档"));
        // 第一个定时器选择保存，第二个定时器在随后出现的另存为窗口取消。
        QTimer::singleShot(0, [] {
            auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (message) {
                cancelFileDialog();
                message->button(QMessageBox::Save)->click();
            }
        });
        QCloseEvent event;
        window.closeEvent(&event);
        QVERIFY(!event.isAccepted());
        QVERIFY(window.isWindowModified());
        QCOMPARE(window.ui->editor->toPlainText(), QStringLiteral("新文档"));
    }

    void newCancelAndDiscard()
    {
        MainWindow window;
        window.ui->editor->insertPlainText(QStringLiteral("不要丢失"));
        answerMessage(QMessageBox::Cancel);
        window.newFile();
        QCOMPARE(window.ui->editor->toPlainText(), QStringLiteral("不要丢失"));
        answerMessage(QMessageBox::Discard);
        window.newFile();
        QVERIFY(window.ui->editor->toPlainText().isEmpty());
        QVERIFY(window.currentFile.isEmpty());
        QVERIFY(!window.isWindowModified());
    }

    void findWrapDirectionCaseAndMissing()
    {
        CodeEditor editor;
        editor.setPlainText(QStringLiteral("cat Cat cat category"));
        SearchDialog dialog(&editor, true);
        dialog.ui->findEdit->setText(QStringLiteral("cat"));
        QVERIFY(dialog.findNext());
        QCOMPARE(editor.textCursor().selectionStart(), 0);
        QVERIFY(dialog.findNext());
        QCOMPARE(editor.textCursor().selectionStart(), 4);
        dialog.ui->caseBox->setChecked(true);
        QVERIFY(dialog.findNext());
        QCOMPARE(editor.textCursor().selectionStart(), 8);
        dialog.ui->wholeBox->setChecked(true);
        QVERIFY(dialog.findNext());
        QCOMPARE(editor.textCursor().selectionStart(), 0);
        dialog.ui->upButton->setChecked(true);
        QVERIFY(dialog.findNext());
        QCOMPARE(editor.textCursor().selectionStart(), 8);
        QVERIFY(dialog.findNext());
        QCOMPARE(editor.textCursor().selectionStart(), 0);
        const QTextCursor previous = editor.textCursor();
        dialog.ui->findEdit->setText(QStringLiteral("missing"));
        QVERIFY(!dialog.findNext());
        QCOMPARE(editor.textCursor().selectionStart(), previous.selectionStart());
        QCOMPARE(editor.textCursor().selectionEnd(), previous.selectionEnd());
        QVERIFY(dialog.ui->messageLabel->text().contains(QStringLiteral("未找到")));
        dialog.ui->findEdit->clear();
        QVERIFY(!dialog.ui->findButton->isEnabled());
        QVERIFY(!dialog.ui->replaceAllButton->isEnabled());
    }

    void enterFindsExactlyOneMatch()
    {
        CodeEditor editor;
        editor.setPlainText(QStringLiteral("cat cat cat"));
        SearchDialog dialog(&editor, false);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        dialog.ui->findEdit->setText(QStringLiteral("cat"));
        dialog.ui->findEdit->setFocus();
        QTest::keyClick(dialog.ui->findEdit, Qt::Key_Return);
        QCOMPARE(editor.textCursor().selectionStart(), 0);
        QTest::keyClick(dialog.ui->findEdit, Qt::Key_Return);
        QCOMPARE(editor.textCursor().selectionStart(), 4);
        QVERIFY(dialog.ui->replaceEdit->isHidden());
    }

    void replaceOneHonorsWholeWordSelection()
    {
        CodeEditor editor;
        editor.setPlainText(QStringLiteral("category cat"));
        QTextCursor cursor = editor.textCursor();
        cursor.setPosition(0);
        cursor.setPosition(3, QTextCursor::KeepAnchor);
        editor.setTextCursor(cursor);
        SearchDialog dialog(&editor, true);
        dialog.ui->findEdit->setText(QStringLiteral("cat"));
        dialog.ui->replaceEdit->setText(QStringLiteral("dog"));
        dialog.ui->wholeBox->setChecked(true);
        dialog.replaceOne();
        QCOMPARE(editor.toPlainText(), QStringLiteral("category dog"));
        editor.undo();
        QCOMPARE(editor.toPlainText(), QStringLiteral("category cat"));
    }

    void replaceAllSelfContainingEmptyAndUndo()
    {
        CodeEditor editor;
        editor.setPlainText(QStringLiteral("cat dog cat category"));
        SearchDialog dialog(&editor, true);
        dialog.ui->findEdit->setText(QStringLiteral("cat"));
        dialog.ui->replaceEdit->setText(QStringLiteral("catcat"));
        dialog.ui->wholeBox->setChecked(true);
        dialog.replaceAll();
        QCOMPARE(editor.toPlainText(), QStringLiteral("catcat dog catcat category"));
        QVERIFY(dialog.ui->messageLabel->text().contains(QStringLiteral("2")));
        editor.undo();
        QCOMPARE(editor.toPlainText(), QStringLiteral("cat dog cat category"));
        editor.redo();
        QCOMPARE(editor.toPlainText(), QStringLiteral("catcat dog catcat category"));
        editor.undo();
        dialog.ui->replaceEdit->clear();
        dialog.replaceAll();
        QCOMPARE(editor.toPlainText(), QStringLiteral(" dog  category"));
        editor.undo();
        QCOMPARE(editor.toPlainText(), QStringLiteral("cat dog cat category"));
        dialog.ui->findEdit->setText(QStringLiteral("missing"));
        dialog.replaceAll();
        QCOMPARE(editor.toPlainText(), QStringLiteral("cat dog cat category"));
        QVERIFY(dialog.ui->messageLabel->text().contains(QStringLiteral("0")));
    }

    void lineNumbersGeometryFontAndWrap()
    {
        MainWindow window;
        window.resize(700, 500);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *editor = window.ui->editor;
        auto *area = editor->findChild<QWidget *>("lineNumberArea");
        QVERIFY(area);
        editor->setPlainText(QString("line\n").repeated(120));
        QCoreApplication::processEvents();
        QCOMPARE(area->width(), editor->lineNumberAreaWidth());
        const int previous = area->width();
        QFont font = editor->font();
        font.setPointSize(24);
        editor->setFont(font);
        QCoreApplication::processEvents();
        QVERIFY(area->width() > previous);
        QCOMPARE(area->width(), editor->lineNumberAreaWidth());
        window.ui->actionLineNumbers->setChecked(false);
        QCOMPARE(editor->lineNumberAreaWidth(), 0);
        QVERIFY(!area->isVisible());
        window.ui->actionLineNumbers->setChecked(true);
        QVERIFY(area->isVisible());
        QCOMPARE(area->width(), editor->lineNumberAreaWidth());
        window.ui->actionWrap->setChecked(false);
        QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::NoWrap);
        window.ui->actionWrap->setChecked(true);
        QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::WidgetWidth);
        window.ui->actionToolbar->setChecked(false);
        QVERIFY(!window.ui->toolBar->isVisible());
        window.ui->actionToolbar->setChecked(true);
        QVERIFY(window.ui->toolBar->isVisible());
        window.ui->actionStatusbar->setChecked(false);
        QVERIFY(!window.statusBar()->isVisible());
        window.ui->actionStatusbar->setChecked(true);
        QVERIFY(window.statusBar()->isVisible());
    }

    void singletonDialogsAndDeveloperDetails()
    {
        MainWindow window;
        window.showSearchDialog(false);
        auto *first = window.findDialog.data();
        QVERIFY(first);
        window.showSearchDialog(false);
        QCOMPARE(window.findDialog.data(), first);
        window.showSearchDialog(true);
        QVERIFY(window.replaceDialog);
        QVERIFY(window.replaceDialog != window.findDialog);
        first->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(window.findDialog.isNull());
        AboutDialog about;
        const auto *developer = about.findChild<QLabel *>("developerLabel");
        const auto *student = about.findChild<QLabel *>("studentLabel");
        QVERIFY(developer && student);
        QVERIFY(developer->text().contains(QStringLiteral("龙智森")));
        QVERIFY(student->text().contains(QStringLiteral("2024414290224")));
    }

    void fontAndColorsThroughActions()
    {
        MainWindow window;
        QFont selected = window.ui->editor->font();
        selected.setPointSize(20);
        selected.setBold(true);
        QTimer::singleShot(0, [selected] {
            if (auto *dialog = qobject_cast<QFontDialog *>(QApplication::activeModalWidget())) {
                dialog->setCurrentFont(selected);
                dialog->accept();
            }
        });
        window.ui->actionFont->trigger();
        QCOMPARE(window.ui->editor->font().pointSize(), 20);
        QVERIFY(window.ui->editor->font().bold());
        QTimer::singleShot(0, [] {
            if (auto *dialog = qobject_cast<QColorDialog *>(QApplication::activeModalWidget())) {
                dialog->setCurrentColor(QColor(30, 60, 150));
                dialog->accept();
            }
        });
        window.ui->actionTextColor->trigger();
        QCOMPARE(window.ui->editor->palette().color(QPalette::Text), QColor(30, 60, 150));
        QTimer::singleShot(0, [] {
            if (auto *dialog = qobject_cast<QColorDialog *>(QApplication::activeModalWidget())) {
                dialog->setCurrentColor(QColor(220, 230, 240));
                dialog->accept();
            }
        });
        window.ui->actionBackground->trigger();
        QCOMPARE(window.ui->editor->palette().color(QPalette::Base), QColor(220, 230, 240));
        QVERIFY(!window.isWindowModified());
    }

    void nativeWidgetScreenshots()
    {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.ui->editor->insertPlainText(QStringLiteral(
            "简单文本编辑器\n\n开发者：龙智森\n学号：2024414290224\n\n"
            "支持新建、打开、保存和另存为。\n"
            "支持撤销/重做、剪切/复制/粘贴、查找与替换。\n"
            "支持字体和颜色、自动换行、行号、工具栏和状态栏。\n"));
        QTest::qWait(100);
        QVERIFY(window.grab().save(imageRoot + "/main-window.png"));
        SearchDialog search(window.ui->editor, true);
        search.ui->findEdit->setText(QStringLiteral("支持"));
        search.ui->replaceEdit->setText(QStringLiteral("包含"));
        search.show();
        QVERIFY(QTest::qWaitForWindowExposed(&search));
        QVERIFY(search.grab().save(imageRoot + "/replace-dialog.png"));
        AboutDialog about;
        about.show();
        QVERIFY(QTest::qWaitForWindowExposed(&about));
        QVERIFY(about.grab().save(imageRoot + "/about-dialog.png"));
    }

    void cleanupTestCase()
    {
        auto *mime = new QMimeData;
        for (auto it = clipboardBackup.cbegin(); it != clipboardBackup.cend(); ++it)
            mime->setData(it.key(), it.value());
        QApplication::clipboard()->setMimeData(mime);
    }
};

QTEST_MAIN(EditorTests)
#include "editor_tests.moc"
