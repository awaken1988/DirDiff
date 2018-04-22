/*
 * opengui.cpp
 *
 *  Created on: Feb 8, 2018
 *      Author: martin
 */

#include "opengui.h"

#include <iostream>
#include <chrono>
#include <QtGui>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStyle>
#include <QMainWindow>
#include <QDesktopWidget>
#include <QFileDialog>
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

	m_main_layout = new QGridLayout;

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

		m_main_layout->addWidget(lblChoose, iSide, 0);
		m_main_layout->addWidget(m_paths[iSide], iSide, 1);
		m_main_layout->addWidget(bntDialog, iSide, 2);
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
			FileLoadWalker* worker = new FileLoadWalker(m_paths_str, m_filter->getModel().getExpressions());

			worker->moveToThread(thread);
			connect(this, &OpenGui::operateFilehash, worker, &FileLoadWalker::hashAllFiles);
			connect(worker, &FileLoadWalker::resultReady, this, &OpenGui::recListFilesReady, Qt::BlockingQueuedConnection);
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


	m_main_layout->addLayout(btnLayout, m_main_layout->rowCount(), 0, 1, 3);
	m_main_layout->addWidget(m_load_progress, m_main_layout->rowCount(), 0, 1, 3);
	m_main_layout->addWidget(m_status, m_main_layout->rowCount(), 0, 1, 3);
	setLayout(m_main_layout);

	init_filter();
}

void OpenGui::init_filter()
{
	m_filter = new Filter();

	m_main_layout->addWidget(m_filter, m_main_layout->rowCount(), 0, 1, 3);

	//some predefined filter
	//later we could move this to a config file
	m_filter->addExpression(".git", true);
	m_filter->addExpression(".svn", true);
}

void OpenGui::recListFilesReady(shared_ptr<fsdiff::diff_t> aDiff)
{
	m_diff = aDiff;
	emit accept();
}

OpenGui::~OpenGui()
{

}

void OpenGui::stepLoad(std::string aFileName)
{
	m_status->setText(QString("%1").arg(aFileName.c_str()));
}

FileLoadWalker::FileLoadWalker(std::vector<boost::filesystem::path> aPaths, std::vector<fsdiff::filter_item_t> aFilter)
	: m_paths(aPaths), m_filter(aFilter)
{

}

FileLoadWalker::~FileLoadWalker()
{
	std::cout<<"FilehashWorker deleted";
}

void FileLoadWalker::hashAllFiles()
{
	auto last_time = std::chrono::steady_clock::now();
	auto difftree = fsdiff::compare(m_paths[0], m_paths[1], [this, last_time](std::string aFileName) mutable {
		auto curr_time = std::chrono::steady_clock::now();
		auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>( curr_time - last_time );
		auto delta_time_count = delta_time.count();

		if( delta_time_count < 2000 ) {
			return;
		}

		last_time = std::chrono::steady_clock::now();

		emit stepReady(aFileName);
	}, m_filter);

	emit resultReady(difftree);
}

