#include "nonlineardistortion.h"
#include "ui_nonlineardistortion.h"
#include "mainwindow.h"

static MainWindow *mainWindow;

NonlinearDistortion::NonlinearDistortion(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::NonlinearDistortion) {
	ui->setupUi(this);
	mainWindow = (MainWindow *)parentWidget();
	inPlot = ui->inPlot;
	outPlot = ui->outPlot;
	QCP::Interactions interactions = QCP::iRangeDrag //鼠标可拖动
								   | QCP::iRangeZoom //滚轮滑动缩放
	;
	inPlot->setInteractions(interactions);
	outPlot->setInteractions(interactions);
	inGraph = inPlot->addGraph();
	outGraph = outPlot->addGraph();
	QList <QCPAxis *> inAxisX, outAxisX;
	inAxisX.append(inPlot->xAxis);
	outAxisX.append(outPlot->xAxis);
	const QFont font_en("Segoe UI", 10);
	inPlot->xAxis->setTickLabelFont(font_en);
	inPlot->yAxis->setTickLabelFont(font_en);
	inPlot->axisRect()->setRangeZoomAxes(inAxisX);
	inPlot->axisRect()->setRangeDragAxes(inAxisX);
	inPlot->yAxis->setRange(0, 5);
	outPlot->xAxis->setTickLabelFont(font_en);
	outPlot->yAxis->setTickLabelFont(font_en);
	outPlot->axisRect()->setRangeZoomAxes(outAxisX);
	outPlot->axisRect()->setRangeDragAxes(outAxisX);
	outPlot->yAxis->setRange(0, 5);
	inGraph->setPen(QPen(Qt::blue, 2));
	outGraph->setPen(QPen(Qt::red, 2));
	{
		QList <QPushButton *> distortionBtns({
												 ui->DistortionNoBtn,
												 ui->DistortionSaturationBtn,
												 ui->DistortionCutoffBtn,
												 ui->DistortionBidirectionalBtn,
												 ui->DistortionCrossoverBtn
											 });
		int i = 0;
		foreach(auto button, distortionBtns) {
			connect(button, &QPushButton::clicked, this, [&, i]() { exampleDistort(i); }); i++;
		}
	}
}

NonlinearDistortion::~NonlinearDistortion() {
	delete ui;
}

#define PI acos(-1)

void NonlinearDistortion::exampleDistort(int num) {
	//显示范例图形
	const int precision = 100, length = 5, max = 5;
	const double amp = max / 2.0;
	QVector <double> X, Y;
	for (int i = 0; i <= precision; i++) {
		double x, y;
		x = (double)length / precision * i;
		y = amp * sin(PI * x) + amp;
		switch (num) {
			case 1: y = qMax(y, 1.0); break;
			case 2: y = qMin(y, 4.0); break;
			case 3: y = qMin(qMax(y, 1.0), 4.0); break;
			case 4: y = pow(y - amp, 3) / pow(2.5, 3 - 1) + amp; break;
			default: break;
		}
		X.append(x);
		Y.append(y);
	}
	outGraph->setData(X, Y);
	outGraph->setSmooth(true);
	outPlot->replot(QCustomPlot::rpQueuedReplot);
	//发送指令
	QString data = "0330" + QString("%1").arg(num, 2, 10, QChar('0')),
			crc = getCrc(data),
			code = startCode + data + crc + endCode;
	mainWindow->send(code, Encode::HEX);
}

/*void MainWindow::square() {
	QVector<double> x(101), y(101);
	for (int i = 0; i < 101; ++i) {
		x[i] = i / 50.0 - 1; // -1 到 1
		y[i] = pow(x[i], 2);
	}
//	plot->addGraph(); // 添加一个曲线图QGraph
	graphI->setData(x, y); // 为曲线图添加数据
	graphI->setName("第一个示例"); // 设置曲线图的名字
	plot->xAxis->setLabel("x"); // 设置x轴的标签
	plot->yAxis->setLabel("y");
	plot->xAxis->setRange(-1, 1); // 设置x轴的范围为(-1,1)
	plot->yAxis->setRange(0, 1);
	plot->legend->setFont(QFont("微软雅黑", 9));
	plot->legend->setVisible(true); // 显示图例
}

void MainWindow::smooth() {
	QVector <double> xdata = { 1, 2, 3, 4, 5, 6, 7 };
	QVector <double> ydataI = { 820, 932, 901, 934, 1290, 1330, 1320 };
	QVector <double> ydataU = { 979, 789, 1054, 1320, 899, 1007, 1240 };
	graphI->setPen(QPen(ORANGE, 2));
	graphI->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(ORANGE), QColor(WHITE), 6));
	graphI->setData(xdata, ydataI);
	graphI->setSmooth(true); // 开启平滑曲线
	graphU->setPen(QPen(BLUE, 2));
	graphU->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(BLUE), QColor(WHITE), 6));
	graphU->setData(xdata, ydataU);
	graphU->setSmooth(true);
}*/

QCustomPlot *NonlinearDistortion::getCurrentPlot() {
	return (currentPlot == CurrentInPlot ? inPlot : outPlot);
}
