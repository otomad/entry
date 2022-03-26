#include "signalgenerator.h"
#include "ui_signalgenerator.h"
#include "mainwindow.h"
#include "common.h"

static MainWindow *mainWindow;
#define waitMs 500
#define USE_MODIFY_SEND false

QStringList SignalGenerator::commandName = {"设置频率","设置幅度","设置波形","设置频率步进","读取参数","设置扫频参数"};

SignalGenerator::SignalGenerator(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SignalGenerator) {
	ui->setupUi(this);
	mainWindow = (MainWindow *)parentWidget();
//	spinBoxMapper = new QSignalMapper(this);
	setEnabled(false);
	//重载信号和槽
	for (int j = 0; j < 4; j++) modified[j] = true;
	int i = 1;
	#define iconnect(obj, signal, slot) connect(ui->obj, signal, this, [&, i]() slot); i++
	#if USE_MODIFY_SEND
	iconnect(freqBox, &QSpinBox::textChanged, { modify(i); });
	iconnect(ampBox, &QDoubleSpinBox::textChanged, { modify(i); });
	iconnect(waveformBox, &QComboBox::currentTextChanged, { modify(i); });
	iconnect(freqStepBox, &QSpinBox::textChanged, { modify(i); });
	#else
	i = 5;
	#endif
	iconnect(readValueBtn, &QPushButton::clicked, { btnDo(i); });
	iconnect(scanBtn, &QPushButton::clicked, { btnDo(i); });
	#undef iconnect
}

SignalGenerator::~SignalGenerator() {
	delete ui;
}

void SignalGenerator::send(QByteArray hex) {
	mainWindow->send(hex);
}

void SignalGenerator::send(QString hexStr) {
	mainWindow->send(hexStr, Encode::HEX);
}

uint CrcCheckSum(uchar *puchMsg, uint usDataLen) { // CRC16/XMODEM 校验码
	uint wCRCin = 0x0000;
	uint wCPoly = 0x1021;
	uchar wChar = 0;
	while (usDataLen--) {
		wChar = *(puchMsg++);
		wCRCin ^= (uint)(wChar << 8);
		for (int i = 0; i < 8; i++)
			if (wCRCin & 0x8000)
				wCRCin = (wCRCin << 1) ^ wCPoly;
			else
				wCRCin = wCRCin << 1;
	}
	return (wCRCin);
}

uint CrcCheckSum(QString hex) {
	hex.remove(QRegExp("[^A-Fa-f0-9]"));
	uint len = (uint)hex.length();
	if (len % 2) {
		hex = "0" + hex;
		len++;
	}
	const uint byte = len / 2;
//	uchar *data = new uchar[byte];
	QByteArray data;
	for (uint i = 0; i < byte; i++) {
		const uint j = i * 2;
		bool ok;
		data[i] = (char)hex.midRef((int)j, 2).toInt(&ok, 16);
		if (!ok) qDebug("Cannot transform hex data!");
	}
	const uint crc = CrcCheckSum((uchar *)data.data(), byte);
	/*qDebug(QString("Original Hex: %1\nCRC16/XMODEM: %2")
		   .arg(hex).arg(crc, 4, 16, QLatin1Char('0')).toLatin1()
		  );*/
	qDebug("Original Hex: %s\nCRC16/XMODEM: %04x", hex.toStdString().c_str(), crc);
//	delete[] data;
	return crc;
}

QString int2hexQs(int num, int byte) {
	const int fieldWidth = byte * 2; // 一字节两位十六进制数
	return QString("%1").arg(num, fieldWidth, 16, QLatin1Char('0'));
}

void SignalGenerator::sendCommand(int command) {
	QString uart;
	bool read = false;
	switch (command) {
		case 1: //设置频率
			uart = "0523"
				 + int2hexQs(ui->freqBox->value(), 3);
			break;
		case 2: //设置幅度
			uart = "0322"
				 + int2hexQs((int)(ui->ampBox->value() * 10), 1);
			break;
		case 3: //设置波形
			uart = "0320"
				 + int2hexQs(ui->waveformBox->currentIndex(), 1);
			break;
		case 4: //设置频率步进值
			uart = "0421"
				 + int2hexQs(ui->freqStepBox->value(), 2);
			break;
		case 5: //读取参数
			uart = "032400";
			read = true;
			break;
		case 6: //设置扫频参数
			uart = "0c25"
				 + int2hexQs(ui->lwrFreqBox->value(), 3)
				 + int2hexQs(ui->uprFreqBox->value(), 3)
				 + int2hexQs(ui->scanFreqStepBox->value(), 2)
				 + int2hexQs(ui->scanPeriodBox->value(), 1)
				 + int2hexQs(ui->scanLoopRadio->isChecked(), 1);
			break;
		default:
			qDebug("Unexpected Token!");
			return;
	}
	success = (read ? "09" : "02") + uart.mid(2, 2);
	Command = command;
	QString crc = getCrc(uart);
	QString result = startCode + uart + crc + endCode;
	qDebug(result.toLatin1());
	send(result);
}

QString getCrc(QString data) {
	QString crc = int2hexQs((int)CrcCheckSum(data), 2);
	// 此时转换出来的CRC有点问题，长短不一，但最后两个字节是有效的，但也高低字节顺序弄反了。因此干脆治标不治本，不抓病根，直接用字符串暴力解决。
	crc = crc.right(4);
	crc = crc.right(2) + crc.left(2);
	return crc;
}

#define getCommandName commandName[Command - 1]

#define itsWrong {\
	qDebug(QString("Error in: %3!\nExpect Check: %1\nFact Check: %2")\
	.arg(expectCheck, check, getCommandName).toUtf8());\
	errorList.append({ getCommandName, expectCheck, check });\
}

#define readRet(x) {readResult = x; startRead = false; return x;}

int SignalGenerator::readyRead(const QByteArray buffer) {
	if (!startRead) readRet(-1);
	QString check = buffer.toHex().toUpper(), expectCheck, crc;
	qDebug(("Receive: " + check).toLatin1());
	if (success != "0924") {
		crc = getCrc(success).toUpper();
		expectCheck = startCode + success + crc + endCode;
		if (check != expectCheck) {
			itsWrong;
			readRet(1);
		} else readRet(0);
	} else {
		bool correct = true;
		if (check.length() != 30) correct = false;
		if (check.left(4) != startCode || check.right(4) != endCode || check.mid(4, 4) != success) correct = false;
		expectCheck = check.mid(4, 18);
		crc = getCrc(expectCheck).toUpper();
		expectCheck = startCode + expectCheck + crc + endCode;
		if (crc != check.right(8).left(4)) correct = false;
		if (!correct) {
			itsWrong;
			readRet(1);
		} else {
			check = check.mid(8, 14);
			QString waveform[3] = {"正弦波","方波","三角波"};
			ui->curValueText->setText(
						QString("步进频率：%1\n电压：%2\n频率值：%3\n波形种类：%4")
						.arg(check.leftRef(4).toInt(nullptr, 16))
						.arg((double)(check.midRef(4, 2).toInt(nullptr, 16)) / 10)
						.arg(check.midRef(6, 6).toInt(nullptr, 16))
						.arg(waveform[check.rightRef(2).toInt(nullptr, 16)])
						);
			readRet(0);
		}
	}
}

void SignalGenerator::setEnabled(bool enabled) {
	const QList <QPushButton *> buttons({
											ui->setValueBtn,
											ui->readValueBtn,
											ui->scanBtn
										});
	foreach (QPushButton *el, buttons)
		el->setEnabled(enabled);
	if (!enabled) resetModified(true);
}

void SignalGenerator::connectDo(bool open) {
	if (!open) {
		ui->curValueText->clear();
	}
}

void SignalGenerator::Modified::reset() {
	freqStep = false;
	amp = false;
	freq = false;
	waveform = false;
}

void SignalGenerator::resetModified(bool allTrue = false) {
	for (int i = 0; i < 4; i++) modified[i] = allTrue;
}

void SignalGenerator::modify(int command) {
	if (!enableModify) return;
	command--;
	const QString style = "color: cyan;";
	const QList <QLabel *> labels({
									ui->freqLbl,
									ui->ampLbl,
									ui->waveformLbl,
									ui->freqStepLbl
								 });
	for (int j = 0; j < 2; j++)
		for (int i = 0; i < labels.count(); i++)
			if (command == i || j) {
				modified[i] = !j;
				labels[i]->setStyleSheet(j ? "" : style);
				if (!j) return;
			}
}

void SignalGenerator::on_setValueBtn_clicked() {
	btnDo();
	for (int i = 0; i < 4; i++)
		#if USE_MODIFY_SEND
		if (modified[i]) // 解除注释掉这行即可根据是否修改参数来决定发送数据
		#endif
			sendQueue.append(i + 1);
	startSend();
}

void SignalGenerator::btnDo(int command) {
	errorList.clear();
	resultList.clear();
	if (command) {
		sendQueue.clear();
		sendQueue.append(command);
		startSend();
	}
	iniSave();
}

void SignalGenerator::startSend() {
	if (sendQueue.isEmpty()) {
		modify(0);
		Command = 0;
		if (!errorList.isEmpty()) {
			QString errors;
			for (int i = 0; i < errorList.count(); i++)
				errors += "错误操作：" + errorList[i].command + "\n"
						+ "预期返回：" + errorList[i].expectCheck + "\n"
						+ "实际返回：" + errorList[i].check + "\n\n";
			QMessageBox::critical(this, "数据返回错误", errors);
		}
		QString showResult;
		for (int i = 0; i < resultList.count(); i++) {
			if (i) showResult += "，";
			showResult += resultList[i].command
					+ (readResult==1 ? "错误" : readResult==-1 ? "无效" : "成功");
		}
		if (resultList.count()) showResult += "。";
		mainWindow->showStatus(showResult, 5000);
		return;
	}
	QString currentCommandName = commandName[sendQueue[0] - 1];
	mainWindow->showStatus(QString("正在%1……").arg(currentCommandName), 5000);
	readResult = -1;
	startRead = true;
	setEnabled(false);
	sendCommand(sendQueue[0]);
	QTimer::singleShot(waitMs, this, [&, currentCommandName]() {
//		mainWindow->showStatus(readResult==1?"数据返回错误":readResult==-1?"没有数据返回":"数据发送成功", 5000);
		resultList.append({ currentCommandName, readResult });
		startRead = false;
		setEnabled(true);
		sendQueue.pop_front();
		startSend();
	});
}

void SignalGenerator::iniSave() {
	configIni->beginGroup("SignalGenerator");
	configIni->setValue("FreqStep", ui->freqStepBox->value());
	configIni->setValue("Amp", ui->ampBox->value());
	configIni->setValue("Freq", ui->freqBox->value());
	configIni->setValue("Waveform", ui->waveformBox->currentIndex());
	configIni->setValue("LwrFreq", ui->lwrFreqBox->value());
	configIni->setValue("UprFreq", ui->uprFreqBox->value());
	configIni->setValue("ScanFreqStep", ui->scanFreqStepBox->value());
	configIni->setValue("ScanPeriod", ui->scanPeriodBox->value());
	configIni->setValue("ScanWay", ui->scanLoopRadio->isChecked());
	configIni->endGroup();
}

void SignalGenerator::personalizeCheck(int freqStep, double amp, int freq, int waveform, int lwrFreq, int UprFreq, int ScanFreqStep, int ScanPeriod, bool scanWay) {
	enableModify = false;
	ui->freqStepBox->setValue(freqStep);
	ui->ampBox->setValue(amp);
	ui->freqBox->setValue(freq);
	ui->waveformBox->setCurrentIndex(waveform);
	ui->lwrFreqBox->setValue(lwrFreq);
	ui->uprFreqBox->setValue(UprFreq);
	ui->scanFreqStepBox->setValue(ScanFreqStep);
	ui->scanPeriodBox->setValue(ScanPeriod);
	if (!scanWay) ui->scanOnceRadio->setChecked(true);
	else ui->scanLoopRadio->setChecked(true);
	enableModify = true;
}
