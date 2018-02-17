/*
 * opengui.cpp
 *
 *  Created on: Feb 8, 2018
 *      Author: martin
 */

#include "opengui.h"

#include <QtGui>
#include <QLineEdit>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStyle>
#include <QMainWindow>
#include <QDesktopWidget>
#include <QFileDialog>

OpenGui::OpenGui(QWidget *parent)
	: QDialog(parent)
{
	//resize
	{
		QMainWindow mainWindow;
		auto desksize = QDesktopWidget().availableGeometry(&mainWindow).size() * 0.7;
		this->resize(desksize.width(), 0);
	}


	auto mainLayout = new QGridLayout;

	for(int iSide=0; iSide<2; iSide++) {
		auto text = 0 == iSide ? "Left: " : "Right: ";

		auto lblChoose 	= new QLabel(text, this);
		m_paths[iSide]	= new QLineEdit(this);
		auto bntDialog	= new QPushButton(this->style()->standardIcon(QStyle::SP_DialogOpenButton), "Open", this);

		if( 0 == iSide ) {
			m_paths[iSide]->setText("/home/martin/Dropbox/Programming/DirDiff/left/");
		}
		else {
			m_paths[iSide]->setText("/home/martin/Dropbox/Programming/DirDiff/right/");
		}

		decltype(m_paths[iSide]) curr_paths = m_paths[iSide];

		//show directory dialog
		QObject::connect(bntDialog, &QPushButton::clicked, [this,curr_paths](int a) {
			QFileDialog dialog(this);
			dialog.setFileMode(QFileDialog::DirectoryOnly);
			if( dialog.exec() ) {
				curr_paths->setText(dialog.selectedFiles()[0]);
			}
		});

		mainLayout->addWidget(lblChoose, iSide, 0);
		mainLayout->addWidget(m_paths[iSide], iSide, 1);
		mainLayout->addWidget(bntDialog, iSide, 2);
	}

	auto btnLayout = new QHBoxLayout;
	btnLayout->addStretch(1);
	{
		auto btnOk = new QPushButton("Ok", this);
		btnLayout->addWidget(btnOk);
		QObject::connect(btnOk, &QPushButton::clicked, [this](bool a) {
			for(int iSide=0; iSide<2; iSide++) {
				m_paths_str.push_back( boost::filesystem::path( m_paths[iSide]->text().toStdString() ) );
			}

			emit okClicked(m_paths_str);
			emit accept();
		});

		auto btnCancel = new QPushButton("Cancel", this);
		btnLayout->addWidget(btnCancel);
		QObject::connect(btnCancel, &QPushButton::clicked, [this](bool a) {
			emit cancelClicked();
			emit reject();
		});
		btnLayout->addStretch(1);
	}

	mainLayout->addLayout(btnLayout, mainLayout->rowCount(), 0, 1, 3);
	setLayout(mainLayout);
}

OpenGui::~OpenGui()
{

}

