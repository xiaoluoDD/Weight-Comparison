#include "visualizationslotlabel.h"
#include <QFrame>
#include <QPaintEvent>
#include <QStyleOptionFrame>
#include <QStylePainter>

VisualizationSlotLabel::VisualizationSlotLabel(QWidget *parent)
    : QLabel(parent)
{
    setFrameShape(QFrame::Box);
    setLineWidth(1);
}

void VisualizationSlotLabel::setFlashHighlight(bool on)
{
    if (m_flashHighlight == on)
        return;
    m_flashHighlight = on;
    update();
}

void VisualizationSlotLabel::setDeviationState(const QString &state)
{
    if (m_deviationState == state)
        return;
    m_deviationState = state;
    update();
}

QColor VisualizationSlotLabel::textColorForState() const
{
    if (m_deviationState == QStringLiteral("alarm"))
        return Qt::red;
    if (m_deviationState == QStringLiteral("ok"))
        return Qt::darkGreen;
    return palette().color(QPalette::WindowText);
}

static void drawFlashBorder(QStylePainter &sp, const QWidget *widget)
{
    QPen pen(QColor(Qt::darkYellow));
    pen.setWidth(3);
    sp.setPen(pen);
    sp.setBrush(Qt::NoBrush);
    QRect r = widget->rect().adjusted(2, 2, -3, -3);
    sp.drawRect(r);
}

static QStringList wrapTextToWidth(const QString &text, const QFontMetrics &fm, int maxWidth)
{
    QStringList lines;
    if (text.isEmpty() || maxWidth <= 0)
        return lines;

    int i = 0;
    while (i < text.size()) {
        int len = 1;
        while (i + len <= text.size() && fm.horizontalAdvance(text.mid(i, len)) <= maxWidth)
            ++len;
        if (len > 1 && i + len < text.size())
            --len;
        if (len < 1)
            len = 1;
        lines.append(text.mid(i, len));
        i += len;
    }
    return lines;
}

/** 保持 formatSlotText 的三行结构；仅当整行超宽时才按像素折行（条码与「条码：」同一行） */
static QStringList layoutSlotLines(const QString &doc, int contentWidth, const QFont &font)
{
    QStringList out;
    const QFontMetrics fm(font);
    const QStringList rows = doc.split(QLatin1Char('\n'));
    for (const QString &ln : rows) {
        if (ln.isEmpty())
            continue;
        if (fm.horizontalAdvance(ln) <= contentWidth)
            out.append(ln);
        else
            out.append(wrapTextToWidth(ln, fm, contentWidth));
    }
    return out;
}

static qreal totalLineHeight(const QStringList &lines, const QFontMetrics &fm)
{
    return fm.height() * lines.size();
}

static QFont pickFontToFit(const QFont &baseFont, const QString &doc, int contentWidth, int maxHeight,
                           QStringList *linesOut)
{
    const int minPt = 11;
    QFont font = baseFont;
    for (int pt = baseFont.pointSize(); pt >= minPt; --pt) {
        font.setPointSize(pt);
        const QFontMetrics fm(font);
        const QStringList lines = layoutSlotLines(doc, contentWidth, font);
        if (totalLineHeight(lines, fm) <= maxHeight) {
            *linesOut = lines;
            return font;
        }
    }
    font.setPointSize(minPt);
    *linesOut = layoutSlotLines(doc, contentWidth, font);
    return font;
}

void VisualizationSlotLabel::paintEvent(QPaintEvent *event)
{
    const QString doc = QLabel::text();
    if (doc.isEmpty() && !m_flashHighlight) {
        QLabel::paintEvent(event);
        return;
    }

    QStylePainter sp(this);
    sp.setClipRegion(event->region());
    sp.setRenderHint(QPainter::TextAntialiasing);

    QStyleOptionFrame panelOpt;
    panelOpt.initFrom(this);
    panelOpt.rect = rect();
    panelOpt.frameShape = frameShape();
    panelOpt.lineWidth = lineWidth();
    sp.drawControl(QStyle::CE_ShapedFrame, panelOpt);

    if (!doc.isEmpty()) {
        sp.setPen(textColorForState());

        const QRect r = contentsRect().adjusted(4, 4, -4, -4);
        QStringList lines;
        const QFont drawFont = pickFontToFit(font(), doc, qMax(1, r.width()), qMax(1, r.height()), &lines);
        sp.setFont(drawFont);
        const QFontMetrics fm(drawFont);

        qreal y = r.y();
        const qreal lineH = fm.height();
        for (const QString &ln : lines) {
            sp.drawText(QRectF(r.x(), y, r.width(), lineH),
                        Qt::AlignLeft | Qt::AlignVCenter, ln);
            y += lineH;
        }
    }

    if (m_flashHighlight)
        drawFlashBorder(sp, this);
}
