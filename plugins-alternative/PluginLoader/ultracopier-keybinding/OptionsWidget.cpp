#include "OptionsWidget.h"
#include "KeyBind.h"
#include "ui_OptionsWidget.h"

#include <QKeyEvent>

OptionsWidget::OptionsWidget(QWidget *parent) :
        QWidget(parent),
    ui(new Ui::OptionsWidget)
{
    ui->setupUi(this);

    keyBind=new KeyBind(this);
    ui->vboxLayout->addWidget(keyBind);
    connect(keyBind,&KeyBind::newKey,this,&OptionsWidget::newKeyBind);
}

OptionsWidget::~OptionsWidget()
{
    delete ui;
}

void OptionsWidget::retranslate()
{
    ui->retranslateUi(this);
}

void OptionsWidget::setKeyBind(const QKeySequence &keySequence)
{
    keyBind->setText(keySequence.toString());
}

void OptionsWidget::newKey(QKeyEvent * event)
{
    int keyInt = event->key();
    QList<int> modifier = QList<int>() << Qt::Key_Control << Qt::Key_Shift << Qt::Key_Meta << Qt::Key_Alt << Qt::Key_AltGr;
    if(modifier.indexOf(keyInt) != -1)
         keyInt = 0;
    else {
        Qt::KeyboardModifiers modifiers = event->modifiers();
        if(modifiers & Qt::ShiftModifier)
            keyInt += Qt::SHIFT;
        if(modifiers & Qt::ControlModifier)
            keyInt += Qt::CTRL;
        if(modifiers & Qt::AltModifier)
            keyInt += Qt::ALT;
        if(modifiers & Qt::MetaModifier)
            keyInt += Qt::META;

        QKeySequence keySeq = QKeySequence(keyInt);
        keyBind->setText(keySeq.toString());

        sendKeyBind(keySeq);
    }
}

