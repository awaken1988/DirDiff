#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_vs2007_project.h"

class vs2007_project : public QMainWindow
{
	Q_OBJECT

public:
	vs2007_project(QWidget *parent = Q_NULLPTR);

private:
	Ui::vs2007_projectClass ui;
};
