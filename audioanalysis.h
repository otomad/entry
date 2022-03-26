#ifndef AUDIOANALYSIS_H
#define AUDIOANALYSIS_H

#include <QWidget>
#include "qcustomplot.h"

namespace Ui {
	class AudioAnalysis;
}

class AudioAnalysis : public QWidget {
		Q_OBJECT

	public:
		explicit AudioAnalysis(QWidget *parent = nullptr);
		~AudioAnalysis() override;
		void showInfo(float freq2, float freq3, float power2, float power3,
					  float allPower, bool isDuration, float distort, int freq);
		void showInfo(QStringList infos);
		void readyRead(const QString str);
		void addPoint(float point);
		void addPoint(QList<float> points);
		void drawSpect(QList<float> data);
		void updatePlot();
		QCustomPlot *wavePlot;
		QCPGraph *waveGraph;
		QCustomPlot *spectPlot;
		QCPGraph *spectGraph;
		static const int SPECT_GAP = 20;
		static QList<float> strs2floats(QStringList strs);

	private slots:
		void on_clearBtn_clicked();
		void on_exportBtn_clicked();

	private:
		Ui::AudioAnalysis *ui;
		QString historyInfo = "";
};

#endif // AUDIOANALYSIS_H
