#ifndef SIGNALGENERATOR_H
#define SIGNALGENERATOR_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
//#include <QSignalMapper>
#include "common.h"

namespace Ui {
	class SignalGenerator;
}

class SignalGenerator : public QWidget {
		Q_OBJECT

	public:
		explicit SignalGenerator(QWidget *parent = nullptr);
		~SignalGenerator() override;
		int readyRead(const QByteArray buffer);
		void setEnabled(bool enabled);
		void iniSave();
		void personalizeCheck(int freqStep, double amp, int freq, int waveform, int lwrFreq, int UprFreq, int ScanFreqStep, int ScanPeriod, bool scanWay);
		struct ErrorStrings {
			QString command;
			QString expectCheck;
			QString check;
		};
		struct ResultList {
			QString command;
			int result;
		};
		void connectDo(bool open);
		class Modified {
			public:
				bool freqStep = false;
				bool amp = false;
				bool freq = false;
				bool waveform = false;
				void reset();
		};

	private slots:
		void modify(int command);
		void on_setValueBtn_clicked();

	private:
		Ui::SignalGenerator *ui;
//		QSerialPort *serial;
		void send(QByteArray hex);
		void send(QString hexStr);
		void sendCommand(int command);
		QString success;
//		Modified *modified = new Modified;
//		QVector <bool> modified;
		bool modified[4];
//		QSignalMapper *spinBoxMapper;
		void resetModified(bool allTrue);
		bool startRead = false;
//		QByteArray receive;
//		QList <QString> errorList;
		QList <ErrorStrings> errorList;
		QList <ResultList> resultList;
		void btnDo(int command = 0);
		int readResult = -1;
		QList <int> sendQueue;
		void startSend();
		int Command = 0;
		bool enableModify = false;
		static QStringList commandName;
};

#endif // SIGNALGENERATOR_H
