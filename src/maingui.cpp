/*
 * maingui.cpp
 *
 *  Created on: Jan 3, 2018
 *      Author: martin
 */

#include "detailgui.h"
#include "maingui.h"
#include "opengui.h"
#include "duplicatemodel.h"
#include "logger.h"
#include <chrono>
#include <ctime>
#include <QMimeDatabase>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QApplication>
#include <QStatusBar>
#include <QAction>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>

MainGui::MainGui(  )
{

}

MainGui::~MainGui()
{

}

void MainGui::startDiff(shared_ptr<fsdiff::diff_t> aDiff)
{
	statusBar()->showMessage("diff...");

	//auto path0 = boost::filesystem::path("/home/martin/Dropbox/Programming/DirDiff/left/");
	//auto path1 = boost::filesystem::path("/home/martin/Dropbox/Programming/DirDiff/right/");
	//auto difftree = fsdiff::compare(path0, path1, [](std::string aFileName){});
	//difftree->createFileHashes();

	auto difftree = aDiff;


	m_model = new TreeModel(this, difftree);
	m_filter = new Filter();

	if( m_with_filter ) {
		m_filter_proxy = new SortFilterProxy(this);		//TODO: who is destroying this object
		m_filter_proxy->setSourceModel(m_model);
	}

	m_layout = new QGridLayout;

	//filter and other buttons
	m_layout->addWidget(createFilterBtns(), m_layout->rowCount(), 0, 1, 2);


	//tree + filter
	{
		auto* curr_lyt = new QHBoxLayout();

		m_tree_view = new QTreeView(this);
		m_tree_view->setModel( m_with_filter ? static_cast<QAbstractItemModel*>(m_filter_proxy) : static_cast<QAbstractItemModel*>(m_model));


		QObject::connect(m_tree_view, &QTreeView::clicked, this, &MainGui::clicked_diffitem);

		m_tree_view->setColumnWidth(0, 300);
		m_tree_view->setAutoExpandDelay(0);

		//filter
		auto filter_group = new QGroupBox("Filter/Regex");
		{
			init_file_filter();
			auto filter_lyt = new QHBoxLayout;
			filter_group->setLayout(filter_lyt);

			filter_lyt->addWidget(m_filter);
		}

		curr_lyt->addWidget(m_tree_view, 2);
		curr_lyt->addWidget(filter_group, 1);
		m_layout->addLayout(curr_lyt, m_layout->rowCount(), 0, 1, 2);
	}

	//left right box
	m_detail_tab = new QTabWidget(this);
	m_layout->addWidget(m_detail_tab, m_layout->rowCount(), 0, 1, 2);
	init_left_right_info();

	//log
	init_log();

	//Progress bar
	{
		m_progress_list = new QVBoxLayout();
		m_layout->addLayout(m_progress_list, m_layout->rowCount(), 0, 1, 2);
	}

	//add to widgets to QMainWindow
	{
		auto centralWidget = new QWidget();
		auto centralLayout = new QGridLayout();
		centralWidget->setLayout(centralLayout);
		setCentralWidget(centralWidget);

		//add main tab (e.g for diff and duplicate category)
		m_main_tab = new QTabWidget();
		centralLayout->addWidget(m_main_tab, 0, 0);

		//diff tab
		{
			auto diffTabContent = new QWidget();
			diffTabContent->setLayout(m_layout);
			m_main_tab->addTab(diffTabContent, "Diff");
		}

		//log tab
		m_main_tab->addTab(m_log_text, "Log");
	}

}

void MainGui::clicked_diffitem(const QModelIndex &index)
{
	using namespace fsdiff;

	QModelIndex sourceIndex = index;
	if( m_with_filter ) {
		sourceIndex = m_filter_proxy->mapToSource(index);
	}

	if( !sourceIndex.isValid() )
		return;
	diff_t* diff = static_cast<diff_t*>(sourceIndex.internalPointer());
	if( nullptr == diff )
		return;

	cout<<"clicked"<<endl;

	//recreate widgets
	init_left_right_info();


	auto tab_changed_fun = [&](int aIdx) {
		if( aIdx < 0)
			return;
		m_detail_tab_idx = aIdx;
	};

	//detail tab: filename, path, mime
	m_detail_tab->addTab(detailgui::show_detail(diff), "Details");

	//content
	m_detail_tab->addTab(detailgui::show_content(diff), "Content");

	//show duplicates
	m_detail_tab->addTab(detailgui::show_duplicates(diff), "Duplicates");

	QObject::connect(m_detail_tab, &QTabWidget::tabBarClicked, tab_changed_fun);

	//restore las tab
	m_detail_tab_idx = m_detail_tab->count() <= m_detail_tab_idx ? 0 : m_detail_tab_idx;
	m_detail_tab->setCurrentIndex(m_detail_tab_idx);
}

QPushButton* MainGui::createFileHashBtn()
{
	auto ret = new QPushButton("Compare Files");
	QObject::connect(ret, &QPushButton::clicked, [this, ret]() {

		ret->deleteLater();

		auto progress = new QProgressBar();
		m_progress_list->addWidget(progress);

		auto ready = [progress, this]()->void {
			progress->hide();

			{
				auto duplicateWidget = new QWidget();
				auto duplicateLayout = new QGridLayout();
				duplicateWidget->setLayout(duplicateLayout);
				m_main_tab->addTab(duplicateWidget, "Duplicates");

				for(auto iSide: {fsdiff::diff_t::LEFT, fsdiff::diff_t::RIGHT} ) {
					auto duplicateModel = new DuplicateModel(iSide,
							DuplicateModel::create_summary(m_model->rootItem.get(), iSide));

					auto duplicateTree = new QTreeView();
					duplicateTree->setModel(duplicateModel);

					duplicateLayout->addWidget(duplicateTree, 0, static_cast<int>(iSide));

					duplicateTree->expandAll();
					duplicateTree->setContextMenuPolicy(Qt::ActionsContextMenu);

					//reset main model
					m_model->refresh();

					//show in diffview
					auto diffview_action = new QAction("Show in Diffview", duplicateWidget);

					connect(diffview_action, &QAction::triggered, [duplicateModel,duplicateTree,this, iSide]() {
						auto idx = duplicateTree->currentIndex();

						auto result = duplicateModel->data(idx, Qt::DisplayRole).toString();

						cout<<result.toStdString()<<endl;

						//look if we can find the string
						{
							m_model->iterate_over_all( [this,result, iSide](QModelIndex aModelIndex) {

								auto diffPtr = static_cast<fsdiff::diff_t*>(aModelIndex.internalPointer());
								if( nullptr == diffPtr )
									return;

								if( diffPtr->fullpath[iSide] == result.toStdString() ) {
									std::cout<<"found: "<<diffPtr->fullpath[0].c_str()<<std::endl;

									QModelIndex map_src = m_filter_proxy->mapFromSource( aModelIndex );
									m_tree_view->expandAll(); // workaround for scrollTo
									m_tree_view->selectionModel()->select(map_src, QItemSelectionModel::ClearAndSelect);
									m_tree_view->scrollTo(map_src, QAbstractItemView::PositionAtCenter);

									//m_tree_view->selectionModel()->select(aModelIndex, QItemSelectionModel::ClearAndSelect);
									//m_tree_view->scrollTo(aModelIndex, QAbstractItemView::PositionAtCenter);
								}
							});

							m_main_tab->setCurrentIndex(0); //TODO: make a global mapping between indices and tabs
						}

					});

					duplicateTree->addAction(diffview_action);

				}


			}

			return;
		};

		auto last_time = std::chrono::steady_clock::now();
		auto step = [progress,last_time,this](int aMin, int aMax, int aCurr) mutable -> void {

			auto curr_time = std::chrono::steady_clock::now();
			auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(last_time - curr_time);

			if( delta_time.count() < 1000 ) {
				return;
			}

			last_time = std::chrono::steady_clock::now();


			progress->setMinimum(aMin);
			progress->setMaximum(aMax);
			progress->setValue(aCurr);

			progress->setTextVisible(true);
			progress->setFormat("Hash files");
			return;
		};
		m_model->startFileHash( ready, step );
	});

	return ret;
}

QWidget* MainGui::createFilterBtns()
{
	auto ret = new QWidget();
	QHBoxLayout* filter_layout = new QHBoxLayout;
	ret->setLayout(filter_layout);

	for(auto iCause: fsdiff::cause_t_list()) {
		auto* chBx = new QCheckBox( fsdiff::cause_t_str(iCause).c_str(), this );
		chBx->setCheckState(Qt::Checked);
		filter_layout->addWidget(chBx);

		QObject::connect(chBx, &QCheckBox::stateChanged, [iCause, this](int aState) {
			m_filter_proxy->set_cause_filter(iCause, static_cast<bool>(aState));
			cout<<"change filter: name="<<cause_t_str(iCause)<<"; state="<<aState<<endl;
		});
	}

	filter_layout->addStretch(1);

	//Size units
	{
		auto* size_unit_vbox = new QVBoxLayout();
		auto* size_unit_widget = new QWidget();
		auto* cmbo_box = new QComboBox();
		size_unit_widget->setLayout(size_unit_vbox);

		size_unit_vbox->addWidget(new QLabel("Size Unit: "));
		size_unit_vbox->addWidget(cmbo_box);

		for(auto iUnit: fsdiff::pretty_print_styles) {
			cmbo_box->addItem(iUnit.c_str());
		}

		connect(cmbo_box, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), [this](const QString &text) -> void {
			m_model->setSizeUnit(text);
		});


		filter_layout->addWidget( size_unit_widget );
	}

	//Create File Hashes
	{
		filter_layout->addWidget( createFileHashBtn() );
	};

	//collapse all
	{
		auto ret = new QPushButton("Collapse All");
		QObject::connect(ret, &QPushButton::clicked, [this]() {
			m_tree_view->collapseAll();
		});
		filter_layout->addWidget(ret);
	};

	//expand all
	{
		auto ret = new QPushButton("Expand All");
		QObject::connect(ret, &QPushButton::clicked, [this]() {
			m_tree_view->expandAll();
		});
		filter_layout->addWidget(ret);
	};

	return ret;
}

void MainGui::init_log()
{
	m_log_text = new QTextEdit();
	m_log_text->setReadOnly(true);

	Logger::Instance()->attach(
			[this](const log_item_t& aItem) -> void {

		 	 	 std::time_t now_c = std::chrono::system_clock::to_time_t( aItem.time );

				m_log_text->insertHtml(QString("<span style=\"background-color: #%4; font-family: courier;\"> <b>%3</b> <i>%1:</i> %2 </span> <br />")
				.arg( std::ctime(&now_c), -24, QChar('_') )
				.arg(aItem.msg.c_str())
				.arg(log_level_t_str(aItem.level).c_str(), -16, QChar('_') )
				.arg(log_level_t_color(aItem.level), 8, 16, QChar('0')));
	});

	LoggerInfo("Attach Gui Log");
	LoggerWarning("Attach Gui Log");
	LoggerError("Attach Gui Log");
}

void MainGui::init_left_right_info()
{
	m_detail_tab->clear();
}

void MainGui::init_file_filter()
{
	connect(&m_filter->getModel(), &FilterModel::changed_filter, [this]() ->void {
		m_filter_proxy->setExpressions(m_filter->getModel().getExpressions());
	});
}





