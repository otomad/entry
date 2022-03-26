#ifndef SERIALPORTCHAT_H
#define SERIALPORTCHAT_H

#include <QWidget>
#include <QListWidgetItem>
#include <QDateTime>
#include "chatmessage/qnchatmessage.h"
#include "common.h"

namespace Ui {
	class SerialPortChat;
}

class SerialPortChat : public QWidget {
	Q_OBJECT

	public:
		explicit SerialPortChat(QWidget *parent = nullptr);
		~SerialPortChat() override;
		void dealMessage(QNChatMessage *messageW, QListWidgetItem *item, QString text, QString time, QNChatMessage::User_Type type);
		void dealMessageTime(QString curMsgTime);
		void addMessage(QString utf8, QDateTime time, bool isSend = true, QString hex = "", QString gbk = "", Encode_t stringPrefer = Encode::HEX);
		void addMessage(QString utf8, QDateTime time, QNChatMessage::User_Type user, QString hex = "", QString gbk = "", Encode_t stringPrefer = Encode::HEX);
		void setConf(Encode_t sendEncode, Encode_t recvEncode, bool sendNewLine, bool sendAutoClr);
		struct ChatItem {
			QDateTime time;
			QString utf8;
			QString gbk;
			QString hex;
			QNChatMessage::User_Type who;
			Encode_t encode;
			QNChatMessage *msgw;
			QListWidgetItem *item;
			Encode_t stringPrefer;
		};
		void setEnabled(bool enabled);
		void readyRead(const QString utf8, const QString gbk, const QByteArray hex, Encode_t stringPrefer);

	protected:
		void resizeEvent(QResizeEvent *event) override;

	private slots:
		void on_sendBtn_clicked();
		void on_sendBox_textChanged();
		void on_chatList_customContextMenuRequested(const QPoint &pos);
		void on_intervalSend_clicked(bool checked);

	private:
		Ui::SerialPortChat *ui;
		QList <ChatItem> chats;
		bool getChatItem(const QListWidgetItem *item, ChatItem *chat);
		void clearList();
//		static const ChatItem nullChatItem;
//		bool isLegalChatItem(ChatItem chat);
		void removeChatItem(ChatItem chat);
		void deleteSeedSlot(ChatItem chat);
		void setChatItemEncode(QNChatMessage *msgw, Encode_t encode);
		bool isChanging = false; //不弄个判断会死循环的
		QTimer *sendTimer = new QTimer(this);
};

//const SerialPortChat::ChatItem nullChatItem = {0, "", "", QNChatMessage::User_System, false, nullptr, nullptr};

#endif // SERIALPORTCHAT_H
