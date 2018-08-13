#include "findapp.h"



FindApp::FindApp(QWidget *parent)
	: QWidget(parent)
{
	m_main_layout = new QGridLayout;
	this->setLayout(m_main_layout);

	m_main_layout->addWidget(m_app_list = new QListWidget, 0, 0);

	//Add AppItem
	{
		m_add_app = new QPushButton("Add");

		connect(m_add_app, &QPushButton::clicked, [this](bool aChecked) {
			addItem();
		});

		m_main_layout->addWidget(m_add_app, 1, 0);
	}

	//create empty entry
	addItem();
}

void FindApp::addItem()
{
	auto* listWidgetItem = new QListWidgetItem(m_app_list);
	m_app_list->addItem(listWidgetItem);

	auto appItem = new AppItem;

	listWidgetItem->setSizeHint(appItem->sizeHint());

	m_app_list->setItemWidget(listWidgetItem, appItem);
}


FindApp::~FindApp()
{
}



AppItem::AppItem(QWidget *parent)
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
		{
			auto groupBox = new QGroupBox;
			auto groupLayout = new QHBoxLayout;

			groupLayout->addWidget(m_add_app_radio_diff = new QRadioButton("Diff"));
			groupLayout->addWidget(m_add_app_radio_file = new QRadioButton("File"));
			groupBox->setLayout(groupLayout);

			m_add_app_radio_diff->setChecked(true);

			main_ctrl_grid->addWidget(groupBox, 0, 2);
		}


		//path
		main_ctrl_grid->addWidget(new QLabel("Tool Path: "), 1, 0);
		main_ctrl_grid->addWidget(m_add_app_path = new QLineEdit, 1, 1);

		//calling
		auto cmdline_label = new QLabel("${exec}: path to executable\n ${left} left file\n ${right}: right side\n ${file}: for non diff tools");
		cmdline_label->setWordWrap(true);
		main_ctrl_grid->addWidget(new QLabel("Calling Commandline"), 2, 0);
		main_ctrl_grid->addWidget(m_add_app_cmdline = new QLineEdit, 2, 1);
		//main_ctrl_grid->addWidget(cmdline_label, 2, 2);

		{
			QWidget *separator = new QWidget;
			separator->setFixedHeight(2);
			separator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			separator->setStyleSheet(QString("background-color: #c0c0c0;"));

			main_ctrl_grid->addWidget(separator, 3, 0, 1, 3);
		}

		m_add_app_cmdline->setText("${exec} \"${left}\" \"${right}\"");
	}
}
