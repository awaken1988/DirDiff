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

	QGridLayout* layout = new QGridLayout;
	m_layout = layout;

	//filter
	{
		QHBoxLayout* filter_layout = new QHBoxLayout;

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
			auto ret = new QPushButton("Compare Files");
			QObject::connect(ret, &QPushButton::clicked, [this]() {

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
			filter_layout->addWidget(ret);
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



		layout->addLayout(filter_layout, layout->rowCount(), 0, 1, 2);
	}

	//tree
	{
		m_tree_view = new QTreeView(this);
		m_tree_view->setModel( m_with_filter ? static_cast<QAbstractItemModel*>(m_filter) : static_cast<QAbstractItemModel*>(m_model));
		layout->addWidget(m_tree_view, layout->rowCount(), 0, 1, 2);

		QObject::connect(m_tree_view, &QTreeView::clicked, this, &MainGui::clicked_diffitem);

		m_tree_view->setColumnWidth(0, 300);
	}

	//left right box
	m_detail_tab = new QTabWidget(this);
	layout->addWidget(m_detail_tab, layout->rowCount(), 0, 1, 2);
	init_left_right_info();

	//Progres bar
	{
		m_progress_list = new QVBoxLayout();
		layout->addLayout(m_progress_list, layout->rowCount(), 0, 1, 2);
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
			diffTabContent->setLayout(layout);
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

void MainGui::init_left_right_info()
{
		m_detail_tab->clear();
}






