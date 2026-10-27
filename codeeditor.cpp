#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QResizeEvent>
CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent), lineNumberArea(new LineNumberArea(this)) {
    connect(this, &QPlainTextEdit::blockCountChanged, this, [this] { updateLineNumberAreaWidth(); });
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    updateLineNumberAreaWidth(); highlightCurrentLine();
}
int CodeEditor::lineNumberAreaWidth() const {
    if (!showLineNumbers) return 0;
    int digits = 1; for (int n = qMax(1, blockCount()); n >= 10; n /= 10) ++digits;
    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}
void CodeEditor::setLineNumbersVisible(bool visible) { showLineNumbers = visible; lineNumberArea->setVisible(visible); updateLineNumberAreaWidth(); viewport()->update(); }
void CodeEditor::updateLineNumberAreaWidth() { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); }
void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy) lineNumberArea->scroll(0, dy); else lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth();
}
void CodeEditor::resizeEvent(QResizeEvent *event) { QPlainTextEdit::resizeEvent(event); QRect r = contentsRect(); lineNumberArea->setGeometry(QRect(r.left(), r.top(), lineNumberAreaWidth(), r.height())); }
void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea); painter.fillRect(event->rect(), QColor(235,235,235));
    QTextBlock block = firstVisibleBlock(); int number = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top()); int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) { painter.setPen(Qt::darkGray); painter.drawText(0, top, lineNumberArea->width()-4, fontMetrics().height(), Qt::AlignRight, QString::number(number+1)); }
        block = block.next(); top = bottom; bottom = top + qRound(blockBoundingRect(block).height()); ++number;
    }
}
void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) { QTextEdit::ExtraSelection selection; selection.format.setBackground(QColor(255,255,190)); selection.format.setProperty(QTextFormat::FullWidthSelection, true); selection.cursor = textCursor(); selection.cursor.clearSelection(); selections.append(selection); }
    setExtraSelections(selections);
}
