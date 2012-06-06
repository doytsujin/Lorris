/**********************************************
**    This file is part of Lorris
**    http://tasssadar.github.com/Lorris/
**
**    See README and COPYING
***********************************************/

#include <QDropEvent>
#include <QMouseEvent>
#include <QPainter>

#include "widgetarea.h"
#include "lorrisanalyzer.h"
#include "datafileparser.h"
#include "storage.h"

#include "DataWidgets/numberwidget.h"
#include "DataWidgets/barwidget.h"
#include "DataWidgets/colorwidget.h"
#include "DataWidgets/GraphWidget/graphwidget.h"
#include "DataWidgets/ScriptWidget/scriptwidget.h"
#include "DataWidgets/inputwidget.h"
#include "DataWidgets/terminalwidget.h"
#include "DataWidgets/buttonwidget.h"

WidgetArea::WidgetArea(QWidget *parent) :
    QFrame(parent)
{
    m_widgetIdCounter = 0;
    m_skipNextMove = false;
}

WidgetArea::~WidgetArea()
{
    clear();
}

void WidgetArea::clear()
{
    w_map::iterator itr = m_widgets.begin();
    while(itr != m_widgets.end())
    {
        delete *itr;
        itr = m_widgets.erase(itr);
    }
    m_marks.clear();
}

void WidgetArea::dropEvent(QDropEvent *event)
{
    QString data = event->mimeData()->text().remove(0, 1);
    quint8 type = data.toInt();

    event->acceptProposedAction();

    addWidget(event->pos(), type);
}

DataWidget *WidgetArea::addWidget(QPoint pos, quint8 type, bool show)
{
    DataWidget *w = newWidget(type, this);
    if(!w)
        return NULL;

    quint32 id = getNewId();
    w->setId(id);

    w->setUp(m_storage);

    w->move(pos);
    if(show)
        w->show();

    m_widgets.insert(id, w);

    connect(m_analyzer, SIGNAL(newData(analyzer_data*,quint32)), w, SLOT(newData(analyzer_data*,quint32)));
    connect(w,          SIGNAL(removeWidget(quint32)),              SLOT(removeWidget(quint32)));
    connect(w,          SIGNAL(updateMarker(DataWidget*)),          SLOT(updateMarker(DataWidget*)));
    connect(w,          SIGNAL(updateData()),                       SIGNAL(updateData()));
    connect(w,   SIGNAL(mouseStatus(bool,data_widget_info,qint32)), SIGNAL(mouseStatus(bool,data_widget_info,qint32)));
    connect(w,          SIGNAL(SendData(QByteArray)),   m_analyzer, SIGNAL(SendData(QByteArray)));
    connect(m_analyzer, SIGNAL(setTitleVisibility(bool)),        w, SLOT(setTitleVisibility(bool)));

    //events
    connect(this,       SIGNAL(onWidgetAdd(DataWidget*)),        w, SLOT(onWidgetAdd(DataWidget*)));
    connect(this,       SIGNAL(onWidgetRemove(DataWidget*)),     w, SLOT(onWidgetRemove(DataWidget*)));
    connect(this,       SIGNAL(onScriptEvent(QString)),          w, SLOT(onScriptEvent(QString)));
    connect(w,          SIGNAL(scriptEvent(QString)),         this, SIGNAL(onScriptEvent(QString)));

    emit onWidgetAdd(w);

    w->setTitleVisibility(m_analyzer->showTitleBars());
    return w;
}

void WidgetArea::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->source() && event->mimeData()->text().at(0) == 'w')
        event->acceptProposedAction();
    else
        QFrame::dragEnterEvent(event);
}

DataWidget *WidgetArea::newWidget(quint8 type, QWidget *parent)
{
    switch(type)
    {
        case WIDGET_NUMBERS: return new NumberWidget(parent);
        case WIDGET_BAR:     return new BarWidget(parent);
        case WIDGET_COLOR:   return new ColorWidget(parent);
        case WIDGET_GRAPH:   return new GraphWidget(parent);
        case WIDGET_SCRIPT:  return new ScriptWidget(parent);
        case WIDGET_INPUT:   return new InputWidget(parent);
        case WIDGET_TERMINAL:return new TerminalWidget(parent);
        case WIDGET_BUTTON:  return new ButtonWidget(parent);
    }
    return NULL;
}

void WidgetArea::removeWidget(quint32 id)
{
    w_map::iterator itr = m_widgets.find(id);
    if(itr == m_widgets.end())
        return;

    emit onWidgetRemove(*itr);

    delete *itr;
    m_widgets.erase(itr);

    m_marks.remove(id);
    update();
}

DataWidget *WidgetArea::getWidget(quint32 id)
{
    w_map::iterator itr = m_widgets.find(id);
    if(itr == m_widgets.end())
        return NULL;
    return *itr;
}

void WidgetArea::SaveWidgets(DataFileParser *file)
{
    // We want widgets saved in same order as they were created. It does not have to be super-fast,
    // so I am using std::map to sort them.
    std::map<quint32, DataWidget*> widgets;
    for(w_map::iterator itr = m_widgets.begin(); itr != m_widgets.end(); ++itr)
        if((*itr)->getWidgetControlled() == -1)
            widgets[itr.key()] = *itr;

    // write widget count
    quint32 count = widgets.size();
    file->write((char*)&count, sizeof(quint32));

    for(std::map<quint32, DataWidget*>::iterator itr = widgets.begin(); itr != widgets.end(); ++itr)
    {
        file->writeBlockIdentifier(BLOCK_WIDGET);
        itr->second->saveWidgetInfo(file);
    }
}

void WidgetArea::LoadWidgets(DataFileParser *file, bool skip)
{
    clear();

    quint32 count = 0;
    file->read((char*)&count, sizeof(quint32));

    for(quint32 i = 0; i < count; ++i)
    {
        if(!file->seekToNextBlock(BLOCK_WIDGET, 0))
            break;

        // type
        if(!file->seekToNextBlock("widgetType", BLOCK_WIDGET))
            break;

        quint8 type = 0;
        file->read((char*)&type, sizeof(quint8));

        // pos and size
        if(!file->seekToNextBlock("widgetPosSize", BLOCK_WIDGET))
            break;

        int val[4];
        file->read((char*)&val, sizeof(val));

        DataWidget *w = addWidget(QPoint(val[0], val[1]), type, !skip);
        if(!w)
            continue;

        w->resize(val[2], val[3]);
        w->loadWidgetInfo(file);

        if(skip)
            removeWidget(w->getId());
        else
            updateMarker(w);
    }
    update();
}

void WidgetArea::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_mouse_orig = event->globalPos();
        setCursor(Qt::SizeAllCursor);
    }
}

void WidgetArea::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
        setCursor(Qt::ArrowCursor);
}

void WidgetArea::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    event->accept();

    if(m_marks.empty())
        return;

    QPainter painter(this);

    painter.setPen(Qt::red);
    painter.setBrush(QBrush(Qt::red, Qt::SolidPattern));

    for(mark_map::iterator itr = m_marks.begin(); itr != m_marks.end(); ++itr)
        painter.drawRect(*itr);
}

void WidgetArea::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() != Qt::LeftButton)
        return;

    QPoint n = event->globalPos() - m_mouse_orig;

    moveWidgets(n);

    m_mouse_orig = event->globalPos();
}

void WidgetArea::moveWidgets(QPoint diff)
{
    for(w_map::iterator itr = m_widgets.begin(); itr != m_widgets.end(); ++itr)
    {
        QPoint pos = (*itr)->pos() + diff;
        (*itr)->move(pos);

        updateMarker(*itr);
    }
    update();
}

void WidgetArea::getMarkPos(int &x, int &y, QSize &size)
{
    size = (y < 0 || y > height()) ? QSize(20, 5) : QSize(5, 20);

    if     (x < 0)       x = 0;
    else if(x > width()) x = width() - 5;

    if     (y < 0)        y = 0;
    else if(y > height()) y = height() - 5;
}

void WidgetArea::updateMarker(DataWidget *w)
{
    bool do_update = false;

    if(!rect().intersects(w->geometry()))
    {
        QPoint markPos(w->pos().x(), w->pos().y());
        markPos.rx() += w->width()/2;
        markPos.ry() += w->height()/2;

        QSize size;
        getMarkPos(markPos.rx(), markPos.ry(), size);

        m_marks[w->getId()] = QRect(markPos, size);

        do_update = true;

        w->setUpdating(false);
    }
    else
    {
        do_update = m_marks.remove(w->getId());
        if(do_update)
        {
            w->setUpdating(true);

            quint32 idx = 0;
            analyzer_data *data = m_analyzer->getLastData(idx);
            if(data)
                w->newData(data, idx);
        }
    }

    if(do_update)
        update();
}

void WidgetArea::moveEvent(QMoveEvent *event)
{
    if(m_skipNextMove)
        m_skipNextMove = false;
    else
        moveWidgets(event->oldPos() - pos());
}

void WidgetArea::resizeEvent(QResizeEvent *)
{
    for(w_map::iterator itr = m_widgets.begin(); itr != m_widgets.end(); ++itr)
        updateMarker(*itr);
}