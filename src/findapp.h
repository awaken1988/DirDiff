#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QRadioButton>

class FindApp : public QWidget
{
	Q_OBJECT
public:
	FindApp(QWidget *parent=nullptr);
	~FindApp();
private:
	QVBoxLayout * m_main_layout;
	QWidget* m_app_list;
	QScrollArea* m_app_list_scroll;

	QLineEdit* m_add_app_name;
	QLineEdit* m_add_app_path;
	QLineEdit* m_add_app_cmdline;

	QPushButton* m_add_app_btn;
	QRadioButton* m_add_app_radio_diff;
	QRadioButton* m_add_app_radio_file;

};

