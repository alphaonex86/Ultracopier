#include "DebugEngine.h"

#include <QColor>

#ifdef ULTRACOPIER_DEBUG

#define COLUMN_COUNT 5

// Model

DebugModel::DebugModel()
{
    displayed           = false;
    inWaitOfDisplay     = false;
    updateDisplayTimer  = NULL;
}

DebugModel::~DebugModel()
{
    if(updateDisplayTimer!=NULL)
        delete updateDisplayTimer;
}

int DebugModel::columnCount( const QModelIndex& parent ) const
{
    return parent == QModelIndex() ? COLUMN_COUNT : 0;
}

QVariant DebugModel::data( const QModelIndex& index, int role ) const
{
    int row,column;
    row=index.row();
    column=index.column();
    if(index.parent()!=QModelIndex() || row < 0 || row >= list.count() || column < 0 || column >= COLUMN_COUNT)
        return QVariant();

    const DebugItem& item = list.at(row);
    if(role==Qt::UserRole)
        return row;
    else if(role==Qt::DisplayRole)
    {
        switch(column)
        {
            case 0:
                return item.time;
            break;
            case 1:
                return item.file;
            break;
            case 2:
                return item.function;
            break;
            case 3:
                return item.location;
            break;
            case 4:
                return item.text;
            break;
            default:
            return QVariant();
        }
    }
    else if(role==Qt::DecorationRole)
        return QVariant();
    else if(role==Qt::ForegroundRole)
    {
        switch(item.level)
        {
            case DebugLevel_custom_Information:
                return QColor(94,165,255);
            break;
            case DebugLevel_custom_Critical:
                return QColor(255,0,0);
            break;
            case DebugLevel_custom_Warning:
                return QColor(255,178,0);
            break;
            case DebugLevel_custom_Notice:
                return QColor(128,128,128);
            break;
            case DebugLevel_custom_UserNote:
                return QColor(0,0,0);
            break;
        }
        return QVariant();
    }
    return QVariant();
}

int DebugModel::rowCount( const QModelIndex& parent ) const
{
    return parent == QModelIndex() ? list.count() : 0;
}

QVariant DebugModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < COLUMN_COUNT ) {
        switch ( section ) {
            case 0:
            return QStringLiteral("Time");
            case 1:
            return QStringLiteral("File");
            case 2:
            return QStringLiteral("Function");
            case 3:
            return QStringLiteral("Location");
            case 4:
            return QStringLiteral("Text");
        }
    }

    return QAbstractTableModel::headerData( section, orientation, role );
}

bool DebugModel::setData( const QModelIndex&, const QVariant&, int)
{
    return false;
}

void DebugModel::addDebugInformation(const int &time, const DebugLevel_custom &level, const std::string &function, const std::string &text, const std::string &file, const unsigned int& ligne, const std::string &location)
{
    DebugItem item;
    item.time=time;
    item.level=level;
    item.function=function;
    item.text=text;
    item.file=file+":"+std::to_string(ligne);
    item.location=location;
    list << item;
    if(!displayed)
    {
        displayed=true;
        emit layoutChanged();
    }
    else
        inWaitOfDisplay=true;
}

void DebugModel::setupTheTimer()
{
    if(updateDisplayTimer!=NULL)
        return;
    updateDisplayTimer=new QTimer();
    connect(updateDisplayTimer,&QTimer::timeout,this,&DebugModel::updateDisplay);
    updateDisplayTimer->start(100);
}

void DebugModel::updateDisplay()
{
    displayed=false;
    if(!inWaitOfDisplay)
    {
        inWaitOfDisplay=false;
        displayed=true;
        emit layoutChanged();
    }
}
#endif
