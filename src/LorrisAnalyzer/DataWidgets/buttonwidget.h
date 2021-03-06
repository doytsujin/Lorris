/**********************************************
**    This file is part of Lorris
**    http://tasssadar.github.com/Lorris/
**
**    See README and COPYING
***********************************************/

#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H

#include "datawidget.h"

class ButtonWidget : public DataWidget
{
    Q_OBJECT

Q_SIGNALS:
    void clicked();
    void pressed();
    void released();

public:
    ButtonWidget(QWidget *parent);
    void setUp(Storage *storage);
    void saveWidgetInfo(DataFileParser *file);
    void loadWidgetInfo(DataFileParser *file);

    QStringList getScriptEvents();

public slots:
    void setButtonName(const QString& name);
    void setButtonName();
    void setShortcut();
    void setShortcut(const QString &shortcut);
    void setColor(const QString& color);
    void setTextColor(const QString& color);
    QPushButton *getWidget() const;

private slots:
    void buttonClicked();
    void buttonPressed();
    void buttonReleased();
    void setColors();

private:
    QPushButton *m_button;
    QList<QColor> m_colors;
};

#endif // TERMINALWIDGET_H
