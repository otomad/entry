/**
 * 图表编号范围：1 ~ ∞
 * 图例（即颜色）编号范围：0 ~ ∞
 * 数值的范围：0 ~ 4095
 * 单次数据数目：1024
 *
 * 发送第一个点的指令：
 * add 1,0,4095
 */

#include "oscilloscope.h"
#include "ui_oscilloscope.h"
#include "mainwindow.h"

static MainWindow *mainWindow;
static const uint DATA_MINIMUM = 0, DATA_MAXIMUM = 4095, ONCE_MAXIMUM = 1023;
static QFont bold, normal;

#undef startCode // 本方案的通信协议规则与之前的都不同
#undef endCode

#define DEBUG_MANUAL_ADD false
#define USE_SCATTER false

Oscilloscope::Oscilloscope(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Oscilloscope) {
	ui->setupUi(this);
	mainWindow = (MainWindow *)parentWidget();
	curPlotIndex = 1;
	bold.setBold(true);
	normal.setBold(false);
	ui->manualAddDataBox->setMinimum(DATA_MINIMUM);
	ui->manualAddDataBox->setMaximum(DATA_MAXIMUM);
	#if !DEBUG_MANUAL_ADD
	ui->manualAddDataBtn->setVisible(false);
	ui->manualAddDataBox->setVisible(false);
	#endif
	connect(ui->plotsListWidget, &QListWidget::itemClicked, ui->plotsListWidget, &QListWidget::itemActivated);
	ui->addPlotBtn->click();
}

Oscilloscope::Plot *Oscilloscope::plots_at(uint i) {
	if (i == 0) qDebug("Error: Cannot select plot 0!");
	if (i > (uint)plots.count()) qDebug("Error: The plot you selected is out of the plots count!");
	return plots.at((int)i - 1);
}

Oscilloscope::Plot *Oscilloscope::curPlot() {
	return plots_at(curPlotIndex);
};

Oscilloscope::~Oscilloscope() {
	delete ui;
	for (int i = 0; i < plots.count(); i++)
		if (plots.at(i) != nullptr)
			delete plots.at(i);
}

void Oscilloscope::plotsListWidgetUpdate() {
	bool PolyPlot = ui->plotsListWidget->count() > 1;
	ui->delPlotBtn->setEnabled(PolyPlot);
	ui->delPlotBtn->setToolTip(PolyPlot ? "" : "只有一个示波器时不能删除");
	on_plotsListWidget_itemActivated(ui->plotsListWidget->currentItem());
}

void Oscilloscope::on_addPlotBtn_clicked() {
	curPlotIndex = (uint)plots.count() + 1;
	const static QString PLOT_DEFAULT_NAME = "示波器 ";
	plots.append(new Plot(PLOT_DEFAULT_NAME + QString::number(curPlotIndex), curPlotIndex, ui->plotsListWidget, ui->plotWidget, this));
	plotsListWidgetUpdate();
}

void Oscilloscope::on_delPlotBtn_clicked() {
	if (ui->plotsListWidget->count() <= 1) {
		qDebug("Error: Cannot delete it, or there will be none item!");
		return;
	}
	delete curPlot();
	plots[(int)curPlotIndex - 1] = nullptr;
	for (int i = 0; i < plots.count(); i++)
		if (plots.at(i) == ui->plotsListWidget->currentItem())
			curPlotIndex = (uint)i + 1;
	plotsListWidgetUpdate();
}

void Oscilloscope::on_renPlotBtn_clicked() {
	/* // 单输入框
	bool ok;
	QString text = QInputDialog::getText(this, "重命名", QString("请为示波器 %1 指定新的名称").arg(curPlot()->index), QLineEdit::Normal, curPlot()->text(), &ok);
	if (ok && !text.isEmpty())
		curPlot()->setText(text);
	*/
	uint plotIndex = curPlot()->index;
	QDialog dialog(this);
	QFormLayout form(&dialog);
	dialog.setWindowTitle("重命名");
	dialog.setWindowFlags(DIALOG_WINDOW_NORESIZE);
	form.setRowWrapPolicy(QFormLayout::WrapAllRows);
	// 示波器名称
	QLineEdit *nameBox = new QLineEdit(&dialog);
	nameBox->setText(curPlot()->text());
	form.addRow(QString("请为示波器 %1 指定新的名称").arg(plotIndex), nameBox);
	// 示波器编号
	QSpinBox *indexBox = new QSpinBox(&dialog);
	indexBox->setMinimum(1);
	indexBox->setMaximum(65535);
	indexBox->setValue((int)plotIndex);
	form.addRow(QString("请为示波器 %1 指定新的编号").arg(plotIndex), indexBox);
	form.addRow(new QLabel("注意：\n"
		"名称的改变仅会影响本软件界面的显示；\n"
		"编号改变之后，客户端发送指令的图表编号也应改变。"
	, &dialog));
	// 确定取消
	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
	buttonBox.setMinimumHeight(40);
	form.addRow(&buttonBox);
	connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
	connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
	// 点击确定
	if (dialog.exec() == QDialog::Accepted) {
		QString name = nameBox->text();
		uint index = (uint)indexBox->value();
		if (name.isEmpty()) QMessageBox::critical(this, "错误", "名称不能为空！");
		else curPlot()->setText(name);
		if (index != plotIndex) {
			bool ok = curPlot()->changeIndex(index);
			if (!ok) QMessageBox::critical(this, "错误", "指定的编号已被其它图表占用！");
		}
	}
	plotsListWidgetUpdate();
}

const char Oscilloscope::Plot::listWidgetItem[] = "listWidgetItem";

Oscilloscope::Plot::Plot(QString name, uint index, QListWidget *container, QWidget *layoutPosition, Oscilloscope *parent) :
	QObject(container),
	QListWidgetItem(name, container) {
	this->index = index;
	this->parent = parent;
	this->setToolTip(QString("示波器编号 %1").arg(index));
	plot = new QCustomPlot(layoutPosition);
	layoutPosition->layout()->addWidget(plot);
	plot->setProperty(listWidgetItem, QVariant((quint64)this));
	plot->legend->setBrush(QColor(255, 255, 255, (int)(255 * 0.75)));
	plot->legend->setSelectedTextColor(myColor.CYAN);
	plot->legend->setSelectedFont(myFont.font_bold);
	plot->legend->setSelectedIconBorderPen(QPen(myColor.CYAN, 2));
	plot->legend->setFont(myFont.font_);
	plot->legend->setSelectedFont(myFont.font_);
	plot->xAxis->setTickLabelFont(myFont.font_en);
	plot->yAxis->setTickLabelFont(myFont.font_en);
	plot->xAxis->setLabelFont(myFont.font_);
	plot->yAxis->setLabelFont(myFont.font_);
	plot->setInteractions(QCP::iRangeDrag //鼠标可拖动
						  | QCP::iRangeZoom //滚轮滑动缩放
						  | QCP::iSelectAxes //轴可选
						  | QCP::iSelectLegend //图例可选
						  | QCP::iSelectPlottables //曲线可选
						 );
	plot->legend->setSelectableParts(QCPLegend::spItems); //设置legend只能选择图例
	plot->xAxis->setRange(0, ONCE_MAXIMUM);
	plot->yAxis->setRange(DATA_MINIMUM, DATA_MAXIMUM);
	QList <QCPAxis *> axis_x({ plot->xAxis });
	plot->axisRect()->setRangeZoomAxes(axis_x);
	plot->axisRect()->setRangeDragAxes(axis_x);
	connect(plot, &QCustomPlot::selectionChangedByUser, this, &Plot::selectionChanged); // 链接信号槽plot即为QCustomPlot对象
	container->setCurrentItem(this);
}

void Oscilloscope::Plot::selectionChanged() {
	for (int i = 0; i < plot->graphCount(); ++i)
		mainWindow->setSelectChtLineStyle(i, plot);
}

bool Oscilloscope::Plot::changeIndex(uint index) {
	if (index < 1) return false;
	if (index <= (uint)parent->plots.count()) {
		if (parent->plots_at(index) != nullptr) return false;
	} else {
		while (index > (uint)parent->plots.count()) parent->plots.append(nullptr);
	}
	parent->plots[(int)index - 1] = this;
	parent->plots[(int)parent->curPlotIndex - 1] = nullptr;
	parent->curPlotIndex = index;
	this->index = index;
	this->setToolTip(QString("示波器编号 %1").arg(index));
	return true;
}

Oscilloscope::Plot::~Plot() {
	delete plot;
}

void Oscilloscope::on_plotsListWidget_itemActivated(QListWidgetItem *item) {
	bool notFirstTime = mainWindow->isVisible();
//	qDebug() << item;
	bool flag = false;
	for (int i = 0; i < plots.count(); i++)
		if (plots.at(i) == item) {
			curPlotIndex = (uint)i + 1;
			if (notFirstTime) qDebug("curPlotIndex: %d", curPlotIndex);
			flag = true;
			break;
		}
	if (!flag) {
		qDebug("Error: Cannot select any Plot!");
		return;
	}
	{
		#define _t QWidget *
		QList <_t> items = ui->plotWidget->findChildren <_t> ();
		foreach (_t item, items) {
			/* Plot *listWI = (Plot *)item->property(Plot::listWidgetItem).toULongLong();
			qDebug("remove plot: %s", listWI->text().toStdString().c_str());
			ui->plotWidget->layout()->removeWidget(item); */
			item->setVisible(false);
		}
		#undef _t
	}
	for (int i = 0; i < ui->plotsListWidget->count(); i++)
		ui->plotsListWidget->item(i)->setFont(normal);
	item->setFont(bold);
//	ui->plotWidget->layout()->addWidget(curPlot()->plot);
	curPlot()->plot->setVisible(true);
	if (notFirstTime)  mainWindow->setCurPlotItemText(item->text());
}

void Oscilloscope::on_manualAddDataBtn_clicked() {
	int value = ui->manualAddDataBox->value();
	const static uint DEFAULT_LEGEND = 0;
	addPoint(curPlotIndex, DEFAULT_LEGEND, value);
}

bool Oscilloscope::addPoint(uint chart, uint legend, int value, bool refreshRightNow) {
	#define failReadChart {\
		qDebug("Error: Fail to open chart %d", chart);\
		mainWindow->showStatus(QString("客户端正在试图向您在一个不存在的示波器 %1 上的图例 %2 上绘制一个值为 %3 的点。").arg(chart).arg(legend).arg(value), 3000);\
		return false;\
	}
	if (chart < 1 || chart > (uint)plots.count()) failReadChart;
	if (plots_at(chart) == nullptr) failReadChart;
	plots_at(chart)->addPoint(legend, value, refreshRightNow);
	return true;
	#undef failReadChart
}

void Oscilloscope::Plot::addPoint(uint legend, int value, bool refreshRightNow) {
	qsrand((uint)QTime(0, 0, 0).secsTo(QTime::currentTime()));
	while ((int)legend > plot->graphCount() - 1) {
		QCPGraph *graph = plot->addGraph();
		graph->setName(QString("波形 %1").arg(plot->graphCount() - 1));
		QColor color = QColor::fromHsl(qrand() % 360, 194, 106); // h, 183, 100
		graph->setPen(QPen(color, 2));
		#if USE_SCATTER
		graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(myColor.ORANGE), QColor(myColor.WHITE), 6));
		#endif
		graph->setSmooth(true); // 开启平滑曲线
	}
	plot->legend->setVisible(true);
	QCPGraph *graph = plot->graph((int)legend);
	int x = graph->dataCount();
	graph->addData(x, value); // 为曲线图添加数据
	// 以下是若点超出视图则自动平移
	double xMin = plot->xAxis->range().lower,
		   xMax = plot->xAxis->range().upper,
		   xWidth = xMax - xMin;
	if (x > xMax) {
		xMax = x;
		xMin = x - xWidth;
	} else if (x < xMin) { // 理论上不会出现这种情况
		xMin = x;
		xMax = x + xWidth;
	}
	plot->xAxis->setRange(xMin, xMax);
	if (refreshRightNow) plot->replot(QCustomPlot::rpQueuedReplot);
}

QCustomPlot *Oscilloscope::getCurrentPlot() {
	return curPlot()->plot;
}

QString Oscilloscope::getCurrentPlotName() {
	return curPlot()->text();
}

int Oscilloscope::readyRead(const QString str) {
	const static QString START_CODE = "add ";
	int ret = -1;
	QStringList str_row = str.split("\n", QString::SkipEmptyParts);
	foreach (QString row, str_row) {
		if (row.trimmed().left(START_CODE.length()) != START_CODE) continue;
		row = row.mid(4); // 数据部分
		QStringList arr_str = row.split(',', QString::SkipEmptyParts);
//		if (arr_str.count() != 3) return 0; // 数据长度不是正好为 3 位，则认为指令不合法。
		QList <int> arr;
		for (int i = 0; i < 3 /*arr_str.count()*/; i++) {
			QString item = arr_str[i].trimmed();
			bool ok;
			int value = item.toInt(&ok);
			if (!ok
				|| (i == 0 && value < 1)
				|| (i == 1 && value < 0)
			) { // 不允许输入一个负数
				if (ret == -1) ret = 0;
				continue;
			}
			arr.append(value);
		}
		addPoint((uint)arr.at(0), (uint)arr.at(1), arr.at(2), false);
		ret = 1;
	}
	if (ret == 1)
		foreach (Plot *pl, plots) {
			if (pl != nullptr)
				pl->plot->replot(QCustomPlot::rpQueuedReplot);
		}
	return ret;
}

void Oscilloscope::on_clrPlotBtn_clicked() {
	QCustomPlot *plot = curPlot()->plot;
	plot->clearGraphs();
	plot->legend->setVisible(false);
	plot->replot(QCustomPlot::rpQueuedReplot);
}
