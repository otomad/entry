#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <QWidget>
#include "common.h"
#include "qcustomplot.h"

namespace Ui {
	class Oscilloscope;
}

class Oscilloscope : public QWidget {
		Q_OBJECT

	public:
		explicit Oscilloscope(QWidget *parent = nullptr);
		~Oscilloscope() override;
		class Plot;
		QList <Plot *> plots;
		Plot *plots_at(uint i);
		uint curPlotIndex;
		Plot *curPlot();
		bool addPoint(uint chart, uint legend, int value, bool refreshRightNow = true);
		QCustomPlot *getCurrentPlot(); // 给 MainWindow 使用的
		QString getCurrentPlotName();
		int readyRead(const QString code);

	private slots:
		void on_addPlotBtn_clicked();
		void on_delPlotBtn_clicked();
		void on_renPlotBtn_clicked();
//		void on_plotsListWidget_itemChanged(QListWidgetItem *item);
		void on_plotsListWidget_itemActivated(QListWidgetItem *item);
		void on_manualAddDataBtn_clicked();
		void on_clrPlotBtn_clicked();

	private:
		Ui::Oscilloscope *ui;
		void plotsListWidgetUpdate();
};

class Oscilloscope::Plot : public QObject, public QListWidgetItem {
		Q_OBJECT

	public:
		/**
		 * @param name - 示波器的名称，将会显示在列表框中，后期可以修改。
		 * @param index - 当前示波器在 Oscilloscope::plots 的序号。
		 * @param container - 列表框容器。
		 * @param layoutPosition - 图表展示的容器。
		 * @param parent - 父对象。
		 */
		explicit Plot(QString name, uint index, QListWidget *container, QWidget *layoutPosition, Oscilloscope *parent = nullptr);
		~Plot() override;
		uint index;
		QCustomPlot *plot;
		void addPoint(uint legend, int value, bool refreshRightNow = true);
		bool changeIndex(uint index);

	private slots:
		void selectionChanged();

	private:
		Oscilloscope *parent;
		const static char listWidgetItem[];

	friend class Oscilloscope;
};

#endif // OSCILLOSCOPE_H
