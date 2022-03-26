#include "mainwindow.h"
#include "ui_mainwindow.h"

_MyColor myColor = {
	/* CYAN */ QColor(25, 187, 180),
	/* BLUE */ QColor(0, 129, 255),
	/* RED */ QColor(230, 68, 75),
	/* ORANGE */ QColor(255, 126, 0),
	/* WHITE */ Qt::white,
	/* BLACK */ Qt::black,
	/* GREEN */ QColor(34, 177, 76)
};
_MyFont myFont = {
	/* font_ */ QFont("Microsoft Yahei UI", 10),
	/* font_en */ QFont("Segoe UI", 10),
	/* font_bold */ QFont("Microsoft Yahei UI", 10, QFont::Bold),
	/* font_italic */ QFont("Microsoft Yahei UI", 10, -1, QFont::StyleItalic)
};

QString MainWindow::CurrentPlotItem::currentCaptionVa = "V/A",
		MainWindow::CurrentPlotItem::currentCaptionIn = "IN",
		MainWindow::CurrentPlotItem::currentCaptionOut = "OUT",
		MainWindow::CurrentPlotItem::currentCaptionNA = "当前";

void MainWindow::plot_init() {
	on_showNowBtn_clicked();
	plot = ui->qcp;
	graphI = plot->addGraph();
	graphI->setName("电流 (A)");
	graphU = plot->addGraph();
	graphU->setName("电压 (V)");
	plot->xAxis->setLabel("时间 (s)");
	// plot->yAxis->setLabel("y"); // y轴暂不需要标签
	plot->legend->setVisible(true);
	plot->legend->setBrush(QColor(255, 255, 255, (int)(255 * 0.75)));
	plot->legend->setSelectedTextColor(myColor.CYAN);
	plot->legend->setSelectedFont(myFont.font_bold);
	plot->legend->setSelectedIconBorderPen(QPen(myColor.CYAN, 2));
	//设置字体
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
	axis_x.append(plot->xAxis); //轴缩放
	axis_y.append(plot->yAxis);
	axis_xy.append(plot->xAxis);
	axis_xy.append(plot->yAxis);
	//connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_I), this), &QShortcut::activated, [](){qDebug() << "Here we are!";});
	smooth();
	plot->rescaleAxes();
	plot->setAttribute(Qt::WA_Hover, true); //开启悬停事件
	plot->installEventFilter(this); //安装事件过滤器
	connect(plot, &QCustomPlot::selectionChangedByUser, this, &MainWindow::selectionChanged); // 链接信号槽plot即为QCustomPlot对象
	//工具栏
	#define qati &QAction::triggered, this, [&, i]()
	QList <QAction *> actions({
								 ui->autoZoom,
								 ui->leftMove,
								 ui->rightMove,
								 ui->xZoomIn,
								 ui->xZoomOut,
								 ui->upMove,
								 ui->downMove,
								 ui->yZoomIn,
								 ui->yZoomOut
							 });
	for (int i = 0; i < actions.count(); i++)
		connect(actions[i], qati {
			QString tab = getCurrentTabText();
			QCustomPlot *curPlot = (
				tab == ToolName::wave ? this->plot :
				tab == ToolName::distort ? tabDistort->getCurrentPlot() :
				tab == ToolName::scope ? tabScope->getCurrentPlot() :
				nullptr
			);
			if (curPlot != nullptr) resizePlot(i, curPlot);
		});
	//工具栏选择当前图表的下拉菜单
	curPlotItem->tb = new QToolButton(this);
	curPlotItem->currentPlotMenu = new QMenu(this);
	curPlotItem->currentPlotGroup = new QActionGroup(this);
	curPlotItem->vaPlot = new QAction(this);
	curPlotItem->inPlot = new QAction(this);
	curPlotItem->outPlot = new QAction(this);
	curPlotItem->vaPlot->setText(CurrentPlotItem::currentCaptionVa);
	curPlotItem->vaPlot->setCheckable(true);
	curPlotItem->inPlot->setText(CurrentPlotItem::currentCaptionIn);
	curPlotItem->inPlot->setCheckable(true);
	curPlotItem->outPlot->setText(CurrentPlotItem::currentCaptionOut);
	curPlotItem->outPlot->setCheckable(true);
	curPlotItem->tb->setMenu(curPlotItem->currentPlotMenu);
	curPlotItem->tb->setPopupMode(QToolButton::InstantPopup);
	curPlotItem->tb->setText(CurrentPlotItem::currentCaptionNA);
	curPlotItem->tb->setToolTip("选择当前图表，以响应工具栏图标和快捷键操作");
	ui->toolBar->addSeparator();
	ui->toolBar->addWidget(curPlotItem->tb);
	/*connect(curPlotItem->tb, &QToolButton::triggered, this, [&]() {
		curPlotItem->currentPlotMenu->clear();
		if (getCurrentTabText() == ToolName::wave) {
			curPlotItem->currentPlotMenu->addAction(curPlotItem->currentPlotGroup->addAction(curPlotItem->vaPlot));
		} else if (getCurrentTabText() == ToolName::distort) {
			curPlotItem->currentPlotMenu->addAction(curPlotItem->currentPlotGroup->addAction(curPlotItem->inPlot));
			curPlotItem->currentPlotMenu->addAction(curPlotItem->currentPlotGroup->addAction(curPlotItem->outPlot));
		}
	});*/
	connect(curPlotItem->inPlot, &QAction::triggered, this, [&]() {
		curPlotItem->inPlot->setChecked(true);
		curPlotItem->tb->setText(curPlotItem->inPlot->text());
		tabDistort->currentPlot = NonlinearDistortion::CurrentInPlot;
		configIni->setValue("Personalize/DistortionCurrentPlot", 0);
	});
	connect(curPlotItem->outPlot, &QAction::triggered, this, [&]() {
		curPlotItem->outPlot->setChecked(true);
		curPlotItem->tb->setText(curPlotItem->outPlot->text());
		tabDistort->currentPlot = NonlinearDistortion::CurrentOutPlot;
		configIni->setValue("Personalize/DistortionCurrentPlot", 1);
	});
}

void MainWindow::keyPressEvent(QKeyEvent *ev) {
	const int key = ev->key();
	if (key == Qt::Key_Control) {
		plot->axisRect()->setRangeZoomAxes(axis_x);
	}
	if (key == Qt::Key_Shift) { //暂不清楚为什么不能用Alt
		plot->axisRect()->setRangeZoomAxes(axis_y);
	}
	QWidget::keyPressEvent(ev);
}

void MainWindow::keyReleaseEvent(QKeyEvent *ev) {
	plot->axisRect()->setRangeZoomAxes(axis_xy);
	QWidget::keyReleaseEvent(ev);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) { //事件过滤器
	const int type = event->type();
	if (obj == plot) {
		if (type == QEvent::HoverEnter) {
			showStatus("滚轮改变大小，按住 Ctrl 键滚轮缩放 x 轴，按住 Shift 键滚轮缩放 y 轴。");
			return true;
		}
		if (type == QEvent::HoverLeave) {
			clearStatus();
			return true;
		}
	}
	if (obj->isWidgetType()) {
		if (type == QEvent::Close) {
			backToTabwidget(obj);
			return true;
		}
	}
	return QWidget::eventFilter(obj, event);
}

void MainWindow::square() {
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
	graphI->setPen(QPen(myColor.ORANGE, 2));
	graphI->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(myColor.ORANGE), QColor(myColor.WHITE), 6));
	graphI->setData(xdata, ydataI);
	graphI->setSmooth(true); // 开启平滑曲线
	graphU->setPen(QPen(myColor.BLUE, 2));
	graphU->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QColor(myColor.BLUE), QColor(myColor.WHITE), 6));
	graphU->setData(xdata, ydataU);
	graphU->setSmooth(true);
}

void MainWindow::resizePlot(const int mode, QCustomPlot *plot) {
	QStringList avoidYMoveList({ // 阻止 y 轴移动
		ToolName::distort,
		ToolName::scope
	});
	bool avoidYMove = avoidYMoveList.indexOf(getCurrentTabText()) != -1;
	double xMin = plot->xAxis->range().lower,
		   xMax = plot->xAxis->range().upper,
		   yMin = plot->yAxis->range().lower,
		   yMax = plot->yAxis->range().upper;
	if (mode) {
		double scaleBase = 10, // 此处规定一次平移/缩放 1/10 的屏幕
			   xScale = (xMax - xMin) / scaleBase,
			   yScale = (yMax - yMin) / scaleBase;
		switch (mode) {
			case 1: xMin -= xScale; xMax -= xScale; break;
			case 2: xMin += xScale; xMax += xScale; break;
			case 3: xMin += xScale; xMax -= xScale; break;
			case 4: xMin -= xScale; xMax += xScale; break;
			case 5: if (avoidYMove) break; yMin += yScale; yMax += yScale; break;
			case 6: if (avoidYMove) break; yMin -= yScale; yMax -= yScale; break;
			case 7: if (avoidYMove) break; yMin += yScale; yMax -= yScale; break;
			case 8: if (avoidYMove) break; yMin -= yScale; yMax += yScale; break;
			default: break;
		}
		plot->xAxis->setRange(xMin, xMax);
		plot->yAxis->setRange(yMin, yMax);
	} else {
		plot->rescaleAxes();
		if (avoidYMove) plot->yAxis->setRange(yMin, yMax);
	}
	plot->replot(QCustomPlot::rpQueuedReplot);
}

/* void MainWindow::wheelEvent(QWheelEvent* event) {
	if (event->type() == QEvent::Wheel) {
		QWheelEvent *wheelEvent=static_cast<QWheelEvent *>(event);
		//获取鼠标坐标点
		int x_pos = wheelEvent->pos().x();
		int y_pos = wheelEvent->pos().y();
		// 把鼠标坐标点 转换为 QCustomPlot 内部坐标值 （pixelToCoord 函数）
		// coordToPixel 函数与之相反 是把内部坐标值 转换为外部坐标点
		double x_val = plot->xAxis->pixelToCoord(x_pos);
		double y_val = plot->yAxis->pixelToCoord(y_pos);
		//qDebug()<<x_val << y_val;
		if(wheelEvent->delta()>0) {
			// 这里你可以填写自己想要实现的效果
			//double dCenter = plot->xAxis->range().center();
			//缩小区间 (放大 plotTables 鼠标向外滚动)
			if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
				plot->yAxis->scaleRange(0.8, y_val);
				plot->replot(QCustomPlot::rpQueuedReplot);
				return;
			}
			plot->xAxis->scaleRange(0.5, x_val);
			plot->replot(QCustomPlot::rpQueuedReplot);
		} else {
			// 这里你可以填写自己想要实现的效果
			//double dCenter = plot->xAxis->range().center();
			// 扩大区间 （缩小 plottables 鼠标向内滚动）
			if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
				plot->yAxis->scaleRange(1.25, y_val);
				plot->replot(QCustomPlot::rpQueuedReplot);
				return;
			}
			plot->xAxis->scaleRange(2, x_val);
			plot->replot(QCustomPlot::rpQueuedReplot);
		}
		return;
	}
} */

void MainWindow::selectionChanged() {
	for (int i = 0; i < plot->graphCount(); ++i)
		setSelectChtLineStyle(i, plot);
}

void MainWindow::setSelectChtLineStyle(int sceneIndex, QCustomPlot *plot) {
	QCPGraph *graph = plot->graph(sceneIndex);
	QCPPlottableLegendItem *item = plot->legend->itemWithPlottable(graph);
	if (item->selected()) {
		const bool visible = !graph->visible();
		graph->setVisible(visible);
		item->setTextColor(visible ? myColor.BLACK : myColor.RED);
		item->setFont(visible ? myFont.font_ : myFont.font_italic);
		item->setSelected(false);
		QString name = graph->name().remove(QRegExp("\\(.*\\)")).trimmed();
		showStatus(QString("%1图层已%2。").arg(name, visible ? "显示" : "隐藏"), 3000);
	}
	if (graph->selected()) {
		item->setSelected(true);
		graph->selectionDecorator()->setPen(QPen(myColor.CYAN, 4));
		graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
	}
}

void MainWindow::on_showNowBtn_clicked() {
	ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
}

void MainWindow::on_setVA_clicked() {
	double A = ui->setABox->value(), V = ui->setVBox->value();
	QString dataA = "0431" + QString::number((int)round(A * 100), 16),
			dataV = "0432" + QString::number((int)round(V * 100), 16),
			crcA = getCrc(dataA),
			crcV = getCrc(dataV),
			code;
	code = startCode + dataA + crcA + endCode +
		   startCode + dataV + crcV + endCode;
	send(QString2Hex(code));
}
