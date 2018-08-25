#include "findapp.h"

static const QString TYPE_DIFF = "diffapp";
static const QString TYPE_FILE = "fileapp";
static const QString ITEM_PATH = "path";
static const QString ITEM_CMDLINE = "cmdline";

QMap<QString,app_t> FindApp::get_app_from_settings(bool aIsDiffApp)
{
	QMap<QString,app_t> ret;
	QSettings settings;
	QString prefix = aIsDiffApp ? TYPE_DIFF : TYPE_FILE;

	for (auto iKey : settings.allKeys()) {
		if (!iKey.startsWith(prefix)) { continue; }

		QVariant read_var = settings.value(iKey);
		if (!read_var.isValid()) { continue; }

		app_t app = read_var.value<app_t>();
		ret[app.name] = app;
	}

	return ret;
}

void app_t::register_qt_types()
{
	qRegisterMetaType<app_t>("app_t");
	qRegisterMetaTypeStreamOperators<app_t>("app_t");
}

static QList<find_app_t> impl_apps_to_find()
{
	QList<find_app_t> ret;

	//WinMerge for Windows
	{
		find_app_t tmp;
		tmp.search_name = QRegExp("WinMergeU\.exe$", Qt::CaseInsensitive);
		tmp.name = "WinMerge Diff";
		tmp.cmd_line = "${exec} ${left} ${right}";
		tmp.is_diff = true;
		ret.append(tmp);
	}

	//TODO:KDiff3
	//TODO: Meld
	//TODO: Beyond Compare
	//TODO: Araxis Merge

	//TODO: HxD Hex Editor
	//TODO: Hex-Editor MX
	

	return ret;
}

FindApp::FindApp(QWidget *parent)
	: QWidget(parent)
{
	m_main_layout = new QGridLayout;
	this->setLayout(m_main_layout);

	m_main_layout->addWidget(m_app_list = new QListWidget, 0, 0, 1, 3);

	//Add AppItem
	{
		m_add_app = new QPushButton("Add");
		

		connect(m_add_app, &QPushButton::clicked, [this](bool aChecked) {
			addItem();
		});

		m_main_layout->addWidget(m_add_app, 1, 0);
	}

	//Autoscan diff apps
	{
		m_autoscan_app = new QPushButton("Autoscan");

		connect(m_autoscan_app, &QPushButton::clicked, [this](bool aChecked) {
			this->autoscan();
		});

		m_main_layout->addWidget(m_autoscan_app, 1, 1);
	}

	//Save
	{
		m_save = new QPushButton("Save");

		connect(m_save, &QPushButton::clicked, [this](bool aChecked) {
			this->save();
		});

		m_main_layout->addWidget(m_save, 1, 2);
	}

	//create empty entry
	//addItem();
	{
		find_app_t test;
		test.name = "test app name";
		test.is_diff = true;
		test.path = "C:/testapp/test.exe";
		test.cmd_line = "bla arg0 arg1";
		addItem(test);
	}
	{
		find_app_t test;
		test.name = "test app name";
		test.is_diff = false;
		test.path = "C:/testapp/test.exe";
		test.cmd_line = "bla arg0 arg1";
		addItem(test);
	}

	//fill find apps
	m_search_items = impl_apps_to_find();
}

void FindApp::addItem()
{
	addItem(app_t());
}


void FindApp::addItem(app_t aApp)
{
	auto* listWidgetItem = new QListWidgetItem(m_app_list);
	m_app_list->addItem(listWidgetItem);

	auto appItem = new AppItem;
	appItem->m_add_app_name->setText(aApp.name);
	appItem->m_add_app_path->setText(aApp.path);
	appItem->m_add_app_cmdline->setText(aApp.cmd_line);

	if (aApp.is_diff) { 
		appItem->m_add_app_radio_diff->setChecked(true);
	}
	else { 
		appItem->m_add_app_radio_file->setChecked(true);
	}

	listWidgetItem->setSizeHint(appItem->sizeHint());

	m_app_list->setItemWidget(listWidgetItem, appItem);

	return;

	

}


FindApp::~FindApp()
{
}

void FindApp::autoscan()
{
	//TODO: check if we don't look in network drives
	//TODO: get specific windows filder for programs
	//TODO: on linux search all directories in the path variable
	//auto item_to_traverse = QDir::drives();
	QList<QFileInfo> item_to_traverse = { QFileInfo( "C:\\Program Files (x86)\\" ) };

	while (!item_to_traverse.isEmpty()) {
		auto curr = item_to_traverse.last();
		item_to_traverse.removeLast();

		//qDebug() << curr.canonicalFilePath();

		//for (auto iItems : item_to_traverse) {
		//	qDebug() << iItems.absoluteFilePath();
		//}
		//qDebug() << "-------------------";

		if ( curr.isDir() ) {
			QDir curr_base_dir = QDir(curr.canonicalFilePath());
			auto newdirs = curr_base_dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
			QSet<QString> visited;

			for (auto iDir : newdirs) {
				if (curr_base_dir == QDir(iDir.canonicalFilePath())) { continue; }
				if (visited.contains(iDir.canonicalFilePath())) { continue; }
				visited.insert(iDir.canonicalFilePath());

				//qDebug() << "         " << iDir.canonicalFilePath();
				item_to_traverse.append(iDir.canonicalFilePath());
			}
		}
		else {
			for (auto& iSearchApp : m_search_items) {
				if (-1 == iSearchApp.search_name.indexIn(curr.fileName())) { continue; }

				app_t found_app = iSearchApp;
				found_app.path = curr.canonicalFilePath();

				qDebug() << "AutoscanApp: found" << found_app.name << "; path=" << found_app.path;

				addItem(found_app);
			}
			//item_to_traverse.removeLast();
		}
	}

	qDebug() << "autoscan ready";
}

void FindApp::save()
{
	
	QSettings settings;

	for (int iRound = 0; iRound < 2; iRound++) {
		for (int iItem = 0; iItem < m_app_list->count(); iItem++) {
			auto curr_item = dynamic_cast<AppItem*>(m_app_list->itemWidget(   m_app_list->item(iItem)   ));
			if (nullptr == curr_item) {
				throw "unknown list item";
			}

			//TODO: check curr_name for slashes
			app_t curr_app;
			
			curr_app.name = curr_item->m_add_app_name->text().toLower();
			curr_app.path = curr_item->m_add_app_path->text();
			curr_app.cmd_line = curr_item->m_add_app_cmdline->text();
			curr_app.is_diff = curr_item->m_add_app_radio_diff->isChecked();

			if (curr_app.name.isEmpty() || curr_app.path.isEmpty() || curr_app.cmd_line.isEmpty()) {
				continue;
			}

			if (0 == iRound) {
				//TODO: check for duplicates entries
			}
			else {
				QString type = curr_app.is_diff ? TYPE_DIFF : TYPE_FILE;
				settings.setValue(QString("%1/%2/").arg(type).arg(curr_app.name), qVariantFromValue<app_t>(curr_app));
			}
		}
	}

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
