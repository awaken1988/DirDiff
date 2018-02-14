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
#include "fsdiff.h"
#include "treemodel.h"
#include "sortfilterproxy.h"
#include <boost/filesystem.hpp>
#include "opengui.h"



class MainGui : public QMainWindow
{
public:
	MainGui();
	virtual ~MainGui();

public slots:
	void clicked_diffitem(const QModelIndex &index);
	void startDiff(std::vector<boost::filesystem::path> aPaths);

protected:
	void init_left_right_info();

protected:
	OpenGui* m_open;
	std::array<QGroupBox*, 2> m_cmp_detail;
	QTreeView* m_tree_view;
	TreeModel* m_model;
	SortFilterProxy* m_filter;
	QGridLayout* m_layout;
	QTabWidget* m_detail_tab;
	int m_detail_tab_idx=0;

	QVBoxLayout* m_progress_list;

	const bool m_with_filter = true;


};

#endif /* MAINGUI_H_ */
