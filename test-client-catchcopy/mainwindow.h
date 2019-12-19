#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <QStringList>
#include <QList>

#include "catchcopy-api-0002/ClientCatchcopy.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	public:
		explicit MainWindow(QWidget *parent = 0);
		~MainWindow();
	protected:
		void changeEvent(QEvent *e);
	private:
		Ui::MainWindow *ui;
		ClientCatchcopy client;
		/// \brief Structure of plugins
		struct Command
		{
			QTime sendTime;
			quint32 orderId;
			QStringList sendList;
			bool replied;
			QTime replyTime;
			quint32 returnCode;
			QStringList returnList;
		};
		typedef struct Command Command;
		QList<Command> CommandList;
		void addCommand(int orderId);
	private slots:
		void connected();
		void disconnected();
		void tryConnect();
		void tryDisconnect();
		void errorSocket(QLocalSocket::LocalSocketError socketError);
		void on_actionDisconnect_triggered();
		void on_actionConnect_triggered();
		void on_RawSendRemove_clicked();
		void on_RawSendAdd_clicked();
		void on_sendProtocol_clicked();
		void on_sendRawList_clicked();
		void stateChanged(QLocalSocket::LocalSocketState socketState);
		void dataSend(quint32 orderId,QStringList data);
		void on_listCommand_itemSelectionChanged();
		void newReply(quint32 orderId,quint32 returnCode,QStringList returnList);
		void on_pushButtonAskServerName_clicked();
		void on_pushButtonClientName_clicked();
		void on_pushButtonProtocolName_clicked();
		void on_pushButtonProtocolNameVersion_clicked();
		void on_pushButtonCopyAdd_clicked();
		void on_pushButtonCopyRemove_clicked();
		void on_pushButtonCopyWithoutDestination_clicked();
		void on_pushButtonCopyWithDestination_clicked();
		void error(QString error);
		void on_listCommandListSended_itemSelectionChanged();
};

#endif // MAINWINDOW_H
