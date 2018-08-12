#include "findapp.h"



FindApp::FindApp(QWidget *parent)
	: QWidget(parent)
{
	m_main_layout = new QVBoxLayout;
	this->setLayout(m_main_layout);

	//add item
	{
		auto main_ctrl = new QWidget;
		auto main_ctrl_grid = new QGridLayout;
		main_ctrl->setLayout(main_ctrl_grid);
		m_main_layout->addWidget(main_ctrl);


		//tool name
		main_ctrl_grid->addWidget(new QLabel("Tool Name: "), 0, 0);
		main_ctrl_grid->addWidget(m_add_app_name = new QLineEdit, 0, 1);

		//path
		main_ctrl_grid->addWidget(new QLabel("Tool Path: "), 1, 0);
		main_ctrl_grid->addWidget(m_add_app_path = new QLineEdit, 1, 1);

		//calling
		auto cmdline_label = new QLabel("${exec}: path to executable\n ${left} left file\n ${right}: right side\n ${file}: for non diff tools");
		cmdline_label->setWordWrap(true);
		main_ctrl_grid->addWidget(new QLabel("Calling Commandline"), 2, 0);
		main_ctrl_grid->addWidget(m_add_app_cmdline = new QLineEdit, 2, 1);
		main_ctrl_grid->addWidget(cmdline_label, 2, 2);

		m_add_app_cmdline->setText("${exec} \"${left}\" \"${right}\"");

		//btns
		main_ctrl_grid->addWidget(m_add_app_btn = new QPushButton("add"), 3, 0);
		{
			auto groupBox = new QGroupBox;
			auto groupLayout = new QHBoxLayout;

			groupLayout->addWidget(m_add_app_radio_diff = new QRadioButton("Diff"));
			groupLayout->addWidget(m_add_app_radio_file = new QRadioButton("File"));
			groupBox->setLayout(groupLayout);

			main_ctrl_grid->addWidget(groupBox, 3, 1);
		}
	}

	//app list
	{
		m_app_list = new QWidget;
		m_app_list_scroll = new QScrollArea;
		m_main_layout->addWidget(m_app_list, 1);
	}



}


FindApp::~FindApp()
{
}
