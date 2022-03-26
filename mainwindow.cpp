#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "common.h"
#include <QScreen>
#include <QHostInfo>
#include <QTextCodec>

QString MainWindow::ToolName::wave = "数控恒压源",
		MainWindow::ToolName::serial = "串口助手",
		MainWindow::ToolName::serial_legacy = "串口助手(旧)",
		MainWindow::ToolName::signal = "信号发生器",
		MainWindow::ToolName::single = "单片机",
		MainWindow::ToolName::distort = "非线性失真",
		MainWindow::ToolName::scope = "示波器",
		MainWindow::ToolName::audio = "音频分析";

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow) {
	ui->setupUi(this);
	asstConf.parent = this;
	setWindowIcon(QIcon(":/icon/dqy.ico"));
	setTitle();

	//plot
	plot_init();

	//widgets
	#define add(widget, name) ui->tabWidget->addTab(widget, ToolName::name); tabNameList.append(ToolName::name)
	ui->tabWidget->setTabText(0, ToolName::wave); tabNameList.append(ToolName::wave);
	add(tabChat, serial);
	add(tabSerial, serial_legacy);
	add(tabSignal, signal);
	add(tabSingle, single);
	add(tabDistort, distort);
	add(tabScope, scope);
	add(tabAudio, audio);
	#undef add
	setting->tabNameList = &tabNameList;

	//文本校验
	QRegExp regExpIP("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[\\.]){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
	QRegExp regExpNetPort("((6553[0-5])|[655[0-2][0-9]|65[0-4][0-9]{2}|6[0-4][0-9]{3}|[1-5][0-9]{4}|[1-9][0-9]{3}|[1-9][0-9]{2}|[1-9][0-9]|[0-9])");
	ui->serverIpBox->setValidator(new QRegExpValidator(regExpIP, this));
	ui->serverPortBox->setValidator(new QRegExpValidator(regExpNetPort, this));

	//槽函数
	tcpSocket = tcpSocket_Client;
	ui->asServerIpInfo->setVisible(false);
	ui->showClientsBtn->setVisible(false);
	connect(serial, &QSerialPort::readyRead, this, [&]() { //连接信号和槽
		QByteArray buffer = serial->readAll();
		readyRead(buffer, ConnectWay::SERIAL);
	});
	connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::QTcpSocket_connected);
	connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::QTcpServer_connected);
	connect(ui->showClientsBtn, &QPushButton::clicked, this, [&]() { toggleDialog(socketListMgr); });
	connect(ui->tabConnect, &QTabWidget::currentChanged, this, [&](int index) {
		const int LAN_TAB = 1;
		ui->showClientsBtn->setVisible(index == LAN_TAB && ui->asServerRadio->isChecked());
	});

	iniRead();
	on_tabWidget_currentChanged(ui->tabWidget->currentIndex());
	setEnabled(false);

	//浮窗
	popWindow->setCentralWidget(popWindowTab);
	popWindowTab->tabBar()->hide();
	popWindow->installEventFilter(this);

	//根据需要隐藏选项卡
	int tabsNum = ui->tabWidget->count();
	if (startMode >= 0 && startMode < tabsNum)
		for (int i = tabsNum - 1; i >= 0; i--)
			if (i != startMode)
				ui->tabWidget->removeTab(i);

	on_checkSerial_clicked(); //打开界面先自动检测一下串口
}

inline void MainWindow::connect_tcpSocket() {
	connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::QTcpSocket_readyRead);
	connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::QTcpSocket_disconnected);
}

void MainWindow::send(const QByteArray data) {
	if (openingWay == ConnectWay::SERIAL) serial->write(data);
	else if (openingWay == ConnectWay::LAN_CLIENT) tcpSocket->write(data);
	else if (openingWay == ConnectWay::LAN_SERVER) {
		foreach (QTcpSocket *socket, socketList) { //发送给所有客户端
			socket->write(data);
			socket->flush(); //清空缓存，马上发送消息（可以不写）
		}
	}
}

void MainWindow::send(const QString text, Encode_t encode) {
	QByteArray gbk, hex;
	if (encode == Encode::GBK) gbk = QTextCodec::codecForName("gbk")->fromUnicode(text);
	if (encode == Encode::HEX) hex = QString2Hex(avoidOdd(text));
	if (startMode == -1) setting->addMsgLog(
		true,
		openingWay,
		"",
		encode == Encode::UTF8 ? text : "",
		encode == Encode::GBK ? text : "",
		encode == Encode::HEX ? hex : ""
	);
	send(
		encode == Encode::HEX ? hex :
		encode == Encode::UTF8 ? text.toUtf8() : gbk
	);
}

void MainWindow::readyRead(QByteArray buffer, ConnectWay_t way, QString clientName) { // 当串口收到数据时，执行这里的语句
	if (way == ConnectWay::CONNECT_OFF) {
//		qDebug() << QString("警报，有黑客正通过未知隧道向您发送了密函，请立即使用杀毒软件全面检查您的电脑！").toStdString().c_str();
		showStatus("警报，有黑客正通过未知隧道向您发送了密函，请立即使用杀毒软件全面检查您的电脑！", 5000);
	}
	if (buffer.length() == 0) return; // 不接受空指令
	Encode_t stringPrefer = Encode::UTF8;
	QTextCodec::ConverterState state;
	QTextCodec::codecForName("UTF-8")->toUnicode(buffer.constData(), buffer.size(), &state);
	if (state.invalidChars > 0) stringPrefer = Encode::GBK;
	QString utf8 = buffer;
	QString gbk = QTextCodec::codecForName("GBK")->toUnicode(buffer);
	//配置选择性发送信息以节约性能
	#define start(mode) startMode == -1 || startMode == tabNameList.indexOf(ToolName::mode)
	if (start(serial_legacy)) tabSerial->readyRead(utf8, gbk, buffer, stringPrefer);
	if (start(signal)) tabSignal->readyRead(buffer);
	if (start(single)) tabSingle->readyRead(buffer);
	if (start(serial)) tabChat->readyRead(utf8, gbk, buffer, stringPrefer);
	if (start(scope)) tabScope->readyRead(utf8);
	if (start(audio)) tabAudio->readyRead(utf8);
	if (startMode == -1) setting->addMsgLog(false, way, clientName, utf8, gbk, buffer);
	#undef start
}

void MainWindow::QTcpSocket_disconnected() {
	if (openingWay == ConnectWay::LAN_CLIENT) {
		QMessageBox::information(this, "连线结束", "对方已关闭连接。");
		on_openSerial_clicked();
	} else if (openingWay == ConnectWay::LAN_SERVER) {
		QTcpSocket *st = qobject_cast <QTcpSocket *> (sender()); //得到当前与服务端交互的客户端的套接字
		showStatus(QString("客户端 %1 已断开连接！").arg(socket2str(st)), 5000);
		socketList.removeAll(st); //将断开连接的套接字移除
		qDebug() << socketList;
		/* foreach (QTcpSocket *socket, socketList) {
			qDebug()<<socket->peerAddress()<<"Port"<<socket->peerPort();
		} */
		updateSocketList();
		setEnabled(true);
		qDebug()<<"Disconnected!";
	}
}

void MainWindow::QTcpSocket_readyRead() {
	if (openingWay == ConnectWay::LAN_CLIENT) {
		QByteArray buffer = tcpSocket->readAll();
		readyRead(buffer, openingWay);
	} else if (openingWay == ConnectWay::LAN_SERVER) {
		QByteArray buffer;
		//读取缓冲数据
		foreach (QTcpSocket *socket, socketList) {//只要客户端的数据发送过来就读取
			buffer = socket->readAll();
			if (!buffer.isEmpty()) {
				QString port = socket2str(socket); //得到端口
				qDebug() << QString("客户端 %1 发来的消息：\n%2").arg(port, QString(buffer)).toStdString().c_str();
				readyRead(buffer, openingWay, port);
			}
		}
	}
}

MainWindow::~MainWindow() {
//	delete serial;
//	delete tcpSocket;
//	delete setting;
//	delete tabNameList;
	delete popWindow;
	delete curPlotItem;
	delete ui;
}

void MainWindow::showStatus(QString text, int ms) {
	ui->statusBar->showMessage(text, ms);
}

void MainWindow::clearStatus() {
	ui->statusBar->clearMessage();
}

void MainWindow::on_actionExit_triggered() {
//	this->close();
	QApplication::exit();
}

void MainWindow::on_checkSerial_clicked() {
	ui->portNameBox->clear();
	foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) { //通过QSerialPortInfo查找可用串口
		ui->portNameBox->addItem(info.portName());
	}
}

ConnectWay_t MainWindow::getConnectWay() {
	way = ConnectWay_t(ui->tabConnect->currentIndex());
	if (way == ConnectWay::LAN_CLIENT && ui->asServerRadio->isChecked()) way = ConnectWay::LAN_SERVER;
	return way;
}

void MainWindow::on_openSerial_clicked() {
	way = getConnectWay();
	#define unconnect openingWay == ConnectWay::CONNECT_OFF
	switch (way) {
		case ConnectWay::SERIAL:
			if (unconnect) {
				serial->setPortName(ui->portNameBox->currentText());//设置串口名
				serial->setBaudRate(ui->baudrateBox->currentText().toInt());//设置波特率
				switch (ui->dataBitsBox->currentText().toInt()) {//设置数据位数
					case 8: serial->setDataBits(QSerialPort::Data8); break;
					case 7: serial->setDataBits(QSerialPort::Data7); break;
					case 6: serial->setDataBits(QSerialPort::Data6); break;
					case 5: serial->setDataBits(QSerialPort::Data5); break;
					default: break;
				}
				switch (ui->ParityBox->currentIndex()) {//设置奇偶校验
					case 0: serial->setParity(QSerialPort::NoParity); break;
					case 1: serial->setParity(QSerialPort::OddParity); break;
					case 2: serial->setParity(QSerialPort::EvenParity); break;
					default: break;
				}
				switch (ui->stopBitsBox->currentIndex()) {//设置停止位
					case 0: serial->setStopBits(QSerialPort::OneStop); break;
					case 1: serial->setStopBits(QSerialPort::OneAndHalfStop); break;
					case 2: serial->setStopBits(QSerialPort::TwoStop); break;
					default: break;
				}
				serial->setFlowControl(QSerialPort::NoFlowControl);//设置流控制
				if (!serial->open(QIODevice::ReadWrite)) {//打开串口
					QMessageBox::critical(this, "警告", "无法打开串口！\n\
可能的原因：\n\
1. 当前串口正在被其它应用程序占用，请关闭其它使用该串口的应用程序。\n\
2. 该串口设备已正确连接至计算机，但相应的驱动程序未正确安装，请联系设备供应商安装驱动程序。");
					QString conf = QString("Cannot open the serial port!\nSpecify Configurations\n\
PortName: %1\nBaudRate: %2\nDataBits: %3\nParity: %4\nStopBits: %5")
							.arg(serial->portName())
							.arg(serial->baudRate())
							.arg(serial->dataBits())
							.arg(serial->parity())
							.arg(serial->stopBits());
					qDebug() << conf.toStdString().c_str();
					return;
				}
				iniSave();
				setEnabled(true);
				connectDo(true);
			} else {
				serial->close();//关闭串口
				setEnabled(false);
				connectDo(false);
			}
			break;
		case ConnectWay::LAN_CLIENT:
			if (unconnect) {
				tcpSocket = tcpSocket_Client;
				connect_tcpSocket();
				QString ip = ui->serverIpBox->text();
				quint16 port = ui->serverPortBox->text().toUShort();
				tcpSocket->connectToHost(ip, port); //QHostAddress
				ui->openSerial->setText("连接中...");
				ui->tabConnect->setEnabled(false);
				setTitle("等待局域网服务端连接");
				openingWay = way;
			} else {
				setEnabled(false); // 提前该命令以免手动关闭连接却提示对方已关闭连接
				connectDo(false);
				tcpSocket->disconnectFromHost();
				tcpSocket->close();
			}
			break;
		case ConnectWay::LAN_SERVER:
			if (unconnect) {
				QString ip_str = ui->serverIpBox->text();
				QHostAddress ip;
				serverAutoIpFlag = ip_str.isEmpty();
				if (serverAutoIpFlag) ip = QHostAddress::Any;
				else if (ip_str.split(".", QString::SkipEmptyParts).count() != 4) {
					QMessageBox::critical(this, "错误", "输入的 IP 地址不合法。\n若要随机分配 IP 地址，请保留该选项为空。\n目前仅支持 IPv4 地址，不支持 IPv6 地址。");
					return;
				}
				ip = QHostAddress(ip_str);
//				QHostAddress ip = QHostAddress::Any;
				quint16 port = ui->serverPortBox->text().toUShort();
				if (!tcpServer->listen(ip, port)) {
					QMessageBox::warning(this, "配置服务端失败", tcpServer->errorString()); //若出错，输出错误信息
					return;
				}
				socketListMgr->clearAll();
				iniSave();
				if (serverAutoIpFlag) { // 获取本机 IP 地址，方便他人连接
					QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());
					QStringList myIps;
					QString myIp = "";
					foreach (QHostAddress address, info.addresses()) {
						myIps.append(address.toString());
						if (myIp.isEmpty() && address.protocol() == QAbstractSocket::IPv4Protocol) // 只需要 IPv4
							myIp = address.toString();
					}
					if (myIp.isEmpty()) qDebug("Warning: Cannot find my IP address");
					else ui->serverIpBox->setText(myIp);
					socketListMgr->setMyIpList(myIps);
				} else socketListMgr->setMyIpList(QStringList({ip_str}));
				ui->openSerial->setText("等待客户端连接…");
				ui->showClientsBtn->setEnabled(true);
				ui->tabConnect->setEnabled(false);
				setTitle_openingServer(0);
				openingWay = way;
			} else {
				if (tcpSocket->state() == QAbstractSocket::ConnectedState) // 如果正在连接(点击侦听后立即取消侦听，若socket没有指定对象会有异常)
					tcpSocket->disconnectFromHost();
				if (socketListMgr->isVisible()) socketListMgr->hide();
				if (serverAutoIpFlag) ui->serverIpBox->setText("");
				tcpSocket->close();
				tcpServer->close();
				iniSave();
				setEnabled(false);
				connectDo(false);
			}
			break;
		default:
			break;
	}
}

void MainWindow::setTitle(QString title) {
	setWindowTitle(QString("破濒科技上位机（当前状态：%1）").arg(title));
}

void MainWindow::setTitle_openingServer(int num) {
	setTitle(QString("已连接到局域网并开启服务端，%1 台设备连接").arg(num));
	socketListMgr->setTitle(num);
}

void MainWindow::QTcpSocket_connected() {
	iniSave();
	setEnabled(true);
	connectDo(true);
}

void MainWindow::QTcpServer_connected() {
	tcpSocket = tcpServer->nextPendingConnection();
	connect_tcpSocket();
	socketList.append(tcpSocket);//将所有的连接过来的socket保存下来
	qDebug() << socketList;
	QString port = socket2str(tcpSocket);
//	qDebug("客户端 IP %s:%d", ip.toStdString().c_str(), port);
	showStatus(QString("客户端 %1 已连接！").arg(port), 5000);
	updateSocketList();
	setEnabled(true);
	connectDo(true);
}

inline QString MainWindow::socket2str(QTcpSocket *socket) {
	return QString("%1:%2").arg(socket->peerAddress().toString(), QString::number(socket->peerPort()));
}

void MainWindow::updateSocketList() {
	if (openingWay != ConnectWay::LAN_SERVER) return;
	QStringList list;
	foreach (QTcpSocket *socket, socketList) {
		list.append(socket2str(socket));
	}
	socketListMgr->setClientIpList(list);
}

void MainWindow::disconnectedAClient(QString clientName) {
	if (openingWay != ConnectWay::LAN_SERVER) return;
	foreach (QTcpSocket *socket, socketList) {
		if (socket2str(socket) == clientName) {
			socket->close();
			qDebug("客户端 %s 已被移除", clientName.toStdString().c_str());
			showStatus(QString("客户端 %1 已被移除！").arg(clientName), 3000);
			updateSocketList();
			return;
		}
	}
	qDebug("No Such Client!");
}

void MainWindow::setEnabled(bool enabled) {
	// 使能操作
//	bool *openingWhich = way == ConnectWay::SERIAL ? &openingSerial : &openingWlan;
//	*openingWhich = enabled;
	openingWay = enabled ? way : ConnectWay::CONNECT_OFF;
	tabSerial->setEnabled(enabled);
	tabSignal->setEnabled(enabled);
	tabSingle->setEnabled(enabled);
	tabChat->setEnabled(enabled);
	// 失能操作
	/*const QList <QComboBox *> comboBoxes({
								ui->portNameBox,
								ui->baudrateBox,
								ui->dataBitsBox,
								ui->ParityBox,
								ui->stopBitsBox,
								ui->serverIpBox,
								ui->serverPortBox
							  });
//	const QList <QComboBox *> comboBoxes = ui->portLayout->findChildren <QComboBox *>();
	foreach (QComboBox *el, comboBoxes)
		el->setEnabled(!enabled);
	ui->checkSerial->setEnabled(!enabled);*/
	ui->tabConnect->setEnabled(!enabled);
	ui->showClientsBtn->setEnabled(enabled);
	ui->openSerial->setText(enabled ? "断开连接" : "开始连接");
	if (!enabled) setTitle();
	else if (way == ConnectWay::LAN_SERVER) setTitle_openingServer(socketList.count());
	else setTitle(
		way == ConnectWay::SERIAL ? "已连接串口通信" :
		way == ConnectWay::LAN_CLIENT ? "已作为客户端连接到局域网" :
		"已连接个 J8"
	);
}

void MainWindow::connectDo(bool open) {
	tabSignal->connectDo(open);
	tabSingle->connectDo(open);
}

#define windowSizeValue QString("%1,%2").arg(geometry().width()).arg(geometry().height())
#define windowSizeList QList<QVariant>({ geometry().width(), geometry().height() })

void MainWindow::iniSave() {
	if (isIniRead) return;

	configIni->beginGroup("Serial");
//	configIni->setValue("PortName", ui->portNameBox->currentText());
	configIni->setValue("BaudRate", ui->baudrateBox->currentText());
	configIni->setValue("DataBits", ui->dataBitsBox->currentText());
	configIni->setValue("Parity", ui->ParityBox->currentIndex());
	configIni->setValue("StopBits", ui->stopBitsBox->currentText());
	configIni->setValue("ServerIP", ui->serverIpBox->text());
	configIni->setValue("ServerPort", ui->serverPortBox->text());
	configIni->endGroup();

	configIni->beginGroup("Personalize");
	configIni->setValue("Widget", ui->tabWidget->tabText(ui->tabWidget->currentIndex()));
	configIni->setValue("ConnectWay", ui->tabConnect->currentIndex());
	configIni->setValue("LanAs", (int)(ui->asServerRadio->isChecked()));
	configIni->setValue("SendEncode", (int)asstConf._sendEncode);
	configIni->setValue("ReceiveEncode", (int)asstConf._recvEncode);
	configIni->setValue("SendNewLine", (int)asstConf._sendNewLine);
	configIni->setValue("ReceiveNewLine", (int)asstConf._recvNewLine);
	configIni->setValue("SendAutoClear", (int)asstConf._sendAutoClr);
	configIni->setValue("WindowSize", windowSizeList);
	configIni->endGroup();
}

void MainWindow::iniRead() {
	isIniRead = true;

	configIni->beginGroup("Serial");
	ui->baudrateBox->setCurrentText(configIni->value("BaudRate", 9600).toString());
	ui->dataBitsBox->setCurrentText(configIni->value("DataBits", 8).toString());
	ui->ParityBox->setCurrentIndex(configIni->value("Parity", 0).toInt());
	ui->stopBitsBox->setCurrentText(configIni->value("StopBits", 1).toString());
	ui->serverIpBox->setText(configIni->value("ServerIP", "").toString());
	ui->serverPortBox->setText(configIni->value("ServerPort", "8888").toString());
	configIni->endGroup();

	configIni->beginGroup("Personalize");
	asstConf.set(
		configIni->value("SendEncode", -1).toInt(),
		configIni->value("ReceiveEncode", -1).toInt(),
		configIni->value("SendNewLine", -1).toInt(),
		configIni->value("ReceiveNewLine", -1).toInt(),
		configIni->value("SendAutoClear", -1).toInt()
	);
	ui->tabWidget->setCurrentIndex(getTabWidgetByText(configIni->value("Widget", ToolName::wave).toString()));
	ui->tabConnect->setCurrentIndex(configIni->value("ConnectWay", 0).toInt());
	if (configIni->value("LanAs", false).toBool()) ui->asServerRadio->setChecked(true);
	else ui->asClientRadio->setChecked(true);
	tabDistort->currentPlot = configIni->value("DistortionCurrentPlot", 1).toBool() ? NonlinearDistortion::CurrentOutPlot : NonlinearDistortion::CurrentInPlot;
	{ // 在复屏模式下会导致窗口位置不居中 // 已解决
		QScreen *screen = QGuiApplication::screens().at(0);
		QRect screenRect = screen->geometry();
		QList <QVariant> windowSize = configIni->value("WindowSize", windowSizeList).toList();
		int width = windowSize.at(0).toInt(), height = windowSize.at(1).toInt();
		int x = (screenRect.width() - width) / 2, y = (screenRect.height() - height) / 2;
//		qDebug() << screenRect.width() << screenRect.height() << width << height;
		setGeometry(x, y, width, height);
	}
	configIni->endGroup();

	configIni->beginGroup("SignalGenerator");
	tabSignal->personalizeCheck(
		configIni->value("FreqStep", 100).toInt(),
		configIni->value("Amp", 4.4).toDouble(),
		configIni->value("Freq", 1000).toInt(),
		configIni->value("Waveform", 0).toInt(),
		configIni->value("LwrFreq", 100).toInt(),
		configIni->value("UprFreq", 100000).toInt(),
		configIni->value("ScanFreqStep", 100).toInt(),
		configIni->value("ScanPeriod", 10).toInt(),
		configIni->value("ScanWay", 0).toBool()
	);
	configIni->endGroup();

	configIni->beginGroup("SingleChip");
	tabSingle->personalizeCheck(
		configIni->value("ScrollSpeed", 500).toInt(),
		configIni->value("MainKey", "C").toString(),
		configIni->value("BPM", 120).toInt()
	);
	configIni->endGroup();

	isIniRead = false;
}

#undef windowSizeValue
#undef windowSizeList

int MainWindow::getTabWidgetByText(QString text) {
	const QTabWidget *tab = ui->tabWidget;
	for (int i = 0; i < tab->count(); i++)
		if (tab->tabText(i) == text)
			return i;
	return -1;
}

QString MainWindow::getCurrentTabText() {
	return ui->tabWidget->tabText(ui->tabWidget->currentIndex());
}

void MainWindow::on_tabWidget_currentChanged(int index) {
	iniSave();
	const QString tabText = ui->tabWidget->tabText(index);
	curPlotItem->currentPlotMenu->clear();
	if (tabText == ToolName::wave
	 || tabText == ToolName::distort
	 || tabText == ToolName::scope) {
		ui->toolBar->show();
		ui->toolBar_normal->show();
		curPlotItem->tb->setEnabled(true);
		if (tabText == ToolName::wave) {
			setCurPlotItemText(CurrentPlotItem::currentCaptionVa);
			curPlotItem->currentPlotMenu->addAction(curPlotItem->currentPlotGroup->addAction(curPlotItem->vaPlot));
			curPlotItem->vaPlot->setChecked(true);
		} else if (tabText == ToolName::distort) {
			setCurPlotItemText(tabDistort->currentPlot == tabDistort->CurrentInPlot ?
				CurrentPlotItem::currentCaptionIn : CurrentPlotItem::currentCaptionOut
			);
			curPlotItem->currentPlotMenu->addAction(curPlotItem->currentPlotGroup->addAction(curPlotItem->inPlot));
			curPlotItem->currentPlotMenu->addAction(curPlotItem->currentPlotGroup->addAction(curPlotItem->outPlot));
			if (tabDistort->currentPlot == NonlinearDistortion::CurrentInPlot) curPlotItem->inPlot->setChecked(true);
			else curPlotItem->outPlot->setChecked(true);
		} else if (tabText == ToolName::scope) {
			setCurPlotItemText(tabScope->getCurrentPlotName());
			curPlotItem->tb->setEnabled(false);
		}
	} else {
		ui->toolBar->hide();
		setCurPlotItemText(CurrentPlotItem::currentCaptionNA);
		curPlotItem->tb->setEnabled(false);
	}
}

void MainWindow::setCurPlotItemText(QString text) {
	curPlotItem->tb->setText(text);
}

int MainWindow::isConnect() {
	return (int)openingWay + 1;
}

char ConvertHexChar(char ch) {
	if ((ch >= '0') && (ch <= '9'))
		return ch-0x30;
	else if ((ch >= 'A') && (ch <= 'F'))
		return ch-'A'+10;
	else if ((ch >= 'a') && (ch <= 'f'))
		return ch-'a'+10;
	else return -1;
}

QByteArray QString2Hex(QString str) {
	QByteArray senddata;
	int hexdata, lowhexdata;
	int hexdatalen = 0;
	int len = str.length();
	senddata.resize(len / 2);
	char lstr, hstr;
	for (int i = 0; i < len; ){
		hstr = str[i].toLatin1();
		if (hstr == ' '){
			i++;
			continue;
		}
		i++;
		if (i >= len) break;
		lstr = str[i].toLatin1();
		hexdata = ConvertHexChar(hstr);
		lowhexdata = ConvertHexChar(lstr);
		if ((hexdata == 16) || (lowhexdata == 16)) break;
		else hexdata = hexdata * 16 + lowhexdata;
		i++;
		senddata[hexdatalen] = (char)hexdata;
		hexdatalen++;
	}
	senddata.resize(hexdatalen);
	return senddata;
}

double QMax(QVector<double> data) {
	return *(std::max_element(std::begin(data), std::end(data)));
}

double QMin(QVector<double> data) {
	return *(std::min_element(std::begin(data), std::end(data)));
}

void MainWindow::on_actionSetting_triggered() {
	setting->givenValue();
	toggleDialog(setting);
}

void MainWindow::toggleDialog(QDialog *dialog) {
	if (!dialog->isVisible()) dialog->show();
	else dialog->hide();
}

void MainWindow::AssistantConfig::set(
		int sendEncode,
		int recvEncode,
		int sendNewLine,
		int recvNewLine,
		int sendAutoClr
	) { // (bool SendHex, bool ReceiveHex) {
	if (isSetting) return;
	isSetting = true;

	if (sendEncode != -1) _sendEncode = Encode_t(sendEncode);
	if (recvEncode != -1) _recvEncode = Encode_t(recvEncode);
	if (sendNewLine != -1) _sendNewLine = sendNewLine;
	if (recvNewLine != -1) _recvNewLine = recvNewLine;
	if (sendAutoClr != -1) _sendAutoClr = sendAutoClr;
	parent->tabSerial->setConf(_sendEncode, _recvEncode, _sendNewLine, _recvNewLine);
	parent->tabChat->setConf(_sendEncode, _recvEncode, _sendNewLine, _sendAutoClr);
	parent->iniSave();

	isSetting = false;
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index) {
	QWidget *tab = ui->tabWidget->widget(index);
	QString tabName = ui->tabWidget->tabText(index);
	popWindow->setWindowTitle(tabName);
	popWindow->setGeometry(this->geometry());
	popWindowTab->addTab(tab, tabName);
	popWindow->show();
	popWindow->setProperty("index", index);
	hide();
}

void MainWindow::backToTabwidget(QObject *obj) {
	/*QObjectList objList = obj->children();
	foreach (QObject* object, objList) {
		QDockWidget* dockWidget = qobject_cast<QDockWidget*>(object);
		if (dockWidget) {
			QString windowTitle = qobject_cast<QWidget*>(obj)->windowTitle();
			int index =
			//重新插入tab，如果想tab顺序不变，可以用insertTab()
			ui->tabWidget->addTab(dockWidget,windowTitle);
		}
	}*/
	Q_UNUSED(obj);
	QString tabName = popWindow->windowTitle();
	int index = popWindow->property("index").toInt();
	QWidget *tab = popWindowTab->widget(0);
	show();
	ui->tabWidget->insertTab(index, tab, tabName);
	ui->tabWidget->setCurrentIndex(index);
}
