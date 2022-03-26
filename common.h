/* 这里放置一些比较常用的内容，比如全局变量，以及需要广泛使用的函数。 */

#ifndef COMMON_H
#define COMMON_H

#include <QSettings>
#include <QTextEdit>
#include "sqlconnect.h"

#define startCode "55AA"
#define endCode "AA55"

extern QSettings *configIni; // ini 配置设置
extern const int EXIT_CODE_REBOOT; // 重启代码
extern int startMode; // 启动模式
extern SqlConnect *sql; // MySQL 数据库
extern QString appPath; // 应用路径
extern QApplication *app; // 主应用程序
extern const Qt::WindowFlags DIALOG_WINDOW_NORESIZE; // 设定窗口不可更改大小的状态

// 枚举类型
struct ConnectWay {
	enum type {
		CONNECT_OFF = -1,
		SERIAL,
		LAN_CLIENT,
		LAN_SERVER
	};
};
typedef ConnectWay::type ConnectWay_t;

struct Encode {
	enum type {
		UTF8,
		GBK,
		HEX
	};
};
typedef Encode::type Encode_t;

// 颜色和字体
extern struct _MyColor {
	const QColor
		CYAN,
		BLUE,
		RED,
		ORANGE,
		WHITE,
		BLACK,
		GREEN;
} myColor;

extern struct _MyFont {
	const QFont
		font_,
		font_en,
		font_bold,
		font_italic;
} myFont;

// 常用函数
double QMax(QVector<double> data);
double QMin(QVector<double> data);
char ConvertHexChar(char ch);
QByteArray QString2Hex(QString str);
void examineHex(QTextEdit *edit);
QString avoidOdd(QString str);
QString sendNewLineCheck(QTextEdit *txtBox, bool check = false);

uint CrcCheckSum(uchar *puchMsg, uint usDataLen);
uint CrcCheckSum(QString hex);
QString int2hexQs(int num, int byte = 2);
QString getCrc(QString data);
QString ascii2hex(QString ascii);
QString hex2ascii(QString hex);
QString beautifyHex(QString hex);
QString hexstr(const QByteArray buffer, bool beautify = false);

bool changeQss(QString name);

#endif // COMMON_H
