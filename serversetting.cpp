#include "serversetting.h"
#include "ui_serversetting.h"
#include <QButtonGroup>
#include <QMessageBox>
#include <QDesktopServices>
#include <QClipboard>
#include <QFormLayout>

#define showLinkInHtml false // 在HTML部分显示地址栏？（移除）

static QFont
	bold,
	normal;

static QStringList qssNames({
	"QDarkStyle",
	"QLightStyle",
	"ClassicVista",
	"None"
});

bool changeQss(QString name) {
	QStringList qssPathes({
		":/qdarkstyle/dark/darkstyle.qss",
		":/qlightstyle/light/lightstyle.qss",
		":/qss/stylesheet.qss",
		""
	});
	int index = qssNames.indexOf(name);
	if (index == -1) return false;
	QString path = qssPathes.at(index);
	if (path.length() != 0) {
		QFile qss(path); //加载样式表
		if (qss.open(QFile::ReadOnly)) {
			QString style = qss.readAll(); //QStringLiteral为什么会报宏错误
			app->setStyleSheet(style);
			qss.close();
			return true;
		} else {
			qDebug("StyleSheet Load Failed!");
			return false;
		}
	} else {
		app->setStyleSheet("* { font-family: \"Segoe UI\", \"Microsoft Yahei UI\"; }");
		return true;
	}
}

//static int windowHeight;

ServerSetting::ServerSetting(QWidget *parent, ParentName parentName) :
	QDialog(parent),
	ui(new Ui::ServerSetting) {
	ui->setupUi(this);
	/*setWindowFlags(Qt::Dialog
				 | Qt::MSWindowsFixedSizeDialogHint
				 | Qt::WindowTitleHint
				 | Qt::WindowCloseButtonHint
				 | Qt::CustomizeWindowHint
				 | Qt::WindowSystemMenuHint); //固定大小、禁止最大化、最小化*/
//	ui->buttonBox->button(QDialogButtonBox::Ok)->setText("确定(&O)");
//	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("取消(&C)");
	ui->PasswordInput->setEchoMode(QLineEdit::Password);
	ui->tabSetting->setCurrentIndex(0);
	showLedHtml();
	if (parentName == LoginWindow)
		while (ui->tabSetting->count() > 1)
			ui->tabSetting->removeTab(1);
	bold.setBold(true);
	normal.setBold(false);
	ui->skinList->addItems(qssNames);
	selectSkin = selectSkin_bak = configIni->value("Personalize/Skin", "QDarkStyle").toString();
	selectingSkin(selectSkin);
	#if !showLinkInHtml
		ui->urlBox->setVisible(false);
		ui->urlGoBtn->setVisible(false);
	#endif
//	windowHeight = this->height();
//	setFixedSize(this->width(), this->height());
//	connect(ui->tabSetting, &QTabWidget::tabBarClicked, this, &ServerSetting::resetWindowHeight);
	// 初始化消息记录的默认列宽
	if (startMode != -1) {
		ui->startSubAppBtn->setVisible(false);
		for (int i = 0; i < ui->tabSetting->count(); i++)
			if (ui->tabSetting->widget(i) == ui->tabLog) {
				ui->tabSetting->removeTab(i);
				return;
			}
	}
	int columnWidth[] = { 40, 86, 122, 140, 150, 150, 150 };
	for (int i = 0; i < ui->logTable->columnCount(); i++)
		ui->logTable->setColumnWidth(i, columnWidth[i]);
	//校验码
	QButtonGroup *origRadios = new QButtonGroup(this), *crcRadios = new QButtonGroup(this);
	origRadios->addButton(ui->examineOrigHex);
	origRadios->addButton(ui->examineOrigAscii);
	crcRadios->addButton(ui->examineCrcHex);
	crcRadios->addButton(ui->examineCrcBin);
	ui->examineOrigHex->setChecked(true);
	ui->examineCrcHex->setChecked(true);
	QClipboard *clipboard = QApplication::clipboard();
	connect(ui->examineOrigCopyBtn, &QPushButton::clicked, this, [&]() { clipboard->setText(ui->examineOrigText->toPlainText()); });
	connect(ui->examineCrcCopyBtn, &QPushButton::clicked, this, [&]() { clipboard->setText(ui->examineCrcText->text()); });
	connect(ui->examineOrigPasteBtn, &QPushButton::clicked, this, [&]() { ui->examineOrigText->setPlainText(clipboard->text()); });
	connect(ui->examineOrigHex, &QRadioButton::toggled, this, [&](bool checked) { if (checked) {
		ui->examineOrigText->setPlainText(ascii2hex(ui->examineOrigText->toPlainText()));
		ui->examineOrigText->setPlaceholderText("例：11 22 33 44 或 0x11 0x22 0x33 或 0x11,0x22,0x33");
	}});
	connect(ui->examineOrigAscii, &QRadioButton::toggled, this, [&](bool checked) { if (checked) {
		ui->examineOrigText->setPlainText(hex2ascii(ui->examineOrigText->toPlainText()));
		ui->examineOrigText->setPlaceholderText("");
	}});
	CrcsInit();
}

/* void ServerSetting::resetWindowHeight() {
	this->setFixedHeight(windowHeight);
	auto geometry = this->geometry();
	geometry.setHeight(windowHeight);
	this->setGeometry(geometry);
}

void ServerSetting::mouseMoveEvent(QMouseEvent *event) {
	QWidget::mouseMoveEvent(event);
	resetWindowHeight();
} */

ServerSetting::~ServerSetting() {
	delete ui;
}

void ServerSetting::givenValue() {
	ui->HostNameInput->setText(sql->hostName);
	ui->PortInput->setText(QString::number(sql->port));
	ui->UserNameInput->setText(sql->userName);
	ui->PasswordInput->setText(sql->password);
	ui->DatabaseNameInput->setText(sql->databaseName);
}

/*void ServerSetting::on_buttonBox_accepted() {
	QString HostName = ui->HostNameInput->text(),
			UserName = ui->UserNameInput->text(),
			Password = ui->PasswordInput->text(),
			DatabaseName = ui->DatabaseNameInput->text();
	int Port = ui->PortInput->text().toInt();
	sql->changeDatabase(HostName,Port,UserName,Password,DatabaseName);
	iniSave(HostName,Port,UserName,Password,DatabaseName);
	this->close();
}

void ServerSetting::on_buttonBox_rejected() {
	this->close();
}*/

void ServerSetting::iniSave(QString HostName, int Port, QString UserName, QString Password, QString DatabaseName) {
	configIni->beginGroup("MySQL");
	configIni->setValue("HostName", HostName);
	configIni->setValue("Port", Port);
	configIni->setValue("UserName", UserName);
	configIni->setValue("Password", Password);
	configIni->setValue("DatabaseName", DatabaseName);
	configIni->endGroup();
}

void ServerSetting::on_exitBtn_clicked() {
	QApplication::exit();
}

void ServerSetting::on_logoutBtn_clicked() {
//	QApplication::exit(EXIT_CODE_REBOOT);
	QString program = QApplication::applicationFilePath();
	QStringList arguments = QApplication::arguments();
	QString workingDirectory = QDir::currentPath();
	QProcess::startDetached(program, arguments, workingDirectory);
	QApplication::exit();
}

void ServerSetting::on_startSubAppBtn_clicked() {
	QDialog dialog(this);
	QFormLayout form(&dialog);
	dialog.setWindowTitle("启动子程序");
	dialog.setWindowFlags(DIALOG_WINDOW_NORESIZE);
	form.setRowWrapPolicy(QFormLayout::DontWrapRows);
	form.setVerticalSpacing(9);
	form.addRow(new QLabel(
		"选择一个子程序，然后关闭本应用程序，启动子程序版本的上位机。\n"
		"子程序只有选中的功能，不附带其它功能。也许可以节约性能。\n"
		"进入子程序之后，不可轻易退出子程序模式。"
	, &dialog));
	QComboBox subBox(&dialog);
	subBox.addItems(*tabNameList);
	form.addRow(QString("选择子程序"), &subBox);
	// 确定取消
	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
	form.addRow(&buttonBox);
	connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
	connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
	// 点击确定
	if (dialog.exec() == QDialog::Accepted) {
		int subIndex = subBox.currentIndex();
		QString program = QApplication::applicationFilePath();
		QStringList arguments({ QString::number(subIndex) });
		QString workingDirectory = QDir::currentPath();
		QProcess::startDetached(program, arguments, workingDirectory);
		QApplication::exit();
	}
}

void ServerSetting::on_resetIniBtn_clicked() {
	QMessageBox::StandardButton ensure = QMessageBox::warning(this, "警告", "将会重置您的所有个人偏好配置信息。\n仅在程序出现异常时尝试该功能可能解决问题。", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (ensure == QMessageBox::No) return;
	configIni->clear();
	QMessageBox::information(this, "清除成功", "已经成功清除个人配置数据，重新启动本应用程序使更改生效。\n您也可以点击“退出登录”按钮重启程序。");
}


void ServerSetting::on_buttonBox_clicked(QAbstractButton *button) {
	#define Button(name) ui->buttonBox->button(QDialogButtonBox::name)
	#define isButton(name) Button(name) == button
	if (isButton(Cancel)) {
		selectingSkin(selectSkin_bak);
		this->close();
	} else if (isButton(Apply)) {
		QString HostName = ui->HostNameInput->text(),
				UserName = ui->UserNameInput->text(),
				Password = ui->PasswordInput->text(),
				DatabaseName = ui->DatabaseNameInput->text();
		int Port = ui->PortInput->text().toInt();
		sql->changeDatabase(HostName,Port,UserName,Password,DatabaseName);
		iniSave(HostName,Port,UserName,Password,DatabaseName);
		if (selectSkin != selectSkin_bak) {
			selectSkin_bak = selectSkin;
			configIni->setValue("Personalize/Skin", selectSkin);
			changeQss(selectSkin);
		}
	} else if (isButton(Ok)) {
		on_buttonBox_clicked(Button(Apply));
		on_buttonBox_clicked(Button(Cancel));
	}
	#undef Button
	#undef isButton
}

void ServerSetting::showLedHtml() {
	QString url_str = "qrc:/led/html/index.html";
	ui->htmlLayout->addWidget(webview);
	webview->load(QUrl(url_str));
}

void ServerSetting::on_skinList_itemDoubleClicked(QListWidgetItem *item) {
	selectingSkin(item);
}

void ServerSetting::selectingSkin(QListWidgetItem *item) {
	for (int i = 0; i < ui->skinList->count(); i++)
		ui->skinList->item(i)->setFont(normal);
	item->setFont(bold);
	selectSkin = item->text();
	on_skinList_itemClicked(item);
}

void ServerSetting::selectingSkin(QString name) {
	QList <QListWidgetItem *> items = ui->skinList->findItems(name, Qt::MatchFixedString); // Don't call QList::operator[]() on temporary [clazy-detaching-temporary]
	QListWidgetItem *item = items[0];
	selectingSkin(item);
}

void ServerSetting::on_skinList_itemClicked(QListWidgetItem *item) {
	const QString name = item->text();
	QString imgPath = QString(":/qss/preview/%1.png").arg(name);
	ui->skinPreview->setPixmap(QPixmap(imgPath));
}

void ServerSetting::on_urlGoBtn_clicked() {
	webview->load(QUrl(ui->urlBox->text()));
}

void ServerSetting::on_aboutLbl_linkActivated(const QString &link) {
	QDesktopServices::openUrl(QUrl(link));
}

void ServerSetting::on_aboutQtBtn_clicked() {
	QMessageBox::aboutQt(this, "关于 QT");
}

QString QHex::toString(int byte, bool show0x, bool upperData, bool upper0x) const {
	QString result = QString("%1").arg(num, byte * 2, 16, QChar('0'));
	if (upperData) result = result.toUpper();
	if (show0x) {
		QString prefix = upper0x ? "0X" : "0x";
		result = prefix + result;
	}
	return result;
}

void ServerSetting::CrcsInit() {
	Crcs.append({
		getCrc,
		"CRC-16/XMODEM",
		16,
		"x16+x12+x5+1",
		0x1021,
		0x0000,
		false,
		false,
		0x0000,
		false,
		"CRC-16/ZMODEM,\nCRC-16/ACORN",
		"",
		"高字节在前，低字节在后"
	});
	foreach(auto crc, Crcs) {
		ui->examineMannerBox->addItem(QString("%1 %2 %3").arg(crc.name, crc.expression, crc.details));
	}
	on_examineMannerBox_currentTextChanged(ui->examineMannerBox->currentText());
}

QString _bool2Str(bool condition) {
	return condition ? "True" : "False";
}

const ServerSetting::CrcInfo *ServerSetting::getCrcInfo(const QString comboText){
	const QString name = comboText.split(" ").at(0);
	for (int i = 0; i < Crcs.count(); i++) {
		const CrcInfo *crc = &Crcs.at(i);
		if (crc->name == name)
			return crc;
	}
	return nullptr;
}

void ServerSetting::on_examineMannerBox_currentTextChanged(const QString &str) {
	auto crc = getCrcInfo(str);
	if (crc == nullptr) return;
	QString info = QString("\
Name: %1\n\
Width: %2\n\
Expression: %3\n\
Poly: %4\n\
Init: %5\n\
RefIn: %6\n\
RefOut: %7\n\
XorOut: %8\n\
Order: %9\n").arg(
		crc->name,
		QString::number(crc->width),
		crc->expression,
		crc->poly.toString(2, true),
		crc->init.toString(2, true),
		_bool2Str(crc->refIn),
		_bool2Str(crc->refOut),
		crc->xorOut.toString(2, true),
		_bool2Str(crc->order)
	);
	if (crc->alias.length()) info += QString("Alias: %1\n").arg(crc->alias);
	if (crc->use.length()) info += QString("Use: %1\n").arg(crc->use);
	if (crc->details.length()) info += QString("Details: %1\n").arg(crc->details);
	ui->examineCrcInfo->setText(info);
}

QString ascii2hex(QString ascii) {
	QStringList hex;
	for (int i = 0; i < ascii.length(); i++)
		hex.append(QString("%1").arg(ascii.at(i).toLatin1(), 2, 16, QChar('0')).toUpper());
	return hex.join(' ');
}

QString hex2ascii(QString hex) {
	hex = hex.toUpper().remove("0X", Qt::CaseInsensitive).remove(QRegExp("[^0-9A-Fa-f]"));
	if (hex.length() % 2) hex = "0" + hex;
	QString ascii = "";
	for (int i = 0; i < hex.length(); i += 2) {
		bool ok;
		ushort h = hex.mid(i, 2).toUShort(&ok, 16);
		if (!ok) qDebug("Not a Hex String!");
		QChar c(h);
		ascii += c;
	}
	return ascii;
}

QString beautifyHex(QString hex) {
	hex = hex.toUpper().remove("0X", Qt::CaseInsensitive).remove(QRegExp("[^0-9A-Fa-f]"));
	if (hex.length() % 2) hex = "0" + hex;
	QStringList hex_arr;
	for (int i = 0; i < hex.length(); i += 2)
		hex_arr.append(hex.mid(i, 2));
	return hex_arr.join(' ');
}

void ServerSetting::on_examineCalculateBtn_clicked() {
	auto crc = getCrcInfo(ui->examineMannerBox->currentText());
	QString data = ui->examineOrigText->toPlainText();
	if (crc == nullptr) return;
	if (ui->examineOrigAscii->isChecked()) data = ascii2hex(data);
	data = beautifyHex(data);
	if (ui->examineOrigHex->isChecked()) ui->examineOrigText->setPlainText(data);
	QString result = crc->formula(data).toUpper();
	if (ui->examineCrcBin->isChecked()) result = QString("%1").arg(result.toInt(nullptr, 16), 16, 2, QChar('0'));
	ui->examineCrcText->setText(result);
}

void ServerSetting::addMsgLog(bool isSend, ConnectWay_t connectWay, QString clientName, QString utf8, QString gbk, QByteArray hex) {
	ui->logTable->insertRow(0);
	QStringList msgLog({
		isSend ? "发送" : "接收",
		connectWay == ConnectWay::SERIAL ? "串口" :
			connectWay == ConnectWay::LAN_CLIENT ? "局域网客户端" :
			connectWay == ConnectWay::LAN_SERVER ? "局域网服务端" :
			/* 未知 */ "寂寞",
		clientName,
		QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss.zzz"),
		utf8,
		gbk,
		hexstr(hex, true)
	});
	for (int i = 0; i < msgLog.count(); i++) {
		QString item = msgLog.at(i);
		QTableWidgetItem *newItem = new QTableWidgetItem(item);
		newItem->setToolTip(item);
		if (isSend) newItem->setBackground(QColor(128, 128, 128, 100));
		ui->logTable->setItem(0, i, newItem);
	}
}

void ServerSetting::on_clearLogBtn_clicked() {
	QTableWidget *table = ui->logTable;
//	QTableWidgetItem *i = table->item(0, 0);
	while (table->rowCount()) {
		for (int i = 0; i < table->columnCount(); i++)
			delete table->item(0, i);
		table->removeRow(0);
	}
	/*try {
		qDebug() << i->text();
	}  catch (char *e) {Q_UNUSED(e);}*/
}

void ServerSetting::on_getHelpBtn_clicked() {
//	QString helpFileName = QCoreApplication::applicationName() + "_408es_help.chm";
	QString helpFileName = "entry_480es_help.chm";
	QString tmpName = ":/help/entry_408es_help.chm"; // 为什么用字符串拼接就会报错？
	QString tmpNativeName = QDir::tempPath() + '/' + helpFileName;
//	QFile file(tmpName);
	/*QTemporaryFile *tmpFile = QTemporaryFile::createNativeFile(file);
	tmpFile->setAutoRemove(false); // 阻止自动清除文件
	qDebug() << "tempPath: " << QDir::tempPath();
	if (tmpFile->open()) {
		qDebug() << "fileTemplate: " << tmpFile->fileTemplate();
		qDebug() << "fileName: " << tmpFile->fileName();
		QProcess::execute("hh " + tmpFile->fileName());
	}*/
	/*if (file.open(QFile::ReadOnly)) {
		qDebug() << "tempPath: " << QDir::tempPath();
		qDebug() << "file: " << file.fileName();
		qDebug() << "copy" << file.copy(tmpNativeName);
		QProcess::execute("hh " + tmpNativeName);
	} else qDebug() << file.errorString();*/
	QFile file;
	qDebug() << "copy" << file.copy(tmpName, tmpNativeName);
	qDebug() << file.errorString();
	QProcess::startDetached("hh " + tmpNativeName);
}
