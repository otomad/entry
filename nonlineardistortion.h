#ifndef NONLINEARDISTORTION_H
#define NONLINEARDISTORTION_H

#include <QWidget>
#include "qcustomplot.h"
#include "common.h"

namespace Ui {
	class NonlinearDistortion;
}

class NonlinearDistortion : public QWidget
{
		Q_OBJECT

	public:
		explicit NonlinearDistortion(QWidget *parent = nullptr);
		~NonlinearDistortion() override;
		enum CurrentPlot {
			CurrentInPlot,
			CurrentOutPlot
		} currentPlot = CurrentOutPlot;
		QCustomPlot *getCurrentPlot();

	private:
		Ui::NonlinearDistortion *ui;
		QCustomPlot *inPlot;
		QCustomPlot *outPlot;
		QCPGraph *inGraph;
		QCPGraph *outGraph;
		void exampleDistort(int num);
};

#endif // NONLINEARDISTORTION_H
