#ifndef SQLCONNECT_H
#define SQLCONNECT_H

#include <QtSql>
#include <QSqlDatabase>

class SqlConnect {
	public:
		SqlConnect(QString name, QString HostName, int Port, QString UserName, QString Password, QString DatabaseName);
		QSqlDatabase database;
		QString connectName;
		QString errorInfo = "";
		int addNewUser(QString username, QString password);
		int checkPassword(QString username, QString password);
		QString hostName;
		int port;
		QString userName;
		QString password;
		QString databaseName;
		QString userDataSheetName = "user";
		void changeDatabase(QString HostName, int Port, QString UserName, QString Password, QString DatabaseName);
	private:
		void SQL_init();
		void createDB();
		QString error(QString text);
};

#endif // SQLCONNECT_H
