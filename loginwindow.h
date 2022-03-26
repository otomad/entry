#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QKeyEvent>
#include "mainwindow.h"
#include "serversetting.h"
#include "common.h"

namespace Ui {
	class LoginWindow;
}

class LoginWindow : public QMainWindow {
    Q_OBJECT

	public:
		explicit LoginWindow(QWidget *parent = nullptr);
		~LoginWindow() override;

	private slots:
		void on_ExitBtn_clicked();
		void on_ToRegister_clicked();
		void on_LoginBtn_clicked();
		void on_ServerSetting_clicked();
		void keyPressEvent(QKeyEvent *event) override;

	private:
		Ui::LoginWindow *ui;
		void ensurePassword_init();
		MainWindow *mainWindow = new MainWindow(nullptr);
		ServerSetting *setting = new ServerSetting(this, ServerSetting::MainWindow);
		int isReginster = 0;
		void login();
};

#endif // LOGINWINDOW_H
