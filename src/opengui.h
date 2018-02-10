/*
 * opengui.h
 *
 *  Created on: Feb 8, 2018
 *      Author: martin
 */

#ifndef OPENGUI_H_
#define OPENGUI_H_

#include <QDialog>
#include <boost/filesystem.hpp>
#include <vector>

class QLineEdit;

class OpenGui : public QDialog
{
	  Q_OBJECT
public:
	OpenGui(QWidget *parent = 0);
	virtual ~OpenGui();

	std::vector<boost::filesystem::path> m_paths_str;
protected:
	QLineEdit* m_paths[2];


signals:
	void okClicked( std::vector<boost::filesystem::path> aPaths );
	void cancelClicked();
};

#endif /* OPENGUI_H_ */
