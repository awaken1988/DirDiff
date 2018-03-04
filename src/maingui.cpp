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

MainGui::MainGui(  )
{

}

MainGui::~MainGui()
{

}

void MainGui::startDiff(std::vector<boost::filesystem::path> aPaths)
{
	statusBar()->showMessage("diff...");

	auto difftree = fsdiff::compare(aPaths[0], aPaths[1]);
	//difftree->createFileHashes();

	m_model = new TreeModel(this, difftree);

	if( m_with_filter ) {
		m_filter = new SortFilterProxy(this);		//TODO: who is destroying this object
		m_filter->setSourceModel(m_model);
	}

	m_layout = new QGridLayout;

	//filter and other buttons
	m_layout->addWidget(createFilterBtns(), m_layout->rowCount(), 0, 1, 2);


	//tree
	{
		m_tree_view = new QTreeView(this);
		m_tree_view->setModel( m_with_filter ? static_cast<QAbstractItemModel*>(m_filter) : static_cast<QAbstractItemModel*>(m_model));
		m_layout->addWidget(m_tree_view, m_layout->rowCount(), 0, 1, 2);

		QObject::connect(m_tree_view, &QTreeView::clicked, this, &MainGui::clicked_diffitem);

		m_tree_view->setColumnWidth(0, 300);
		m_tree_view->setAutoExpandDelay(0);
	}

	//left right box
	m_detail_tab = new QTabWidget(this);
	m_layout->addWidget(m_detail_tab, m_layout->rowCount(), 0, 1, 2);
	init_left_right_info();

	//Progres bar
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
	}

}

void MainGui::clicked_diffitem(const QModelIndex &index)
{
	using namespace fsdiff;

	QModelIndex sourceIndex = index;
	if( m_with_filter ) {
		sourceIndex = m_filter->mapToSource(index);
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
	QObject::connect(m_detail_tab, &QTabWidget::tabBarClicked, tab_changed_fun);

	//content
	const int content_tab_idx = m_detail_tab->addTab(detailgui::show_content(diff), "Content");
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

				auto duplicateModel = new DuplicateModel(nullptr, m_model->rootItem, 0);
				auto duplicateTree = new QTreeView();
				duplicateTree->setModel(duplicateModel);

				duplicateLayout->addWidget(duplicateTree, 0, 0);

				m_main_tab->addTab(duplicateWidget, "Duplicates");

				duplicateTree->expandAll();
				duplicateTree->setContextMenuPolicy(Qt::ActionsContextMenu);

				//reset main model
				m_model->refresh();

				//show in diffview
				auto diffview_action = new QAction("Show in Diffview", duplicateWidget);

				connect(diffview_action, &QAction::triggered, [duplicateModel,duplicateTree,this]() {
					auto idx = duplicateTree->currentIndex();

					auto result = duplicateModel->data(idx, Qt::DisplayRole).toString();

					cout<<result.toStdString()<<endl;

					//look if we can find the string
					{
						m_model->iterate_over_all( [this,result](QModelIndex aModelIndex) {

							auto diffPtr = static_cast<fsdiff::diff_t*>(aModelIndex.internalPointer());
							if( nullptr == diffPtr )
								return;

							if( diffPtr->fullpath[0] == result.toStdString() ) {
								std::cout<<"found: "<<diffPtr->fullpath[0].c_str()<<std::endl;

								//m_tree_view->selectionModel()->select(aModelIndex, QItemSelectionModel::ClearAndSelect);
								m_tree_view->scrollTo(aModelIndex, QAbstractItemView::PositionAtCenter);
							}
						});

						m_main_tab->setCurrentIndex(0); //TODO: make a global mapping between indices and tabs
					}

				});

				duplicateTree->addAction(diffview_action);

			}

			return;
		};
		auto step = [progress](int aMin, int aMax, int aCurr)->void {
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
			m_filter->set_cause_filter(iCause, static_cast<bool>(aState));
			cout<<"change filter: name="<<cause_t_str(iCause)<<"; state="<<aState<<endl;
		});
	}

	filter_layout->addStretch(1);

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

void MainGui::init_left_right_info()
{
		m_detail_tab->clear();
}






