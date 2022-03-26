#ifndef SERIALPORTASSISTANT_H
#define SERIALPORTASSISTANT_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "common.h"
//#include "mainwindow.h"

namespace Ui {
	class SerialPortAssistant;
}

class SerialPortAssistant : public QWidget
{
		Q_OBJECT

	public:
		explicit SerialPortAssistant(QWidget *parent = nullptr);
		~SerialPortAssistant() override;
		void setEnabled(bool enabled);
		void readyRead(const QString utf8, const QString gbk, const QByteArray hex, Encode_t stringPrefer);
		void setConf(Encode_t sendEncode, Encode_t recvEncode, bool sendNewLine, bool recvNewLine);

	private slots:
		void on_send_clicked();
		void on_sendText_textChanged();

	private:
		Ui::SerialPortAssistant *ui;
		bool isChanging = false; //不弄个判断会死循环的
};

#endif // SERIALPORTASSISTANT_H
