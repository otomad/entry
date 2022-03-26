#include "serialportassistant.h"
#include "ui_serialportassistant.h"
#include "mainwindow.h"
#include <QButtonGroup>

static MainWindow *mainWindow;

SerialPortAssistant::SerialPortAssistant(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SerialPortAssistant) {
	ui->setupUi(this);
	mainWindow = (MainWindow *)parentWidget();

	//单选框分组 //槽函数
	QList <QRadioButton *> radios ({
		ui->sendUtf8,
		ui->sendGbk,
		ui->sendHex,
		ui->receiveUtf8,
		ui->receiveGbk,
		ui->receiveHex
	});
	QButtonGroup *sendEncodeGroup = new QButtonGroup(this), *recvEncodeGroup = new QButtonGroup(this);
	for (int i = 0; i < radios.count(); i++) {
		QRadioButton *radio = radios.at(i);
		QButtonGroup *group = i < 3 ? sendEncodeGroup : recvEncodeGroup;
		group->addButton(radio);
		connect(radio, &QRadioButton::toggled, this, [&]() {
			Encode_t
				sendEncode =
					ui->sendHex->isChecked() ? Encode::HEX :
					ui->sendGbk->isChecked() ? Encode::GBK : Encode::UTF8,
				recvEncode =
					ui->receiveHex->isChecked() ? Encode::HEX :
					ui->receiveGbk->isChecked() ? Encode::GBK : Encode::UTF8;
			if (sendEncode == Encode::HEX) examineHex(ui->sendText);
			mainWindow->asstConf.setEncode(sendEncode, recvEncode);
		});
	}
	connect(ui->sendNewLine, &QCheckBox::toggled, this, [&](bool checked) { mainWindow->asstConf.setSendNewLine(checked); });
	connect(ui->receiveNewLine, &QCheckBox::toggled, this, [&](bool checked) { mainWindow->asstConf.setRecvNewLine(checked); });
}

SerialPortAssistant::~SerialPortAssistant() {
	delete ui;
}

QString hexstr(const QByteArray buffer, bool beautify) {
	QString hex = buffer.toHex().toUpper();
	if (beautify) hex = beautifyHex(hex);
	return hex;
}

#define RecvEncode mainWindow->asstConf.recvEncode()
#define SendEncode mainWindow->asstConf.sendEncode()
#define RecvNewLine mainWindow->asstConf.recvNewLine()
#define SendNewLine mainWindow->asstConf.sendNewLine()

void SerialPortAssistant::readyRead(const QString utf8, const QString gbk, const QByteArray hex, Encode_t stringPrefer) {
	QString recv = ui->receiveText->toPlainText(); //从界面中读取以前收到的数据
	QString newStr;
	if (RecvEncode == Encode::HEX) newStr = (RecvNewLine ? "\r\n" : " ") + hexstr(hex);
	else {
		Encode_t encode = stringPrefer != Encode::HEX ? stringPrefer : RecvEncode;
		newStr = (RecvNewLine ? "\r\n" : "") + (encode == Encode::UTF8 ? utf8 : gbk);
	}
	recv += newStr;
	ui->receiveText->setPlainText(recv.trimmed());
	ui->receiveText->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
}

QString sendNewLineCheck(QTextEdit *txtBox, bool check) {
	QString text = txtBox->toPlainText();
	if (text.isEmpty() || !check) return text;
	if (text.right(2) == "\r\n") return text;
	else if (text.right(1) == "\r" || text.right(1) == "\n") {
		if (text.right(1) == "\n") text[text.length() - 1] = '\r';
		if (text.right(1) == "\r") text += '\n';
	} else text += "\r\n";
//	txtBox->setPlainText(text);
	return text;
}

void SerialPortAssistant::on_send_clicked() {
	QString text = sendNewLineCheck(ui->sendText, SendNewLine);
	mainWindow->send(text, SendEncode);
	/*mainWindow->send(
		SendEncode == Encode::HEX ? QString2Hex(avoidOdd(ui->sendText->toPlainText())) :
		ui->sendText->toPlainText().toUtf8()
	);*/
}

void SerialPortAssistant::on_sendText_textChanged() {
	if (SendEncode == Encode::HEX && !isChanging) {
		isChanging = true;
		examineHex(ui->sendText);
		isChanging = false;
	}
}

void examineHex(QTextEdit *edit) {
	QString text = edit->toPlainText(),
			text_orig = text;
	//		text.remove(QRegExp("[^A-Fa-f0-9 ]|^ *|(?<= ) ")); //保留数字与大小写字母A~F；去除开头空格，去除末尾空格，去除重复的空格（跟在空格后面的空格） // 为什么这行用不了？
	text.remove(QRegExp("[^A-Fa-f0-9 ]|^\\s*"));
	text.replace(QRegExp("\\s{2,}"), " ");
	//		text = text.trimmed();
	qDebug(text.toUtf8());
	if (text_orig != text) {
		QTextCursor tmpCursor = edit->textCursor();
		int position = tmpCursor.position();
		edit->setPlainText(text);
		if (position > edit->textCursor().position())
			edit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
		else {
			tmpCursor.setPosition(position);
			edit->setTextCursor(tmpCursor);
		}
	}
}

void SerialPortAssistant::setEnabled(bool enabled) {
	ui->send->setEnabled(enabled);
}

QString avoidOdd(QString str) {
	str.remove(QRegExp("[^A-Fa-f0-9]"));
	if (str.length() % 2 == 1) {
		str = "0" + str;
		mainWindow->showStatus("你特么发的数据还有半个字节的你资道么？", 5000);
	}
	return str;
}

void SerialPortAssistant::setConf(Encode_t sendEncode, Encode_t recvEncode, bool sendNewLine, bool recvNewLine) {
	ui->sendNewLine->setChecked(sendNewLine);
	ui->receiveNewLine->setChecked(recvNewLine);
	auto checkRadio = [&](Encode_t value, QRadioButton *radio, Encode_t condition) { if (value == condition) radio->setChecked(true); };
	checkRadio(sendEncode, ui->sendUtf8, Encode::UTF8);
	checkRadio(sendEncode, ui->sendGbk, Encode::GBK);
	checkRadio(sendEncode, ui->sendHex, Encode::HEX);
	checkRadio(recvEncode, ui->receiveUtf8, Encode::UTF8);
	checkRadio(recvEncode, ui->receiveGbk, Encode::GBK);
	checkRadio(recvEncode, ui->receiveHex, Encode::HEX);
}
