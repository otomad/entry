#include "audioanalysis.h"
#include "ui_audioanalysis.h"
#include "common.h"

AudioAnalysis::AudioAnalysis(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::AudioAnalysis) {
	ui->setupUi(this);
	wavePlot = ui->wavePlot;
	wavePlot->setInteractions(QCP::iRangeDrag //鼠标可拖动
						  | QCP::iRangeZoom //滚轮滑动缩放
						  | QCP::iSelectAxes //轴可选
						  | QCP::iSelectLegend //图例可选
						  | QCP::iSelectPlottables //曲线可选
						 );
	wavePlot->legend->setSelectableParts(QCPLegend::spItems); //设置legend只能选择图例
	wavePlot->xAxis->setRange(0, 1023);
	wavePlot->yAxis->setRange(0, 3.3);
	wavePlot->yAxis->setLabel("V");
	QList <QCPAxis *> axis_x({ wavePlot->xAxis });
	wavePlot->axisRect()->setRangeZoomAxes(axis_x);
	wavePlot->axisRect()->setRangeDragAxes(axis_x);
	waveGraph = wavePlot->addGraph();
	waveGraph->setPen(QPen(Qt::red));
	waveGraph->setSmooth(true); // 开启平滑曲线
	spectPlot = ui->spectrumPlot;
	spectPlot->xAxis->setRange(0, 10000);
	spectPlot->yAxis->setRange(0, 3.5);
	spectPlot->xAxis->setLabel("Hz");
	spectPlot->yAxis->setLabel("V");
	spectGraph = spectPlot->addGraph();
	spectGraph->setPen(QPen(Qt::blue));
	spectGraph->setBrush(QBrush(Qt::darkBlue));
	spectGraph->setLineStyle(QCPGraph::lsImpulse);
	wavePlot->xAxis->setTickLabelFont(myFont.font_en);
	wavePlot->yAxis->setTickLabelFont(myFont.font_en);
	wavePlot->xAxis->setLabelFont(myFont.font_);
	wavePlot->yAxis->setLabelFont(myFont.font_);
	spectPlot->xAxis->setTickLabelFont(myFont.font_en);
	spectPlot->yAxis->setTickLabelFont(myFont.font_en);
	spectPlot->xAxis->setLabelFont(myFont.font_);
	spectPlot->yAxis->setLabelFont(myFont.font_);
	wavePlot->setVisible(false); // 胡杰你别搞
}

AudioAnalysis::~AudioAnalysis() {
	delete ui;
}

/*void AudioAnalysis::showInfo(float 二频, float 三频, float 一功, float 二功, float 总功率, bool 是否周期, float 失真度, int 频率) {
	ui->infoLbl->setText(QString("二　频：%1\n三　频：%2\n一　功：%3 mW\n二　功：%4 mW\n总功率：%5 mW\n周期性：%6\n失真度：%7%\n频　率：%8 Hz")
		.arg((double)二频).arg((double)三频).arg((double)一功).arg((double)二功).arg((double)总功率).arg(是否周期 ? "周期" : "非周期")
		.arg((double)失真度).arg(频率));
}*/

void AudioAnalysis::showInfo(QStringList strs) {
	if (strs.length() < 8) return;
	const QString OUT_OF_RANGE = "超出或无";
	if (strs.at(0) == "0") strs[0] = OUT_OF_RANGE;
	if (strs.at(1) == "0") strs[1] = OUT_OF_RANGE;
	bool powerOk[3];
	QString powerS[3] = { strs.at(2), strs.at(3), strs.at(4) };
	float powerF[3] = { powerS[0].toFloat(&powerOk[0]), powerS[1].toFloat(&powerOk[1]), powerS[2].toFloat(&powerOk[2]) };
	if (powerOk[2]) {
		if (powerOk[0] && powerF[0] > powerF[2]) {
			powerF[0] /= 10.0f;
			powerS[0] = QString::number((double)powerF[0]);
		}
		if (powerOk[1] && powerF[1] > powerF[2]) {
			powerF[1] /= 10.0f;
			powerS[1] = QString::number((double)powerF[1]);
		}
	}
	QString info = QString("二　频：%1\n三　频：%2\n一　功：%3 mW\n二　功：%4 mW\n总功率：%5 mW\n周期性：%6\n失真度：%7%\n频　率：%8 Hz")
			.arg(strs.at(0), strs.at(1), powerS[0], powerS[1], powerS[2], strs.at(5) != "0" ? "周期" : "非周期", strs.at(6), strs.at(7));
	ui->infoLbl->setText(info);
	QString curTime = QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm:ss.zzz");
	historyInfo += curTime + "\r\n" + info + "\r\n\r\n";
}

void AudioAnalysis::readyRead(QString all) {
	QStringList list = all.split('\n', QString::SkipEmptyParts);
	foreach (QString str, list) {
		str = str.trimmed();
		if (!str.startsWith("audio ", Qt::CaseInsensitive)) continue;
		str = str.mid(6).trimmed();
		if (str.startsWith("info ")) {
			QStringList infos = str.mid(5).trimmed().split(',', QString::SkipEmptyParts);
			showInfo(infos);
		} else if (str.startsWith("wave ")) {
			QStringList strs = str.mid(5).trimmed().split(',', QString::SkipEmptyParts);
			addPoint(strs2floats(strs));
		} else if (str.startsWith("spec ")) {
			QStringList strs = str.mid(5).trimmed().split(',', QString::SkipEmptyParts);
			drawSpect(strs2floats(strs));
		}
	}
	updatePlot();
}

void AudioAnalysis::addPoint(float point) {
	int x = waveGraph->dataCount();
	waveGraph->addData(x, (double)point); // 为曲线图添加数据
}

void AudioAnalysis::addPoint(QList<float> data) {
	foreach (float point, data) {
		addPoint(point);
	}
}

void AudioAnalysis::updatePlot() {
	// 以下是若点超出视图则自动平移
	double xMin = wavePlot->xAxis->range().lower,
		   xMax = wavePlot->xAxis->range().upper,
		   xWidth = xMax - xMin;
	int x = waveGraph->dataCount();
	if (x > xMax) {
		xMax = x;
		xMin = x - xWidth;
	} else if (x < xMin) { // 理论上不会出现这种情况
		xMin = x;
		xMax = x + xWidth;
	}
	wavePlot->xAxis->setRange(xMin, xMax);
	/*if (refreshRightNow) */
	wavePlot->replot(QCustomPlot::rpQueuedReplot);
	spectPlot->replot(QCustomPlot::rpQueuedReplot);
}

void AudioAnalysis::drawSpect(QList<float> data) {
	int startPos = (int)data.at(0);
	for (int i = 1; i < data.length(); i++) {
		double x = (startPos + i - 1) * SPECT_GAP;
		spectGraph->data()->remove(x);
		spectGraph->addData(x, (double)data.at(i));
	}
}

QList<float> AudioAnalysis::strs2floats(QStringList strs) {
	QList<float> list;
	foreach (QString str, strs) {
		bool ok;
		float f = str.toFloat(&ok);
		if (!ok) continue;
		list.append(f);
	}
	return list;
}

void AudioAnalysis::on_clearBtn_clicked() {
	ui->infoLbl->setText("");
	waveGraph->data()->clear();
	spectGraph->data()->clear();
	wavePlot->replot(QCustomPlot::rpQueuedReplot);
	spectPlot->replot(QCustomPlot::rpQueuedReplot);
	historyInfo = "";
}

void AudioAnalysis::on_exportBtn_clicked() {
	QFile file("D:\\audio_data.log");
	if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
		QMessageBox::critical(this, "错误", "无法打开文件！");
		return;
	}
	/*QString content = "";
	for (int i = 0; i < waveGraph->dataCount(); i++)
		content += (i != 0 ? "," : "") + QString::number(waveGraph->data()->at(i)->value);*/
	qint64 length = -1;
	length = file.write(historyInfo.toLatin1(), historyInfo.length());
	if (length == -1) QMessageBox::critical(this, "错误", "无法写入文件！");
	file.close();
	QMessageBox::information(this, "提示", "导出数据成功到 D:\\audio_data.log！");
}
