#include <QApplication>
#include <QDebug>
#include <QFile>
#include "loginwindow.h"
#include "common.h"

QSettings *configIni = nullptr; //指定ini文件
const int EXIT_CODE_REBOOT = -123456789; //退出后重启代码
int startMode = -1; //启动模式（仅可用什么组件）
#define forceStartMode -1 //强制启动模式，用于单独编译子程序
int getStartMode(QString a);
QString appPath; //应用路径
QApplication *app = nullptr; //应用程序
void showConsoleHelp();

int main(int argc, char *argv[]) {
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	// restart:
	QApplication a(argc, argv);
	if (argc > 1) {
		QString arg = argv[1]; // 0号参数是程序的地址，1号参数才是后接的第一个参数
		if (arg == "/?" || arg == "help") {
			showConsoleHelp();
			return 0;
		}
		int _startMode = getStartMode(argv[1]);
		if (_startMode >= 0) startMode = _startMode;
	}
	#if forceStartMode>=0
		startMode = forceStartMode;
	#endif
	app = &a;
	appPath = a.applicationDirPath();
	static QString ini_name = "config";
	configIni = new QSettings(appPath + "/" + ini_name + ".ini", QSettings::IniFormat);
	changeQss(configIni->value("Personalize/Skin", "QDarkStyle").toString());
	QTranslator qtTranslator;
	qtTranslator.load(":/qm/qt_zh_CN.qm");
	a.installTranslator(&qtTranslator);
	QTranslator qtBaseTranslator;
	qtBaseTranslator.load(":/qm/qtbase_zh_CN.qm");
	a.installTranslator(&qtBaseTranslator);
	LoginWindow loginWindow;
	loginWindow.show();
	int exit = a.exec();
	delete configIni;
	delete sql;
	// if (exit == EXIT_CODE_REBOOT) goto restart;
	return exit;
}

int getStartMode(QString a) {
	bool ok = false;
	int result = a.toInt(&ok);
	// QMessageBox::about(nullptr, "", QString("%1 %2").arg(arg).arg(result));
	return ok ? result : -1;
}

void showConsoleHelp() {
	char help[] = "不使用任何参数则正常启动上位机。\n\
\n\
参数：\n\
\n\
  [/? | help]  显示控制台的使用帮助。\n\
  任意数字 使打开的上位机仅包含所输入的编号的功能。若输入的数字编号不合法，则不起任何效果，这与不带任何参数时是一样的。\n\
\n\
  如参数为 0 时，表示仅打开包含“数控恒压源”的上位机。";
	qDebug("%s", help);
	QMessageBox::about(nullptr, "命令行使用说明", help);
}
