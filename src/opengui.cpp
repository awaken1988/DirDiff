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
#include <iostream>
#include "fsdiff.h"

OpenGui::OpenGui(QWidget *parent)
	: QDialog(parent)
{
	//resize
	{
		QMainWindow mainWindow;
		auto desksize = QDesktopWidget().availableGeometry(&mainWindow).size() * 0.7;
		this->resize(desksize.width(), 0);
	}

	//progressbar
	m_load_progress = new QProgressBar();
	m_load_progress->setVisible(false);

	//progress label
	m_status = new QLabel("...");

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

	//start collecting files btn
	auto btnOk = new QPushButton("Ok", this);
	btnLayout->addWidget(btnOk);

	QObject::connect(btnOk, &QPushButton::clicked, [this](bool a) {
		m_paths_str.clear();
		for(int iSide=0; iSide<2; iSide++) {
			m_paths_str.push_back( boost::filesystem::path( m_paths[iSide]->text().toStdString() ) );
		}

		m_load_progress->setVisible(true);
		m_load_progress->setTextVisible(true);
		m_load_progress->setMinimum(0);
		m_load_progress->setMaximum(0);

		//start thread
		{
			QThread* thread = new QThread;
			FileLoadWalker* worker = new FileLoadWalker(m_paths_str);

			worker->moveToThread(thread);
			connect(this, &OpenGui::operateFilehash, worker, &FileLoadWalker::hashAllFiles);
			connect(worker, &FileLoadWalker::resultReady, [this](shared_ptr<fsdiff::diff_t> aDiff){
				m_diff = aDiff;
				emit accept();
			});
			connect(worker, &FileLoadWalker::stepReady, this, &OpenGui::stepLoad, Qt::BlockingQueuedConnection );

			connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
			connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
			connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));


			thread->start();
			emit operateFilehash();
		}

		//emit accept();
	});

	//close program btn
	auto btnCancel = new QPushButton("Cancel", this);
	btnLayout->addWidget(btnCancel);
	QObject::connect(btnCancel, &QPushButton::clicked, [this](bool a) {
		emit cancelClicked();
		emit reject();
	});
	btnLayout->addStretch(1);


	mainLayout->addLayout(btnLayout, mainLayout->rowCount(), 0, 1, 3);
	mainLayout->addWidget(m_load_progress, mainLayout->rowCount(), 0, 1, 3);
	mainLayout->addWidget(m_status, mainLayout->rowCount(), 0, 1, 3);
	setLayout(mainLayout);
}

OpenGui::~OpenGui()
{

}

void OpenGui::stepLoad(std::string aFileName)
{
	m_status->setText(QString("%1").arg(aFileName.c_str()));
}

FileLoadWalker::FileLoadWalker(std::vector<boost::filesystem::path> aPaths)
	: m_paths(aPaths)
{

}

FileLoadWalker::~FileLoadWalker()
{
	std::cout<<"FilehashWorker deleted";
}

void FileLoadWalker::hashAllFiles()
{
	auto difftree = fsdiff::compare(m_paths[0], m_paths[1], [this](std::string aFileName){
		emit stepReady(aFileName);
	});

	emit resultReady(difftree);
}

