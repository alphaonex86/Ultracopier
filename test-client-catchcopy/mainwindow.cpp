#include <QStringList>
#include <QListWidgetItem>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	connect(&client,SIGNAL(connected()),this,SLOT(connected()));
	connect(&client,SIGNAL(disconnected()),this,SLOT(disconnected()));
	connect(&client,SIGNAL(errorSocket(QLocalSocket::LocalSocketError)),this,SLOT(errorSocket(QLocalSocket::LocalSocketError)));
	connect(&client,SIGNAL(error(QString)),this,SLOT(error(QString)));
	connect(&client,SIGNAL(dataSend(quint32 ,QStringList)),this,SLOT(dataSend(quint32 ,QStringList)));
	connect(&client,SIGNAL(newReply(quint32 ,quint32,QStringList)),this,SLOT(newReply(quint32 ,quint32,QStringList)));
	ui->groupBoxPredefinedFunction->setEnabled(false);
	ui->groupBoxRaw->setEnabled(false);
	ui->groupBoxRawSend->setEnabled(false);
	ui->groupBoxCopyMove->setEnabled(false);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindow::connected()
{
	ui->groupBoxPredefinedFunction->setEnabled(true);
	ui->groupBoxRaw->setEnabled(true);
	ui->groupBoxRawSend->setEnabled(true);
	ui->groupBoxCopyMove->setEnabled(true);
	ui->actionConnect->setEnabled(false);
	ui->actionDisconnect->setEnabled(true);
	ui->statusBar->showMessage(tr("Connected"),5000);
}

void MainWindow::disconnected()
{
	ui->groupBoxPredefinedFunction->setEnabled(false);
	ui->groupBoxRaw->setEnabled(false);
	ui->groupBoxRawSend->setEnabled(false);
	ui->groupBoxCopyMove->setEnabled(false);
	ui->actionConnect->setEnabled(true);
	ui->actionDisconnect->setEnabled(false);
	ui->listCommand->clear();
	CommandList.clear();
	on_listCommand_itemSelectionChanged();
	ui->statusBar->showMessage(tr("Disconnected"),5000);
}

void MainWindow::tryConnect()
{
	ui->statusBar->showMessage(tr("Connection..."),5000);
	client.connectToServer();
}

void MainWindow::tryDisconnect()
{
	ui->statusBar->showMessage(tr("Disconnection..."),5000);
	client.disconnectFromServer();
}

void MainWindow::errorSocket(QLocalSocket::LocalSocketError socketError)
{
	if(socketError==QLocalSocket::ConnectionRefusedError)
		ui->statusBar->showMessage(tr("Connection refused"),5000);
	else if(socketError==QLocalSocket::PeerClosedError)
		ui->statusBar->showMessage(tr("Peer have close the connexion"),5000);
	else
		ui->statusBar->showMessage(client.errorStringSocket(),5000);
}


void MainWindow::on_actionDisconnect_triggered()
{
	tryDisconnect();
}

void MainWindow::on_actionConnect_triggered()
{
	tryConnect();
}

void MainWindow::on_RawSendRemove_clicked()
{
	int index=0;
	QList<QListWidgetItem *> list=ui->listRawSend->selectedItems();
	while(index<list.size())
	{

		delete list.at(index);
		index++;
	}
}

void MainWindow::on_RawSendAdd_clicked()
{
	ui->listRawSend->addItem(new QListWidgetItem(ui->lineEditRawSendText->text()));
}

void MainWindow::on_sendProtocol_clicked()
{
	client.sendProtocol();
}

void MainWindow::on_sendRawList_clicked()
{
	QStringList list;
	int index=0;
	while(index<ui->listRawSend->count())
	{
		list<<ui->listRawSend->item(index)->text();
		index++;
	}
	client.sendRawOrderList(list);
}

void MainWindow::stateChanged(QLocalSocket::LocalSocketState socketState)
{
	if(socketState==QLocalSocket::UnconnectedState)
		disconnected();
	else if(socketState==QLocalSocket::ConnectedState)
		connected();
	else
	{
		ui->actionConnect->setEnabled(false);
		ui->groupBoxPredefinedFunction->setEnabled(false);
		ui->groupBoxRaw->setEnabled(false);
		ui->groupBoxRawSend->setEnabled(false);
		ui->groupBoxCopyMove->setEnabled(false);
	}
}

void MainWindow::dataSend(quint32 orderId,QStringList data)
{
	Command newCommand;
	newCommand.orderId=orderId;
	newCommand.sendTime=QTime::currentTime();
	newCommand.replied=false;
	newCommand.sendList=data;
	newCommand.returnCode=0;
	CommandList << newCommand;
	ui->listCommand->addItem(new QListWidgetItem(QString(newCommand.sendTime.toString()+" - "+QString::number(orderId))));
	int index=0;
	while(index<CommandList.size())
	{
		if(orderId==CommandList.at(index).orderId)
		{
			if(data.size()!=0)
				ui->listCommand->item(index)->setText(QString(CommandList.at(index).sendTime.toString()+") "+QString::number(orderId)+" - "+data.first()));
			return;
		}
		index++;
	}
}

void MainWindow::on_listCommand_itemSelectionChanged()
{
	ui->listCommandListSended->clear();
	ui->listReplyList->clear();
	ui->lineReplyCode->setText("");
	if(ui->listCommand->currentRow()<0)
	{
		ui->listCommandListSended->setEnabled(false);
		ui->listReplyList->setEnabled(false);
		ui->lineReplyCode->setEnabled(false);
		ui->timeEditReply->setEnabled(false);
	}
	else
	{
		if(ui->listCommand->currentRow()<CommandList.size())
		{
			ui->listCommandListSended->setEnabled(true);
			ui->listCommandListSended->addItems(CommandList.at(ui->listCommand->currentRow()).sendList);
			if(CommandList.at(ui->listCommand->currentRow()).replied)
			{
				ui->listReplyList->setEnabled(true);
				ui->lineReplyCode->setEnabled(true);
				ui->timeEditReply->setEnabled(true);
				ui->listReplyList->addItems(CommandList.at(ui->listCommand->currentRow()).returnList);
				ui->lineReplyCode->setText(QString::number(CommandList.at(ui->listCommand->currentRow()).returnCode));
				ui->timeEditReply->setTime(CommandList.at(ui->listCommand->currentRow()).replyTime);
			}
			else
			{
				ui->timeEditReply->setEnabled(false);
				ui->listReplyList->setEnabled(false);
				ui->lineReplyCode->setEnabled(false);
			}
		}
		else
			qWarning() << "out of bound!";
	}
}

void MainWindow::newReply(quint32 orderId,quint32 returnCode,QStringList returnList)
{
	int index=0;
	while(index<CommandList.size())
	{
		if(orderId==CommandList.at(index).orderId)
		{
			CommandList[index].replied=true;
			CommandList[index].replyTime=QTime::currentTime();
			CommandList[index].returnCode=returnCode;
			CommandList[index].returnList=returnList;
			if(ui->listCommand->currentRow()==index)
				on_listCommand_itemSelectionChanged();
			return;
		}
		index++;
	}
}

void MainWindow::on_pushButtonAskServerName_clicked()
{
	client.askServerName();
}

void MainWindow::on_pushButtonClientName_clicked()
{
	client.setClientName(ui->lineEditClientName->text());
}

void MainWindow::on_pushButtonProtocolName_clicked()
{
	client.checkProtocolExtension(ui->lineEditProtocolName->text());
}

void MainWindow::on_pushButtonProtocolNameVersion_clicked()
{
	client.checkProtocolExtension(ui->lineEditProtocolName->text(),ui->lineEditProtocolVersion->text());
}

void MainWindow::on_pushButtonCopyAdd_clicked()
{
	ui->listWidgetCopy->addItem(new QListWidgetItem(ui->lineEditAddCopy->text()));
}

void MainWindow::on_pushButtonCopyRemove_clicked()
{
	int index=0;
	QList<QListWidgetItem *> list=ui->listWidgetCopy->selectedItems();
	while(index<list.size())
	{

		delete list.at(index);
		index++;
	}
}

void MainWindow::on_pushButtonCopyWithoutDestination_clicked()
{
	QStringList list;
	int index=0;
	while(index<ui->listWidgetCopy->count())
	{
		list<<ui->listWidgetCopy->item(index)->text();
		index++;
	}
	if(ui->comboBoxCopyType->currentIndex()==0)
		client.addCopyWithoutDestination(list);
	else
		client.addMoveWithoutDestination(list);
}

void MainWindow::on_pushButtonCopyWithDestination_clicked()
{
	QStringList list;
	int index=0;
	while(index<ui->listWidgetCopy->count())
	{
		list<<ui->listWidgetCopy->item(index)->text();
		index++;
	}
	if(ui->comboBoxCopyType->currentIndex()==0)
		client.addCopyWithDestination(list,ui->lineEditCopyDestination->text());
	else
		client.addMoveWithDestination(list,ui->lineEditCopyDestination->text());
}

void MainWindow::error(QString error)
{
	ui->listError->addItem(error);
}

void MainWindow::on_listCommandListSended_itemSelectionChanged()
{
	on_listCommand_itemSelectionChanged();
}
