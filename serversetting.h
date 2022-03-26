#ifndef SERVERSETTING_H
#define SERVERSETTING_H

#include <QDialog>
#include <QPushButton>
#include <QListWidgetItem>
#include <QtWebKitWidgets/QWebView>
#include "sqlconnect.h"
#include "common.h"
#include "qhex.h"

namespace Ui {
	class ServerSetting;
}

class ServerSetting : public QDialog {
		Q_OBJECT

	public:
		enum ParentName { LoginWindow, MainWindow };
		explicit ServerSetting(QWidget *parent = nullptr, ParentName parentName = LoginWindow);
		~ServerSetting() override;
		void givenValue();
		void showLedHtml();
		QWebView *webview = new QWebView(this);
		void addMsgLog(bool isSend, ConnectWay_t connectWay, QString clientName, QString utf8, QString gbk, QByteArray hex);
		QStringList *tabNameList;

	protected:
//		void mouseMoveEvent(QMouseEvent *event) override;

	private slots:
//		void on_buttonBox_accepted();
//		void on_buttonBox_rejected();
		void on_exitBtn_clicked();
		void on_logoutBtn_clicked();
		void on_resetIniBtn_clicked();
		void on_buttonBox_clicked(QAbstractButton *button);
		void on_skinList_itemDoubleClicked(QListWidgetItem *item);
		void on_skinList_itemClicked(QListWidgetItem *item);
		void on_urlGoBtn_clicked();
		void on_aboutLbl_linkActivated(const QString &link);
		void on_aboutQtBtn_clicked();
		void on_examineMannerBox_currentTextChanged(const QString &str);
		void on_examineCalculateBtn_clicked();
//		void resetWindowHeight();
		void on_clearLogBtn_clicked();
		void on_startSubAppBtn_clicked();
		void on_getHelpBtn_clicked();

	private:
		Ui::ServerSetting *ui;
		void iniSave(QString HostName, int Port, QString UserName, QString Password, QString DatabaseName);
		void iniRead();
		QString selectSkin_bak;
		QString selectSkin;
		void selectingSkin(QListWidgetItem *item);
		void selectingSkin(QString item);
		struct CrcInfo {
			QString (&formula)(QString);
			QString name;
			int width;
			QString expression;
			QHex poly;
			QHex init;
			bool refIn;
			bool refOut;
			QHex xorOut;
			bool order;
			QString alias;
			QString use;
			QString details;
			CrcInfo(
					QString (&Formula)(QString),
					QString Name,
					int Width,
					QString Expression,
					uint Poly,
					uint Init,
					bool RefIn,
					bool RefOut,
					uint XorOut,
					bool Order = false,
					QString Alias = "",
					QString Use = "",
					QString Details = ""
			) :
				formula(Formula),
				name(Name),
				width(Width),
				expression(Expression),
				poly(Poly),
				init(Init),
				refIn(RefIn),
				refOut(RefOut),
				xorOut(XorOut),
				order(Order),
				alias(Alias),
				use(Use),
				details(Details) {}
		};
		QList <CrcInfo> Crcs;
		void CrcsInit();
		const CrcInfo *getCrcInfo(const QString comboText);
};

#endif // SERVERSETTING_H
