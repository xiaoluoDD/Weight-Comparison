#ifndef VISUALIZATIONSLOTLABEL_H
#define VISUALIZATIONSLOTLABEL_H

#include <QLabel>
#include <QString>

/** 可视化槽位：工业风面板 + 居中多行文字；偏差色与闪烁边框 */
class VisualizationSlotLabel : public QLabel
{
    Q_OBJECT

public:
    explicit VisualizationSlotLabel(QWidget *parent = nullptr);

    void setFlashHighlight(bool on);
    bool flashHighlight() const { return m_flashHighlight; }

    /** 偏差状态：空/normal、ok、alarm */
    void setDeviationState(const QString &state);
    QString deviationState() const { return m_deviationState; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor textColorForState() const;

    bool m_flashHighlight = false;
    QString m_deviationState;
};

#endif
