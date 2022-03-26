#include "sqlconnect.h"
#include <QMessageBox>

QString SqlConnect::error(QString text) {
	qDebug(text.toUtf8());
	errorInfo = text;
	return text;
}

SqlConnect::SqlConnect(QString name, QString HostName, int Port, QString UserName, QString Password, QString DatabaseName) {
	connectName = name;
	hostName = HostName;
	port = Port;
	userName = UserName;
	password = Password;
	databaseName = DatabaseName;
	SQL_init();
}

void SqlConnect::SQL_init() {
//	if (QSqlDatabase::contains(connectName)) //判断testConnect连接是否存在并连接
//		database = QSqlDatabase::database(connectName);
//	else //未连接则新建数据库连接
		changeDatabase(hostName, port, userName, password, databaseName);
}

void SqlConnect::changeDatabase(QString HostName, int Port, QString UserName, QString Password, QString DatabaseName) {
	QSqlDatabase::removeDatabase(connectName);
	hostName = HostName;
	port = Port;
	userName = UserName;
	password = Password;
	databaseName = DatabaseName;
	database = QSqlDatabase::addDatabase("QMYSQL", connectName); //创建数据库连接，并为其命名testConnect
	database.setHostName(HostName); //连接数据库主机名，这里需要注意（若填的为”127.0.0.1“，出现不能连接，则改为localhost)
	database.setPort(Port); //连接数据库端口号，与设置一致
	database.setUserName(UserName); //数据库用户名，与设置一致
	database.setPassword(Password); //数据库密码，与设置一致
	bool ok = database.open();
	qDebug(QString("MySQL Connect %1!").arg(ok ? "Successful" : "Fail").toLatin1());
	if (!ok) error(database.lastError().text());
	else createDB();
}

//static const char databaseName[] = "entry",
//				 userDataSheetName[] = "user";
/* 注：稍后将上述俩数据作为可变参数 */

void SqlConnect::createDB() {
	// QString querystring;
	//创建数据库
	database = QSqlDatabase::database(connectName);
	database.exec(QString("CREATE DATABASE IF NOT EXISTS %1").arg(databaseName));
	if (database.lastError().isValid()) {
		error("Create Database Failed." + database.lastError().text());
		return;
	}
	//创建数据表
	database.setDatabaseName(databaseName);
	if (!database.open()) {
		error("Database Open Failed.");
		return;
	}
	database.exec(QString("CREATE TABLE IF NOT EXISTS %1.%2(\
ID int auto_increment primary key,\
Username varchar(30),\
Password varchar(30)\
);").arg(databaseName).arg(userDataSheetName)); //执行创建数据表语句。前面如果加缩进可能会报错。
	if (database.lastError().isValid()) {
		error("DataSheet Creat Failed: " + database.lastError().text());
		return;
	}
	//支持中文
	database.exec(QString("alter table %1 convert to character set utf8;").arg(userDataSheetName));
}

#define SQL_error {if (database.lastError().isValid()) { error(database.lastError().text()); return -1; }}

/*
 * 以下函数返回值定义：
 * -1 表示MySQL语句错误；
 * 0 表示操作成功；
 * 1 表示用户名不存在(checkPassword)或用户名已被占用(addNewUser)；
 * 2 表示密码错误。
*/

int SqlConnect::addNewUser(QString username, QString password) {
	const int exist = checkPassword(username, password);
	if (exist == 0 || exist == 2) return 1;
	else if (exist == -1) return -1;
	database.exec(QString("INSERT INTO user (Username, Password) VALUES ('%1','%2');")
				  .arg(username).arg(password));
	SQL_error;
	return 0;
}

int SqlConnect::checkPassword(QString username, QString password) {
	for (int i = 0; i < 2; i++) {
		QSqlQuery query(database);
		query.exec(QString("SELECT %3 FROM %2 WHERE Username = '%1';")
					.arg(username).arg(userDataSheetName).arg(i == 0 ? "COUNT(Password)" : "Password"));
		SQL_error;
		query.first();
		if (i == 1) return (query.value(0).toString() == password ? 0 : 2);
		else if (query.value(0).toInt() == 0) return 1;
	}
	return 250; //我打死也不相信程序会走到这一行
}
