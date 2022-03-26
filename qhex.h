#ifndef QHEX_H
#define QHEX_H

#include <QtCore>

class QHex {
	public:
		explicit QHex() {num = 0;}
		explicit QHex(const long long num = 0) { this->num = num; }
		QString toString(int byte, bool show0x = false, bool upperData = true, bool upper0x = false) const;
		inline long long toLongLong() { return num; }
		inline uint toUint() { return (uint)num; }
		inline int toInt() { return (int)num; }
		inline void operator=(const long long num) { this->num = num; }

	private:
		long long num;
};

class QHexList : public QList <uchar> {
	public:
		QString toString(QString sep = "") {
			QStringList list;
			for (int i = 0; i < this->count(); i++)
				list.append(QString("%1").arg(this->at(i), 2, 16, QChar('0')));
			return list.join(sep);
		}
		inline QString toString(QChar sep) { return toString(QString(sep)); }
};

#endif // QHEX_H
