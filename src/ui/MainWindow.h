#pragma once

#include <QtWidgets/QMainWindow>

#include "../System.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow, public System
{
Q_OBJECT

public:
	explicit MainWindow(MessageBus & messageBus);
	void cout(std::string string) const override;
	void handleMessage(Message & msg) const override;
	MessageBus & messageBus() const;

	~MainWindow() final;

private:
	Ui::MainWindow * ui;

private slots:

};