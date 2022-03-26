#include "socketlistmanager.h"
#include <QVBoxLayout>
#include <QLabel>
#include "mainwindow.h"

static MainWindow *mainWindow;

SocketListManager::SocketListManager(QWidget *parent) :
	QDialog(parent) {
	mainWindow = (MainWindow *)parentWidget();
	setTitle(0);
	setMinimumSize(280, 560);
	setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint); // 去掉问号按钮
	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout(layout);
	layout->setSpacing(6);
	layout->addWidget(new QLabel("本机 IP", this));
	layout->addWidget(myIpList);
	myIpList->setToolTip("其他客户端可以通过这些 IP 地址连接到本机");
	layout->addWidget(new QLabel("连接中的客户端", this));
	layout->addWidget(clientIpList);
	removeBtn->setText("移除并断开所选连接");
	layout->addWidget(removeBtn);
	int stretch[] = { 0, 3, 0, 7, 0 };
	for (int i = 0; i < (int)(sizeof(stretch) / sizeof(stretch[0])); i++)
		layout->setStretch(i, stretch[i]);
	connect(removeBtn, &QPushButton::clicked, this, [&]() {
		if (clientIpList->count() == 0) return;
		auto selected = clientIpList->selectedItems();
		if (selected.count() == 0) {
			QMessageBox::information(this, "提示", "您还没有选中任何连接！");
			return;
		}
		QString clientName = selected.at(0)->text();
		mainWindow->disconnectedAClient(clientName);
	});
}

void SocketListManager::setTitle(int num) {
	setWindowTitle(QString("%1 台设备连接").arg(num));
}

void SocketListManager::setIpList(const QStringList list, bool isClientIp) {
	QListWidget *lw = isClientIp ? clientIpList : myIpList;
	lw->clear();
	lw->addItems(list);
	removeBtn->setEnabled(isClientIp && list.count());
}

void SocketListManager::clearAll() {
	myIpList->clear();
	clientIpList->clear();
	removeBtn->setEnabled(false);
}
