/*
 * maingui.h
 *
 *  Created on: Jan 3, 2018
 *      Author: martin
 */

#ifndef MAINGUI_H_
#define MAINGUI_H_

#include <array>
#include <vector>
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTreeView>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include "fsdiff.h"
#include "treemodel.h"
#include "sortfilterproxy.h"
#include <boost/filesystem.hpp>
#include "opengui.h"
#include "filter.h"



class MainGui : public QMainWindow
{
public:
	MainGui();
	virtual ~MainGui();

public slots:
	void clicked_diffitem(const QModelIndex &index);
	void startDiff(shared_ptr<fsdiff::diff_t> aDiff);

protected:
	void init_left_right_info();
	void init_file_filter();
	void init_log();

	QWidget* startDiffHashes();
	QWidget* startDiffDuplicates();

	QPushButton* createFileHashBtn();
	QWidget* createFilterBtns();
protected:
	OpenGui* m_open;
	std::array<QGroupBox*, 2> m_cmp_detail;
	QTreeView* m_tree_view;
	TreeModel* m_model;
	SortFilterProxy* m_filter_proxy;
	QGridLayout* m_layout;
	QTabWidget* m_detail_tab;
	int m_detail_tab_idx=0;
	QTextEdit* m_log_text;

	QVBoxLayout* m_progress_list;

	QTabWidget* m_main_tab;

	Filter* m_filter;

	const bool m_with_filter = true;

	QLabel* m_statusbar_hash = nullptr;
};

#endif /* MAINGUI_H_ */
