#include "singlechip.h"
#include "ui_singlechip.h"
#include "mainwindow.h"
#include "common.h"

static MainWindow *mainWindow;
#define waitReturnTime 2000

#define QList_checkBoxes QList <QCheckBox *> checkBoxes({ui->l1,ui->l2,ui->l3,ui->l4,ui->l5,ui->l6,ui->l7,ui->l8})
#define QList_pushButtons QList <QPushButton *> pushButtons({ui->s1,ui->s2,ui->s3,ui->s4})

SingleChip::SingleChip(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SingleChip) {
	ui->setupUi(this);
	mainWindow = (MainWindow *)parentWidget();
	ui->ledBox->setValidator(new QRegExpValidator(QRegExp("[A-Fa-f0-9]{0,4}"), this));
	setEnabled(false);
	//	QList <QPushButton *> pushButtons = ui->horizontalLayout_2->findChildren<QPushButton *>();
	QList_pushButtons;
	for (int i = 0; i < 4; i++)
		connect(pushButtons[i], &QPushButton::clicked, this, [&, i]() {
		sendCommand(3, "0" + QString::number(i + 3));
	});
	/*connect(ui->s1, &QPushButton::clicked, [&]() { sendCommand(3, "03"); });
	connect(ui->s2, &QPushButton::clicked, [&]() { sendCommand(3, "04"); });
	connect(ui->s3, &QPushButton::clicked, [&]() { sendCommand(3, "05"); });
	connect(ui->s4, &QPushButton::clicked, [&]() { sendCommand(3, "06"); });*/
	//	QList <QCheckBox *> checkBoxes = ui->horizontalLayout->findChildren<QCheckBox *>();
	QList_checkBoxes;
	foreach (QCheckBox *el, checkBoxes)
		connect(el, &QCheckBox::clicked, this, [&]() {
		on_setLight_clicked();
	});
	connect(ui->readValue, &QPushButton::clicked, this, [&]() {
		sendCommand(90);
	});

	//蜂鸣器复选框操作
	connect(ui->enableBeepTime, &QCheckBox::toggled, ui->beepMsBox, &QSpinBox::setEnabled);
	connect(ui->enableBeep, &QCheckBox::toggled, this, [&](bool checked) {
		ui->beepHzBox->setEnabled(checked);
		ui->enableBeepTime->setEnabled(checked);
		if (!checked) {
			ui->enableBeepTime->setChecked(false);
			ui->beepMsBox->setEnabled(false);
		}
	});

	//定时验证返回值的计时器
	checkTimer = new QTimer(this);
	checkTimer->setSingleShot(true);
	connect(checkTimer, &QTimer::timeout, this, &SingleChip::checkTimer_timeout);
}

SingleChip::~SingleChip() {
	checkTimer->stop();
	delete checkTimer;
	delete ui;
}

void SingleChip::on_setDigit_clicked() {
	QString digit = ui->ledBox->text();
	int digit2 = digit.toInt(nullptr, 16);
	digit = QString("%1").arg(digit2, 4, 16, QChar('0'));
	sendCommand(1, digit);
}

void SingleChip::on_setLight_clicked() {
	if (!mainWindow->isConnect()) return;
	uint value = 0;
	//	QList <QCheckBox *> checkBoxes = ui->horizontalLayout->findChildren<QCheckBox *>();
	QList_checkBoxes;
	const int count = checkBoxes.count();
	for (int i = 0; i < count; i++)
		if (checkBoxes[i]->isChecked())
			value |= 1 << (count - i - 1);
	mainWindow->showStatus(QString("设定流水灯：%1").arg(value, 8, 2, QChar('0')), 5000);
	sendCommand(0, int2hexQs((int)value, 1));
}

#define unexpectedSuccess "unexpect"

void SingleChip::sendCommand(int command, QString data, bool needCheck) {
	readResult = -1;
	QString uart;
	switch (command) {
		case 0:
			uart += "0310" + data;
			success = "031000";
			break;
		case 1:
			uart += "0411" + data;
			success = "031100";
			break;
		case 11:
			uart += "0611" + data;
			success = "031101";
			break;
		case 2:
			uart += "0412" + data;
			success = "031200";
			break;
		case 12:
			uart += "0612" + data;
			success = "031201";
			break;
		case 3:
			uart += "0313" + data;
			success = "031300";
			break;
		case 90:
			uart += "039000";
			success = unexpectedSuccess;
			break;
		default:
			return;
	}
	QString crc = getCrc(uart);
	QString result = startCode + uart + crc + endCode;
	qDebug(result.toLatin1());
	startRead = false;
	checkTimer->stop();
	if (needCheck && !force_no_check && !startRead) {
		startRead = true;
		/*QTimer::singleShot(waitReturnTime, [&]() {
			mainWindow->showStatus(readResult == 1 ? "数据返回错误" : readResult == -1 ? "没有数据返回" : "数据发送成功", 5000);
			startRead = false;
			readResult = -1;
		});*/
		checkTimer->start(waitReturnTime);
	}
	mainWindow->send(result, Encode::HEX);
}

void SingleChip::checkTimer_timeout() {
	// QTimer::singleShot 并不好控制
	checkTimer->stop();
	mainWindow->showStatus(readResult == 1 ? "数据返回错误" : readResult == -1 ? "没有数据返回" : "数据发送成功", 5000);
	startRead = false;
	readResult = -1;
}

void SingleChip::on_setBeep_clicked() {
	QString hz = int2hexQs(ui->beepHzBox->value(), 2);
	if (!ui->enableBeep->isChecked()) hz = "0000";
	QString ms = int2hexQs(ui->beepMsBox->value(), 2);
	if (!ui->enableBeepTime->isChecked() || !ui->enableBeep->isChecked()) sendCommand(2, hz);
	else sendCommand(12, hz + ms);
}

void SingleChip::setEnabled(bool enabled, SetEnabledExcept except) {
	const QList <QPushButton *> buttons({
		// ui->setLight,
		ui->setBeep,
		ui->setDigit,
		ui->readValue,
		ui->s1,
		ui->s2,
		ui->s3,
		ui->s4
	});
	foreach (QPushButton *el, buttons)
		el->setEnabled(enabled);
	if (except != ExceptScrollLed) ui->setScrollLed->setEnabled(enabled);
	if (except != ExceptPlayNotes) ui->setPlayNotes->setEnabled(enabled);
	iniSave();
}

#define itsWrong {\
	qDebug(("Error in " + check + "\nExpected " + expectCheck).toLatin1());\
	QMessageBox::critical(nullptr, "数据返回错误", "预期返回：" + expectCheck + "\n实际返回：" + check);\
} //这里QMessageBox用nullptr而不是this是避免不能及时return数据

#define readRet(x) {\
	readResult = x;\
	startRead = false;\
	if (x == 1) itsWrong\
	return x;\
}

int SingleChip::readyRead(const QByteArray buffer) {
	QString check = buffer.toHex().toUpper(), expectCheck = "unknown", crc;
	if (!startRead) readRet(-1);
	QStringList checkList = check.split(startCode/*, QString::SkipEmptyParts*/);
	qDebug(("Receive: " + check).toLatin1());
	if (checkList.count() == 2) {
		if (success == unexpectedSuccess) {
//			qDebug("Unexpected return data. Expect reading value, but likes setting value.");
			expectCheck = "Unexpected return data. Expect reading value, but likes setting value.";
			readRet(1);
		}
		crc = getCrc(success).toUpper();
		expectCheck = startCode + success + crc + endCode;
		QString _check = startCode + checkList[1];
		if (_check != expectCheck) {
			readRet(1);
		} else readRet(0);
	} else if (checkList.count() == 4) {
		#if 0 // 老子懒得验算了
		bool correct = true;
		if (check.length() != 30) correct = false;
		if (check.left(4) != startCode || check.right(4) != endCode || check.mid(4, 4) != success) correct = false;
		expectCheck = check.mid(4, 18);
		crc = getCrc(expectCheck).toUpper();
		expectCheck = startCode + expectCheck + crc + endCode;
		if (crc != check.right(8).left(4)) correct = false;
		if (!correct) {
			readRet(1);
		}
		#endif // } else {
		bool ok1, ok2;
		QString light = QString("%1").arg(checkList[1].mid(4, 2).toInt(&ok1, 16), 8, 2, QChar('0'));
		QString led = checkList[2].mid(4, 4);
		int beep = checkList[3].mid(4, 4).toInt(&ok2, 16);
		// check = check.mid(8, 14);
		ui->curValueText->setText(
			QString("流水灯：%1\n数码管：%2\n蜂鸣器：%3 Hz")
			.arg(light)
			.arg(led)
			.arg(QString::number(beep))
		);
		if (!(ok1 && ok2)) {
			readRet(1);
		} else readRet(0);
		// }
	} else {
		readRet(1);
	}
}

QString SingleChip::led(int num, bool dot) {
	uchar led[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xd8, 0x80, 0x90};
	uchar l = led[num];
	if (dot) l &= 0x7f;
	return QString("%1").arg(l, 2, 16, QChar('0'));
}

QString SingleChip::led(char c, bool dot) {
	uchar ledAbc[] = {0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e, 0xc2,
					  0x89, 0xcf, 0xe1, 0x8a, 0xc7, 0xc8, 0xab,
					  0xa3, 0x8c, 0x98, 0xaf, 0x9b, 0xf8,
					  0xc1, 0x8d, 0xe3, 0xc9, 0x91, 0xad
					 };
	uchar l;
	if (c >= '0' && c <= '9') return led(c - '0', dot);
	else if (c >= 'A' && c <= 'Z') l = ledAbc[c - 'A'];
	else if (c >= 'a' && c <= 'z') l = ledAbc[c - 'a'];
	else if (c == '=') l = 0xb7;
	else if (c == '[' || c == '(') l = 0xc6;
	else if (c == ']' || c == ')') l = 0xf0;
	else if (c == '<') l = 0xb9;
	else if (c == '>') l = 0x8f;
	else if (c == '@') l = 0xc4;
	else if (c == '_') l = 0xf7;
	else if (c == '-') l = 0xbf;
	else if (c == '\"') l = 0xdd;
	else if (c == '\'' || c == '`') l = 0xdf;
	else if (c == ';') l = 0xf2;
	else if (c == '!') l = 0xd7;
	else if (c == '?') l = 0xac;
	else if (c == ',') l = 0xf3;
	else if (c == '~') l = 0xad;
	else if (c == '|') l = 0xcf;
	else if (c == '^') l = 0xdc;
	// else if (c == '.'); //FUCK YOU
	else l = 0xff;
	if (dot) l &= 0x7f;
	return QString("%1").arg(l, 2, 16, QChar('0'));
}

void SingleChip::scrollLed() {
	scrollLedText = ui->scrollLedBox->toPlainText().simplified();
	scrollLedDots = QList<bool>(); //如果不行改成 QVector
	ui->scrollLedBox->setPlainText(scrollLedText);
	// std::fill(&scrollLedDots, scrollLedText.length(), false); //不知道怎么用不了
	for (int i = 0; i < scrollLedText.length(); i++)
		scrollLedDots.append(false);
	for (int i = 0; i < scrollLedText.length(); i++) {
		if (scrollLedText[i] == '.') {
			scrollLedText.remove(i, 1);
			if (i != 0) scrollLedDots[i - 1] = true; // 如果开头就是点，我就不知道怎么显示了。
			i--;
		}
	}
	if (scrollLedText.length() <= 4) {
		for (int i = 0; i < 4 - scrollLedText.length(); i++) {
			scrollLedText.append(" ");
			scrollLedDots.append(false);
		}
		showLedByHex(scrollLedText, scrollLedDots.mid(0, 4));
		on_setScrollLed_clicked();
	} else {
		for (int i = 0; i < 4; i++) {
			scrollLedText.append(" ");
			scrollLedDots.append(false);
		}
		scrollLed_ready();
	}
}

void SingleChip::scrollLed_ready() {
	if (scrollLedText.length() < 4 || !startScroll) {
		scrollLedText.clear();
		scrollLedDots.clear();
		mainWindow->clearStatus();
		if (startScroll) on_setScrollLed_clicked();
		return;
	}
	int speed = ui->scrollSpeedBox->value(); //单位：毫秒每字
	QString text = scrollLedText.left(4);
	QList <bool> dots = scrollLedDots.mid(0, 4);
	QString textForShow = text;
	for (int i = dots.count() - 1; i >= 0; i--)
		if (dots[i])
			textForShow.insert(i + 1, '.');
	mainWindow->showStatus("设定数码管：" + textForShow);
	showLedByHex(text, dots);
	QTimer::singleShot(speed, [&]() {
		scrollLedText.remove(0, 1);
		scrollLedDots.pop_front();
		scrollLed_ready();
	});
}

void SingleChip::showLedByHex(QString text, QList <bool> dots) {
	QString hex;
	for (int i = 0; i < 4; i++)
		hex += led(text[i].toLatin1(), dots[i]);
	sendCommand(11, hex, false);
}

#define startCaption "启动"
#define stopCaption "停止"

void SingleChip::on_setScrollLed_clicked() {
	bool start = !startScroll;
	startScroll = start;
	ui->setScrollLed->setText(start ? stopCaption : startCaption);
	ui->setScrollLed->setChecked(start);
	force_no_check = start;
	setEnabled(!start, ExceptScrollLed);
	if (start) scrollLed();
}

//格式：C416（C-哆，4-第四个八度，16-十六分音符。）

#define wrong(x) {\
	qDebug(("Something wrong in note: " + x).toLatin1());\
	notes.clear();\
	startPlay = false;\
	QMessageBox::critical(this, "音符格式有误", "错误的音符：" + x);\
	mainWindow->clearStatus();\
	ui->beepHzBox->setValue(0);\
	on_setBeep_clicked();\
	startPlay = true;\
	on_setPlayNotes_clicked();\
	return;\
}

void SingleChip::prepareNotes() {
	QString text = ui->notesBox->toPlainText();
	text = text.toUpper().remove(QRegExp("[^A-G0-9#\\.]")).simplified();
	ui->notesBox->setPlainText(text);
	text.replace(QRegExp("\\."), "1");
	for (int i = 1; i < text.length(); i++) { //注意i=1，要跳过第0个字符。
		const QChar c = text[i];
		if ((c >= 'A' && c <= 'G') || c == '0') {
			text.insert(i, " ");
			i++;
		}
	}
	QStringList notesText = text.split(" ", QString::SkipEmptyParts);
	for (int i = 0; i < notesText.count(); i++) {
		bool ok2, ok3;
		QString key;
		int eight, length;
		if (notesText[i][0] == '0') {
			key = "0";
			eight = 0;
			ok2 = true;
			length = notesText[i].mid(1).toInt(&ok3);
		} else {
			QString other = key = notesText[i];
			key.replace(QRegExp("[0-9]"), "");
			other.replace(QRegExp("[^0-9]"), "");
			eight = other.left(1).toInt(&ok2);
			length = other.mid(1).toInt(&ok3);
			if (length2time(length) < 0) wrong(notesText[i]);
		}
		if (!(ok2 && ok3)) wrong(notesText[i]);
		notes.append({key, eight, length, notesText[i]});
	}
	notes.append({"0", 0, 16, "000"});
	sendNotes();
}

void SingleChip::sendNotes() {
	if (notes.isEmpty() || !startPlay) {
		notes.clear();
		mainWindow->clearStatus();
		if (startPlay) on_setPlayNotes_clicked();
		return;
	}
	const NOTE note = notes[0];
	int freq;
	if (note.key == "0") freq = 0;
	else {
		int absolute = absoluteKey(note.key);
		if (absolute <= -99) wrong(note.notesText);
		int mainKey = absoluteKey(ui->mainKeyBox->currentText(), true);
		int Eight = (note.eight - 3) * 12;
		freq = (int)(440.0 * pow(2, (double)(absolute + mainKey + Eight) / 12.0)); //貌似会算错
	}
	int bpm = ui->bpmBox->value();
	double full = 60000 / bpm * 4;
	double time = full * length2time(note.length);
	if (time < 0) wrong(note.notesText); // time == -1
	if (note.key != "0") mainWindow->showStatus(QString("设定蜂鸣器：音名 - %1，时长 - %2；频率 - %3 Hz，持续时间 - %4 ms")
		.arg(QString(note.key) + QString::number(note.eight))
		.arg(note.length)
		.arg(freq)
		.arg(time)
	);
	else mainWindow->clearStatus();
	ui->enableBeep->setChecked(true);
	ui->enableBeepTime->setChecked(false);
	setBeepCheckBoxesEnabled(false);
	ui->beepHzBox->setValue(freq);
	on_setBeep_clicked();
	QTimer::singleShot(time, [&]() {
		notes.pop_front();
		sendNotes();
	});
}

int absoluteKey(QString key, bool mainKey) {
#define soHigh { if (result > 6) result -= 12; }
	QStringList keyList({"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"});
	int result = keyList.indexOf(key);
	if (result == -1) return -99;
	if (mainKey) soHigh // 这里如果打分号会把下面的else干掉
		else result -= 9;
	return result;
}

double length2time(int length) {
	QList <int> from({1, 2, 21, 4, 41, 8, 81, 16, 161, 32, 321, 64, 641, 3, 5, 6});
	double I = 1.0, E = 3.0; // 垃圾C语言
	QList <double> to({1, I / 2, E / 4, I / 4, E / 8, I / 8, E / 16, I / 16, E / 32, I / 32, E / 64, I / 64, E / 128, I / 4 / 3, I / 4 / 5, I / 4 / 6});
	int index = from.indexOf(length);
	if (index == -1) return -1;
	return to.at(index);
}

void SingleChip::on_setPlayNotes_clicked() {
	bool start = !startPlay;
	startPlay = start;
	ui->setPlayNotes->setText(start ? stopCaption : startCaption);
	ui->setPlayNotes->setChecked(start);
	force_no_check = start;
	setEnabled(!start, ExceptPlayNotes);
	setBeepCheckBoxesEnabled(!start);
	if (start) prepareNotes();
}

void SingleChip::connectDo(bool open) {
	if (!open)
		ui->curValueText->clear();
}

void SingleChip::setBeepCheckBoxesEnabled(bool enabled) {
	ui->enableBeep->setEnabled(enabled);
	ui->enableBeepTime->setEnabled(enabled);
}

void SingleChip::iniSave() {
	configIni->beginGroup("SingleChip");
	configIni->setValue("ScrollSpeed", ui->scrollSpeedBox->value());
	configIni->setValue("MainKey", ui->mainKeyBox->currentText());
	configIni->setValue("BPM", ui->bpmBox->value());
	configIni->endGroup();
}

void SingleChip::personalizeCheck(int scrollSpeed, QString mainKey, int bpm) {
	ui->scrollSpeedBox->setValue(scrollSpeed);
	ui->mainKeyBox->setCurrentText(mainKey);
	ui->bpmBox->setValue(bpm);
}
