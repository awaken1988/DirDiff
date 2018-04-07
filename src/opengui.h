/*
 * opengui.h
 *
 *  Created on: Feb 8, 2018
 *      Author: martin
 */

#ifndef OPENGUI_H_
#define OPENGUI_H_

#include <QDialog>
#include <QProgressBar>
#include <QThread>
#include <QLabel>
#include <QGridLayout>
#include <boost/filesystem.hpp>
#include <vector>
#include <string>
#include <memory>
#include "fsdiff.h"
#include "filter.h"

class QLineEdit;

class OpenGui : public QDialog
{
	  Q_OBJECT
public:
	OpenGui(QWidget *parent = 0);
	virtual ~OpenGui();

	std::vector<boost::filesystem::path> m_paths_str;
	shared_ptr<fsdiff::diff_t> m_diff;
protected:
	QLineEdit* m_paths[2];
	QProgressBar* m_load_progress;
	QLabel* m_status;
	Filter* m_filter;
	QGridLayout* m_main_layout;

	void init_filter();

signals:
	void cancelClicked();
	void operateFilehash();

public slots:
	void stepLoad(std::string aFileName);
	void recListFilesReady(shared_ptr<fsdiff::diff_t> aDiff);
};

class FileLoadWalker : public QObject
{
	Q_OBJECT

public:
	FileLoadWalker(std::vector<boost::filesystem::path> aPaths);
	virtual ~FileLoadWalker();
public slots:
	void hashAllFiles();
signals:
	void resultReady(shared_ptr<fsdiff::diff_t> aDiff);
	void stepReady(std::string aFileName);

private:
	std::vector<boost::filesystem::path> m_paths;

};

#endif /* OPENGUI_H_ */
