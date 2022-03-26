#ifndef SINGLECHIP_H
#define SINGLECHIP_H

#include <QWidget>
#include "common.h"

namespace Ui {
	class SingleChip;
}

class SingleChip : public QWidget {
		Q_OBJECT

	public:
		explicit SingleChip(QWidget *parent = nullptr);
		~SingleChip() override;
		int readyRead(const QByteArray buffer);
		struct NOTE {
			QString key;
			int eight;
			int length;
			QString notesText;
		};
		enum SetEnabledExcept { ExceptNone, ExceptScrollLed, ExceptPlayNotes };
		void setEnabled(bool enabled, SetEnabledExcept except = ExceptNone);
		void connectDo(bool open);
		void setBeepCheckBoxesEnabled(bool enabled);
		QTimer *checkTimer = nullptr;
		void iniSave();
		void personalizeCheck(int scrollSpeed, QString mainKey, int bpm);

	private slots:
		void on_setDigit_clicked();
		void on_setBeep_clicked();
		void on_setScrollLed_clicked();
		void on_setPlayNotes_clicked();
		void checkTimer_timeout();

	private:
		Ui::SingleChip *ui;
		void on_setLight_clicked();
		void sendCommand(int command, QString data = "", bool needCheck = true);
		bool startRead = false;
		int readResult = -1;
		QString success;
		QString led(int num, bool dot = false);
		QString led(char c, bool dot = false);
		QString scrollLedText;
		QList <bool> scrollLedDots;
		void scrollLed();
		void scrollLed_ready();
		void showLedByHex(QString text, QList <bool> dots);
		bool startScroll = false;
		QList <NOTE> notes;
		void prepareNotes();
		void sendNotes();
		bool startPlay = false;
		bool force_no_check = false;
		// bool startCheckTimer = false;
};

int absoluteKey(QString key, bool mainKey = false);
double length2time(int length);

#endif // SINGLECHIP_H
