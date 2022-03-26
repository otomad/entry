#ifndef SOCKETLISTMANAGER_H
#define SOCKETLISTMANAGER_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include "common.h"

class SocketListManager : public QDialog {
		Q_OBJECT

	public:
		explicit SocketListManager(QWidget *parent = nullptr);
		void setTitle(int num = 0);
		inline void setMyIpList(const QStringList list) { setIpList(list, false); }
		inline void setClientIpList(const QStringList list) { setIpList(list, true); }
		void clearAll();

	private:
		QListWidget *myIpList = new QListWidget(this);
		QListWidget *clientIpList = new QListWidget(this);
		QPushButton *removeBtn = new QPushButton(this);

		void setIpList(const QStringList list, bool isClientIp = true);
};

#endif // SOCKETLISTMANAGER_H
