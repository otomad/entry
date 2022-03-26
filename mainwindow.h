#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort> //提供访问串口的功能
#include <QSerialPortInfo> //提供系统中存在的串口的信息
#include <QTcpServer> //监听套接字（服务端）
#include <QTcpSocket> //通信套接字（客户端）
#include "qcustomplot.h"
#include "common.h"
#include "serialportassistant.h"
#include "signalgenerator.h"
#include "singlechip.h"
#include "sqlconnect.h"
#include "serversetting.h"
#include "serialportchat.h"
#include "nonlineardistortion.h"
#include "oscilloscope.h"
#include "socketlistmanager.h"
#include "audioanalysis.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
		Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = nullptr);
		~MainWindow() override;
		QSerialPort *serial = new QSerialPort(this);
		QTcpSocket *tcpSocket;
		QTcpSocket *tcpSocket_Client = new QTcpSocket(this);
		QTcpServer *tcpServer = new QTcpServer(this);
		QList <QTcpSocket *> socketList;
		void showStatus(QString text, int ms = 0);
		void clearStatus();
		void send(const QByteArray data);
		void send(const QString text, Encode_t encode = Encode::UTF8);
		ConnectWay_t way;
		void setEnabled(bool enabled); // 控件的失能与使能
		void connectDo(bool not_endline = false);
		void setTitle(QString title = "已断开");
		int isConnect();
		struct ToolName {
			static QString wave,
						   serial,
						   serial_legacy,
						   signal,
						   single,
						   distort,
						   scope,
						   audio;
		};
		class AssistantConfig {
			public:
				inline Encode_t sendEncode() { return _sendEncode; }
				inline Encode_t recvEncode() { return _recvEncode; }
				inline bool sendNewLine() { return _sendNewLine; }
				inline bool recvNewLine() { return _recvNewLine; }
				inline bool sendAutoClr() { return _sendAutoClr; }

				inline void setEncode(Encode_t sendEncode, Encode_t recvEncode) { set((int)sendEncode, (int)recvEncode); }
				inline void setSendEncode(Encode_t sendEncode) { set((int)sendEncode); }
				inline void setRecvEncode(Encode_t recvEncode) { set(-1, (int)recvEncode); }
				inline void setSendNewLine(bool sendNewLine, bool recvNewLine) { set(-1, -1, sendNewLine, recvNewLine); }
				inline void setSendNewLine(bool sendNewLine) { set(-1, -1, sendNewLine); }
				inline void setRecvNewLine(bool recvNewLine) { set(-1, -1, -1, recvNewLine); }
				inline void setSendAutoClr(bool sendAutoClr) { set(-1, -1, -1, -1, sendAutoClr); }

			private:
				Encode_t _sendEncode = Encode::UTF8;
				Encode_t _recvEncode = Encode::UTF8;
				bool _sendNewLine = false;
				bool _recvNewLine = false;
				bool _sendAutoClr = true;

				bool isSetting = false; // 避免递归调用
				void set(
					int sendEncode = -1,
					int recvEncode = -1,
					int sendNewLine = -1,
					int recvNewLine = -1,
					int sendAutoClr = -1
				);
				MainWindow *parent;
				friend class MainWindow;
		} asstConf;
		QString getCurrentTabText();
		void resizePlot(const int mode, QCustomPlot *plot);
		void setSelectChtLineStyle(int sceneIndex, QCustomPlot *plot);
		void setCurPlotItemText(QString text);
		inline QString socket2str(QTcpSocket *socket);
		void disconnectedAClient(QString clientName);
		QStringList tabNameList;

	protected:
		// 重写事件
		void keyPressEvent(QKeyEvent *ev) override;
		void keyReleaseEvent(QKeyEvent *ev) override;
		bool eventFilter(QObject *obj, QEvent *event) override;

	private slots:
		void selectionChanged();
		void QTcpSocket_connected();
		void QTcpServer_connected();
		void QTcpSocket_readyRead();
		void QTcpSocket_disconnected();

		void on_actionExit_triggered();
		void on_checkSerial_clicked();
		void on_openSerial_clicked();
		void on_tabWidget_currentChanged(int index);
		void on_actionSetting_triggered();
		void on_showNowBtn_clicked();
		void on_setVA_clicked();
		void on_tabWidget_tabBarDoubleClicked(int index);

	private:
		Ui::MainWindow *ui;

		// 图表处理
		void chart();
		void square();
		void smooth();
		QCustomPlot *plot;
		QCPGraph *graphI; //电流
		QCPGraph *graphU; //电压
		QList <QCPAxis *> axis_x, axis_y, axis_xy;
		void plot_init();
		class CurrentPlotItem {
			public:
				QToolButton *tb;
				QMenu *currentPlotMenu;
				QActionGroup *currentPlotGroup;
				QAction *vaPlot,
						*inPlot,
						*outPlot;
				static QString currentCaptionVa,
							   currentCaptionIn,
							   currentCaptionOut,
							   currentCaptionNA;
		} *curPlotItem = new CurrentPlotItem;

		// 串口处理
		void readyRead(QByteArray buffer, ConnectWay_t way, QString clientName = "");
		ConnectWay_t openingWay = ConnectWay::CONNECT_OFF;
		ConnectWay_t getConnectWay();
		inline void connect_tcpSocket();
		void setTitle_openingServer(int num);
		void updateSocketList();
		bool serverAutoIpFlag = false;

		// 配置设置
		void iniSave();
		void iniRead();
		int getTabWidgetByText(QString text);
		bool isIniRead = false;

		// 子对话框
		void toggleDialog(QDialog *dialog);
		ServerSetting *setting = new ServerSetting(this, ServerSetting::MainWindow);
		SocketListManager *socketListMgr = new SocketListManager(this);
		QMainWindow *popWindow = new QMainWindow(nullptr);
		QTabWidget *popWindowTab = new QTabWidget(this);
		void backToTabwidget(QObject *obj);

		// Widget 组件
		SerialPortChat *tabChat = new SerialPortChat(this);
		SerialPortAssistant *tabSerial = new SerialPortAssistant(this);
		SignalGenerator *tabSignal = new SignalGenerator(this);
		SingleChip *tabSingle = new SingleChip(this);
		NonlinearDistortion *tabDistort = new NonlinearDistortion(this);
		Oscilloscope *tabScope = new Oscilloscope(this);
		AudioAnalysis *tabAudio = new AudioAnalysis(this);
};

#endif // MAINWINDOW_H
