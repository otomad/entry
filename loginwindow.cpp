#include "loginwindow.h"
#include "ui_loginwindow.h"

#define loginCheck 0 // 开启登录验证？（移除）
SqlConnect *sql = nullptr;
const Qt::WindowFlags DIALOG_WINDOW_NORESIZE =
	  Qt::Dialog
	| Qt::MSWindowsFixedSizeDialogHint
	| Qt::WindowTitleHint
	| Qt::WindowCloseButtonHint
	| Qt::CustomizeWindowHint
	| Qt::WindowSystemMenuHint;

LoginWindow::LoginWindow(QWidget *parent):
    QMainWindow(parent),
	ui(new Ui::LoginWindow) {
    ui->setupUi(this);
	setWindowFlags(DIALOG_WINDOW_NORESIZE); //固定大小、禁止最大化、最小化
	setWindowIcon(QIcon(":/icon/hhtc.ico")); //改变图标（汤贱帮）
	{
		QList <QLineEdit *> list({ui->passwordInput, ui->ensurePasswordInput});
		foreach (QLineEdit* el, list)
			el->setEchoMode(QLineEdit::Password);
	}
	QRegExpValidator valid(QRegExp("[A-Za-z0-9]{1,20}"), this); //只允许输入数字与大小写字母，且不能超过20字符
	{
		QList <QLineEdit *> list({ui->usernameInput, ui->passwordInput, ui->ensurePasswordInput});
		foreach (QLineEdit* el, list)
			el->setValidator(&valid);
	}
	configIni->beginGroup("MySQL");
	sql = new SqlConnect(
							"testConnect",
							configIni->value("HostName", "127.0.0.1").toString(),
							configIni->value("Port", 3306).toInt(),
							configIni->value("UserName", "root").toString(),
							configIni->value("Password", "123456").toString(),
							configIni->value("DatabaseName", "entry").toString()
						);
	configIni->endGroup();
	ui->usernameInput->setText(configIni->value("Login/UserName", "").toString());
	ui->passwordInput->setText(configIni->value("Login/Password", "").toString());
	if (!ui->usernameInput->text().isEmpty()) ui->rememberPassword->setChecked(true);
	ensurePassword_init();

	#if !loginCheck
		QTimer::singleShot(500, this, &LoginWindow::login);
	#endif
}

LoginWindow::~LoginWindow() {
//	delete sql; //???
	delete mainWindow;
	delete ui;
}

void LoginWindow::on_ExitBtn_clicked() {
	QApplication::exit();
}

void LoginWindow::on_ToRegister_clicked() {
	isReginster = !isReginster;
	QString title[] = {"登录","注册"},
			changeCaption[] = {"我已有账号？登录","我没有账号？注册"};
	ui->LoginTitle->setText(title[isReginster]);
	ui->LoginBtn->setText(title[isReginster]);
	ensurePassword_init();
	ui->ToRegister->setText(changeCaption[!isReginster]);
}

void LoginWindow::ensurePassword_init() {
	ui->ensurePasswordLabel->setVisible(isReginster);
	ui->ensurePasswordInput->setVisible(isReginster);
	ui->rememberPassword->setVisible(!isReginster);
}

//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wdisabled-macro-expansion" //消除禁止宏扩展的警告

#define Critical(x) {QMessageBox::critical(this, "警告", x); return;}
#define Information(x) {QMessageBox::information(this, "提示", x);}
#define SQL_error(x) {if (x == -1) {QMessageBox::critical(this, "MySQL 错误", sql->errorInfo); return; }}

void LoginWindow::on_LoginBtn_clicked() {
	#if loginCheck // if (/* DISABLES CODE */ (0)) {
		const QString username = ui->usernameInput->text(),
					  password = ui->passwordInput->text();
		if (username == "") Critical("请输入用户名！");
		if (password == "") Critical("请输入密码！");
		if (isReginster) {
			if (ui->passwordInput->text() != ui->ensurePasswordInput->text()) Critical("两次输入的密码不相同！");
			int result = sql->addNewUser(username, password);
			SQL_error(result);
			if (result == 1) Critical("该用户名已被占用，请尝试其它用户名！");
			Information("注册成功！");
			on_ToRegister_clicked();
			return;
		}
		int result = sql->checkPassword(username, password);
		SQL_error(result);
		if (result == 1) Critical("用户名不存在！");
		if (result == 2) Critical("密码错误！");
	#endif // }
	Information("登录成功！")
	if (ui->rememberPassword->isChecked()) {
		configIni->setValue("Login/UserName", ui->usernameInput->text());
		configIni->setValue("Login/Password", ui->passwordInput->text());
	} else configIni->remove("Login");
	login();
}

void LoginWindow::login() {
	this->hide();
	mainWindow->show();
}

//#pragma clang diagnostic pop

void LoginWindow::keyPressEvent(QKeyEvent *event) { //键盘按回车也可以登录
	switch (event->key()) {
		case Qt::Key_Return:
			on_LoginBtn_clicked();
			break;
		default:
			return QWidget::keyPressEvent(event);
	}
}

void LoginWindow::on_ServerSetting_clicked() {
//	setting->setAttribute(Qt::WA_DeleteOnClose);
//	setting->givenValue();
//	setting->show();
//	setting->exec();
	setting->givenValue();
	setting->exec();
}


