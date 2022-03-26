#include "serialportchat.h"
#include "ui_serialportchat.h"
#include "mainwindow.h"

static MainWindow *mainWindow;

SerialPortChat::SerialPortChat(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SerialPortChat) {
	ui->setupUi(this);
	sendTimer->stop();
	mainWindow = (MainWindow *)parentWidget();
	connect(ui->sendNewLine, &QCheckBox::toggled, this, [&](bool checked) { mainWindow->asstConf.setSendNewLine(checked); });
	connect(ui->autoClearBtn, &QCheckBox::toggled, this, [&](bool checked) { if (!ui->intervalSend->isChecked()) mainWindow->asstConf.setSendAutoClr(checked); });
	QList <QRadioButton *> radios ({
		ui->sendUtf8,
		ui->sendGbk,
		ui->sendHex
	});
	foreach (auto *radio, radios) {
		connect(radio, &QRadioButton::toggled, this, [&]() {
			Encode_t
				sendEncode =
					ui->sendHex->isChecked() ? Encode::HEX :
					ui->sendGbk->isChecked() ? Encode::GBK : Encode::UTF8;
			if (sendEncode == Encode::HEX) examineHex(ui->sendBox);
			mainWindow->asstConf.setSendEncode(sendEncode);
		});
	}
	connect(sendTimer, &QTimer::timeout, ui->sendBtn, &QPushButton::click);
}

SerialPortChat::~SerialPortChat() {
	delete ui;
}

void SerialPortChat::dealMessage(QNChatMessage *messageW, QListWidgetItem *item, QString text, QString time, QNChatMessage::User_Type type) {
	messageW->setFixedWidth(this->width());
	QSize size = messageW->fontRect(text);
	if (type == QNChatMessage::User_Time) size = QSize(this->width(), 40);
	item->setSizeHint(size);
	messageW->setText(text, time, size, type);
	ui->chatList->setItemWidget(item, messageW);
}

void SerialPortChat::dealMessageTime(QString curMsgTime) {
	const int distance = 60; // 两个消息相差一分钟
	bool isShowTime = false;
	if (ui->chatList->count() > 0) {
		QListWidgetItem *lastItem = ui->chatList->item(ui->chatList->count() - 1);
		QNChatMessage *messageW = (QNChatMessage *)ui->chatList->itemWidget(lastItem);
		int lastTime = messageW->time().toInt();
		int curTime = curMsgTime.toInt();
		qDebug("curTime lastTime: %d", curTime - lastTime);
		isShowTime = ((curTime - lastTime) > distance);
	} else
		isShowTime = true;
	if (isShowTime) {
		QNChatMessage *messageTime = new QNChatMessage(ui->chatList->parentWidget());
		QListWidgetItem *itemTime = new QListWidgetItem(ui->chatList);

		QSize size = QSize(this->width(), 40);
		messageTime->resize(size);
		itemTime->setSizeHint(size);
		messageTime->setText(curMsgTime, curMsgTime, size, QNChatMessage::User_Time);
		ui->chatList->setItemWidget(itemTime, messageTime);
	}
}

void SerialPortChat::resizeEvent(QResizeEvent *event) {
	/*const int margin = 10;

	ui->sendBox->resize(this->width() - margin * 2, ui->widget->height() - margin * 2);
	ui->sendBox->move(margin, margin);

	int layoutLeft = ui->sendBox->width() + ui->sendBox->x() - ui->sendBtn->width() - margin,
		layoutTop = ui->sendBox->height() + ui->sendBox->y() - ui->sendBtn->height() - margin;

	ui->sendHex->move(margin, layoutTop);
	ui->sendBtn->move(layoutLeft, layoutTop);*/

	for (int i = 0; i < ui->chatList->count(); i++) {
		QNChatMessage *messageW = (QNChatMessage *)ui->chatList->itemWidget(ui->chatList->item(i));
		QListWidgetItem *item = ui->chatList->item(i);
		dealMessage(messageW, item, messageW->text(), messageW->time(), messageW->userType());
	}
	ui->chatList->scrollToItem(ui->chatList->currentItem());

	return QWidget::resizeEvent(event);
}

#define RecvEncode mainWindow->asstConf.recvEncode()
#define SendEncode mainWindow->asstConf.sendEncode()
#define SendNewLine mainWindow->asstConf.sendNewLine()
#define SendAutoClr mainWindow->asstConf.sendAutoClr()

void SerialPortChat::on_sendBtn_clicked() {
	QString msg = sendNewLineCheck(ui->sendBox, SendNewLine);
	if (msg.isEmpty()) return;
	Encode_t encode = SendEncode;
	if (encode == Encode::HEX) msg = avoidOdd(msg);
	if (SendAutoClr && !ui->intervalSend->isChecked()) ui->sendBox->clear();
	QDateTime time = QDateTime::currentDateTime();
//	if (ui->chatList->count() % 2)
//	if (!chats.count() % 2)
	addMessage(
		encode == Encode::UTF8 ? msg : "",
		time,
		QNChatMessage::User_Me,
		encode == Encode::HEX ? beautifyHex(msg) : "",
		encode == Encode::GBK ? msg : "",
		encode
	);
	mainWindow->send(msg, SendEncode);
	/*mainWindow->send(
						sendHex() ?
						QString2Hex(msg) :
						msg.toUtf8()
					);*/
	ui->sendBox->setFocus();
}

void SerialPortChat::addMessage(QString utf8, QDateTime time, bool isSend, QString hex, QString gbk, Encode_t stringPrefer) {
	QNChatMessage::User_Type user = isSend ? QNChatMessage::User_Me : QNChatMessage::User_She;
	addMessage(utf8, time, user, hex, gbk, stringPrefer);
}

void SerialPortChat::addMessage(QString utf8, QDateTime time, QNChatMessage::User_Type user, QString hex, QString gbk, Encode_t stringPrefer) {
	if (utf8 == "" && user != QNChatMessage::User_Me) return;
	QString timeStr = QString::number(time.toSecsSinceEpoch()); //时间戳
	qDebug() << QString("addMessage %1 %2 %3").arg(utf8).arg(timeStr).arg(ui->chatList->count()).toStdString().c_str();
	dealMessageTime(timeStr);
	QNChatMessage *messageW = new QNChatMessage(ui->chatList->parentWidget());
	QListWidgetItem *item = new QListWidgetItem(ui->chatList);
	Encode_t encode = user == QNChatMessage::User_Me ? SendEncode :
					  user == QNChatMessage::User_She ? RecvEncode : Encode::UTF8;
	if (user == QNChatMessage::User_She && encode != Encode::HEX && stringPrefer != Encode::HEX && encode != stringPrefer) encode = stringPrefer;
	QString msgDisplay = encode == Encode::UTF8 ? utf8 : //(isHex && user == QNChatMessage::User_She) ? hex : msg;
						 encode == Encode::GBK ? gbk : hex;
	dealMessage(messageW, item, msgDisplay, timeStr, user);
	messageW->setTextSuccess();
	ui->chatList->setCurrentRow(ui->chatList->count() - 1);
	chats.append({time, utf8, gbk, hex, user, encode, messageW, item, stringPrefer});
}

void SerialPortChat::setConf(Encode_t sendEncode, Encode_t recvEncode, bool sendNewLine, bool sendAutoClr) {
	ui->sendNewLine->setChecked(sendNewLine);
	ui->autoClearBtn->setChecked(sendAutoClr);
	auto checkRadio = [&](Encode_t value, QRadioButton *radio, Encode_t condition) { if (value == condition) radio->setChecked(true); };
	checkRadio(sendEncode, ui->sendUtf8, Encode::UTF8);
	checkRadio(sendEncode, ui->sendGbk, Encode::GBK);
	checkRadio(sendEncode, ui->sendHex, Encode::HEX);
	Q_UNUSED(recvEncode);
}

void SerialPortChat::on_sendBox_textChanged() {
	if (SendEncode == Encode::HEX && !isChanging) {
		isChanging = true;
		examineHex(ui->sendBox);
		isChanging = false;
	}
}

void SerialPortChat::clearList() {
	QMessageBox::StandardButton ensure = QMessageBox::information(this, "清空", "确定清空所有记录？", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (ensure != QMessageBox::Yes) return;
	/*foreach (ChatItem item, chats) {
//		delete item.msgw;
		delete item.item;
	}*/
	chats.clear();
	ui->chatList->clear();
}

void SerialPortChat::on_chatList_customContextMenuRequested(const QPoint &pos) {
	QListWidgetItem *curItem = ui->chatList->itemAt(pos);
	#define noChat {\
		qDebug("You may choose no chat item!");\
		return;\
	}
	if (curItem == nullptr) noChat
	ui->chatList->setCurrentItem(curItem);
	ChatItem chat;
	bool legal = getChatItem(curItem, &chat);
	if (!legal) noChat

	QMenu *popMenu = new QMenu(this);
	QString timeStr = chat.time.toString("yyyy/MM/dd HH:mm:ss.zzz");
	QActionGroup *showAsEncodeGroup = new QActionGroup(this);
	QAction *exactTime = new QAction(timeStr, this);
	QAction *showAsUtf8 = new QAction("显示为 &UTF-8 编码", this);
	QAction *showAsGbk = new QAction("显示为 &GBK 编码", this);
	QAction *showAsHex = new QAction("显示为 16 进制(&H)", this);
	QAction *copySeed = new QAction("复制(&C)", this);
	QAction *deleteSeed = new QAction("删除", this);
	QAction *clearSeeds = new QAction("清空", this);
	exactTime->setEnabled(false);
	showAsHex->setIconVisibleInMenu(false);
	showAsUtf8->setCheckable(true);
	showAsGbk->setCheckable(true);
	showAsHex->setCheckable(true);
	if (chat.encode == Encode_t::HEX) showAsHex->setChecked(true);
	else if (chat.encode == Encode_t::GBK) showAsGbk->setChecked(true);
	else showAsUtf8->setChecked(true);
	if (chat.who == QNChatMessage::User_Me) {
		showAsUtf8->setEnabled(false);
		showAsGbk->setEnabled(false);
		showAsHex->setEnabled(false);
	}
	popMenu->addAction(exactTime);
	popMenu->addSeparator();
	popMenu->addAction(showAsEncodeGroup->addAction(showAsUtf8));
	popMenu->addAction(showAsEncodeGroup->addAction(showAsGbk));
	popMenu->addAction(showAsEncodeGroup->addAction(showAsHex));
	popMenu->addSeparator();
	popMenu->addAction(copySeed);
	popMenu->addAction(deleteSeed);
	popMenu->addSeparator();
	popMenu->addAction(clearSeeds);
	auto changeEncode = [&](Encode_t encode) {
		QString text = encode == Encode::HEX ? chat.hex :
					   encode == Encode::GBK ? chat.gbk : chat.utf8;
		dealMessage(chat.msgw, chat.item, text, chat.msgw->time(), chat.msgw->userType());
		mainWindow->asstConf.setRecvEncode(encode);
		setChatItemEncode(chat.msgw, encode);
	};
	connect(showAsUtf8, &QAction::triggered, this, [&]() { changeEncode(Encode::UTF8); });
	connect(showAsGbk, &QAction::triggered, this, [&]() { changeEncode(Encode::GBK); });
	connect(showAsHex, &QAction::triggered, this, [&]() { changeEncode(Encode::HEX); });
	connect(copySeed, &QAction::triggered, this, [&]() {
		QClipboard *clip = QApplication::clipboard();
		clip->setText(chat.msgw->text());
	});
	connect(deleteSeed, &QAction::triggered, this, [&]() { deleteSeedSlot(chat); });
	connect(clearSeeds, &QAction::triggered, this, &SerialPortChat::clearList);
	popMenu->exec(QCursor::pos());
	delete popMenu;
	delete exactTime;
	delete showAsHex;
	delete copySeed;
	delete deleteSeed;
	delete clearSeeds;
}

void SerialPortChat::deleteSeedSlot(ChatItem chat) {
	// ui->chatList->removeItemWidget(curItem);
	int curIndex = ui->chatList->row(chat.item);
	ui->chatList->takeItem(curIndex);
	removeChatItem(chat);
	if (curIndex == 0) return;
	QListWidgetItem *previousItem = ui->chatList->item(curIndex - 1);
	QNChatMessage *previousMsgw = (QNChatMessage *)ui->chatList->itemWidget(previousItem);
	if (previousMsgw->userType() == QNChatMessage::User_Time) {
		bool flag = false;
		QListWidgetItem *nextItem = ui->chatList->item(curIndex);
		if (nextItem == nullptr) flag = true;
		else {
			QNChatMessage *nextMsgw = (QNChatMessage *)ui->chatList->itemWidget(nextItem);
			if (nextMsgw->userType() == QNChatMessage::User_Time) flag = true;
		}
		if (flag) {
			QListWidgetItem *uselessTime = ui->chatList->takeItem(curIndex - 1);
			delete uselessTime;
		}
	}
}

bool SerialPortChat::getChatItem(const QListWidgetItem *item, ChatItem *Chat) {
	foreach (ChatItem chat, chats) // foreach 是会创建副本太垃圾了
		if (chat.item == item) {
			*Chat = chat;
			return true;
		}
	return false;
}

void SerialPortChat::removeChatItem(ChatItem Chat) {
	for (int i = 0; i < chats.count(); i++) {
		const ChatItem chat = chats.at(i);
		if (chat.msgw == Chat.msgw) {
			delete chat.item;
			chats.removeAt(i); // 只能遍历找到这个变量不能直接删，因为可能是副本
			return;
		}
	}
}

void SerialPortChat::setEnabled(bool enabled) {
	ui->sendBtn->setEnabled(enabled);
	ui->intervalSend->setEnabled(enabled);
}

void SerialPortChat::readyRead(const QString utf8, const QString gbk, const QByteArray hex, Encode_t stringPrefer) {
	QString _hex = hexstr(hex, true);
	QDateTime time = QDateTime::currentDateTime();
	addMessage(utf8, time, QNChatMessage::User_She, _hex, gbk, stringPrefer);
//	qDebug() << utf8 << time.toString("yyyy-MM-dd hh:mm:ss.zzz") << _hex << gbk << stringPrefer;
}

void SerialPortChat::setChatItemEncode(QNChatMessage *msgw, Encode_t encode) {
	//其实应该用指针的方法解决的，但是反正就这一个功能需要修改，懒得弄那些麻烦的功能，干脆直接新建一个方法算了。
	for (int i = 0; i < chats.count(); i++)
		if (chats.at(i).msgw == msgw) {
			chats[i].encode = encode;
			return;
		}
}

void SerialPortChat::on_intervalSend_clicked(bool checked) {
	ui->autoClearBtn->setEnabled(!checked);
	ui->intervalSendValue->setEnabled(!checked);
	ui->autoClearBtn->setChecked(checked ? false : SendAutoClr);
	if (checked) {
		int ms = ui->intervalSendValue->value();
		sendTimer->setInterval(ms);
		sendTimer->start();
	} else {
		sendTimer->stop();
		ui->intervalSend->setEnabled(false); // 取消定时发送后先禁用一秒中冷却才能再次开启，避免一取消又立即重新打开
		QTimer::singleShot(1000, this, [&]() {
			if (ui->sendBtn->isEnabled()) ui->intervalSend->setEnabled(true);
		});
	}
}
