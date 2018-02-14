/*
 * detailgui.cpp
 *
 *  Created on: Jan 13, 2018
 *      Author: martin
 */

#include <QGridLayout>
#include <QLabel>
#include <QMimeDatabase>
#include <QFileIconProvider>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QFrame>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/exception.hpp>
#include <QScrollBar>
#include <type_traits>
#include "detailgui.h"



namespace detailgui
{
	constexpr int SIDES = 2;

	QString impl_get_mime(const boost::filesystem::path& aFilePath)
	{
		if( is_regular_file( aFilePath ) ) {
			QMimeDatabase db;
			QMimeType mime = db.mimeTypeForFile(aFilePath.string().c_str());
			return mime.name();
		}

		return QString();
	}

	void impl_show_path(fsdiff::diff_t* aDiff,
						QGridLayout* aGrid,
						fsdiff::diff_t::idx_t aIdx)
	{
		using namespace boost::filesystem;
		int row = 0;
		int col_offset = aIdx*3;

		//symbol
		{
			QFileIconProvider icon_provider;
			QIcon icon = icon_provider.icon( QFileInfo(aDiff->fullpath[aIdx].string().c_str()) );

			QLabel* icon_label = new QLabel;
			icon_label->setPixmap(icon.pixmap(80,80));

			aGrid->addWidget(icon_label, row++, 1+col_offset);

		}

		//last name
		{
			QString path = aDiff->getLastName(aIdx).string().c_str();
			QLabel* fullPathText 	= new QLabel("Name:");
			QLabel* fullPath 		= new QLabel(path);
			aGrid->addWidget(fullPathText, row++, 0+col_offset);
			aGrid->addWidget(fullPath, row-1, 1+col_offset);
		}

		//full path
		{
			QString path = aDiff->fullpath[aIdx].string().c_str();
			QLabel* fullPathText 	= new QLabel("Full Path:");
			QLabel* fullPath 		= new QLabel(path);
			fullPath->setWordWrap(true);
			aGrid->addWidget(fullPathText, row++, 0+col_offset);
			aGrid->addWidget(fullPath, row-1, 1+col_offset);
		}

		//base dir
		{
			QString path = aDiff->baseDir[aIdx].string().c_str();
			QLabel* fullPathText 	= new QLabel("Base Path:");
			QLabel* fullPath 		= new QLabel(path);
			aGrid->addWidget(fullPathText, row++, 0+col_offset);
			aGrid->addWidget(fullPath, row-1, 1+col_offset);
		}

		//mime
		QString mime_type = "";
		{
			QLabel* fullPathText 	= new QLabel("Type:");
			QLabel* fullPath 		= new QLabel(impl_get_mime(aDiff->fullpath[aIdx]));
			aGrid->addWidget(fullPathText, row++, 0+col_offset);
			aGrid->addWidget(fullPath, row-1, 1+col_offset);
		}

		//filesize
		{
			QLabel* fullPathText 	= new QLabel("Filesize:");
			QLabel* fullPath 		= new QLabel;
			boost::system::error_code err;

			auto filesize = boost::filesystem::file_size(aDiff->fullpath[aIdx], err);
			if( boost::system::errc::success ==  err.value() ) {
				if( boost::filesystem::is_regular_file( aDiff->fullpath[aIdx] ) ) {
					fullPath->setText(
							QString("%1").arg(
									filesize) );
				}
			}
			else {
				fullPath->setText("cannot determine size");
			}



			aGrid->addWidget(fullPathText, row++, 0+col_offset);
			aGrid->addWidget(fullPath, row-1, 1+col_offset);
		}

		//hash
		{
			path curr_path = aDiff->fullpath[aIdx];



			if( nullptr != aDiff->file_hashes && aDiff->file_hashes->path_hash.find(curr_path) != aDiff->file_hashes->path_hash.end() ) {

				auto result = aDiff->file_hashes->path_hash[curr_path];

				QString result_str;
				for(auto iByte: result) {
					result_str += QString("%1 ").arg((int)iByte, 2, 16, QChar('0'));
				}


				QLabel* fullPathText 	= new QLabel("Hash:");
				QLabel* fullPath 		= new QLabel(result_str);


				fullPath->setWordWrap(true);
				aGrid->addWidget(fullPathText, row++, 0+col_offset);
				aGrid->addWidget(fullPath, row-1, 1+col_offset);
			}


		}
	}

	static void impl_diffgrid_settings(QGridLayout* aGridLayout)
	{
		//set stretch
		aGridLayout->setColumnStretch(0, 0);
		aGridLayout->setColumnStretch(1, 1);
		aGridLayout->setColumnStretch(2, 0);	//for vert line
		aGridLayout->setColumnStretch(3, 0);
		aGridLayout->setColumnStretch(4, 1);

		//add vertical bar
		for(int iRow=0; iRow<aGridLayout->rowCount(); iRow++)
		{
			QFrame* vline = new QFrame;
			vline->setFrameShape(QFrame::VLine); // Replace by VLine for vertical line
			vline->setFrameShadow(QFrame::Sunken);

			vline->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
			vline->setFixedWidth(12);

			aGridLayout->addWidget(vline, iRow, 2);
		}
	}

	QWidget* show_detail(fsdiff::diff_t* aDiff)
	{
		using namespace fsdiff;

		QWidget* mainWidget = new QWidget;
		QGridLayout * gridLayout = new QGridLayout;
		mainWidget->setLayout(gridLayout);

		for(int iSide=0; iSide<SIDES; iSide++) {
			if( (cause_t::ADDED == aDiff->cause && iSide != diff_t::RIGHT)
				|| (cause_t::DELETED == aDiff->cause && iSide != diff_t::LEFT) )
			{
				QLabel* lbl = new QLabel( fsdiff::cause_t_str(aDiff->cause).c_str());
				QLabel* lblEmpty = new QLabel( fsdiff::cause_t_str(aDiff->cause).c_str());
				gridLayout->addWidget(lbl, 0, 0+3*iSide);
				gridLayout->addWidget(lbl, 0, 1+3*iSide);
			}
			else
			{
				impl_show_path(aDiff, gridLayout, static_cast<diff_t::idx_t>(iSide));
			}
		}

		impl_diffgrid_settings(gridLayout);

		return mainWidget;
	}

	static QWidget* impl_load_content(const boost::filesystem::path& aFilePath)
	{
		QWidget* content_widget = nullptr;
		QString mime_type = impl_get_mime(aFilePath);

		//open diff item
		if( mime_type.startsWith("text") )
		{
			QPlainTextEdit* plainText = new QPlainTextEdit;

			QSizePolicy policy = plainText->sizePolicy();
			policy.setVerticalStretch(2);
			plainText->setSizePolicy(policy);

			string content;

			//TODO: what type of exception thrown?
			try {
				boost::filesystem::ifstream file(aFilePath.c_str());

				while( getline(file, content) ) {
					plainText->appendPlainText(content.c_str());
				}
			}
			catch(...) {

			}

			plainText->setReadOnly(1);
			content_widget = static_cast<QWidget*>(plainText);
		}
		else if( mime_type.startsWith("image") )
		{
			QLabel* lbl = new QLabel;

			lbl->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);

			QPixmap pxmp( aFilePath.c_str() );
			pxmp =pxmp.scaledToHeight(lbl->height()/2, Qt::FastTransformation );

			lbl->setPixmap(pxmp);

			content_widget = lbl;
		}
		else
		{
			content_widget = new QLabel("cannot load content");
		}

		return content_widget;
	}

	static void impl_after_content(std::array<QWidget*, 2>& aWidgets)
	{
		//QPlainTextEdit
		{
			QPlainTextEdit* left = dynamic_cast<QPlainTextEdit*>(aWidgets[0]);
			QPlainTextEdit* right = dynamic_cast<QPlainTextEdit*>(aWidgets[1]);

			if( nullptr != left && nullptr != right) {

				auto slider_sync_fun = [](QPlainTextEdit* aLeft, QPlainTextEdit* aRight) {
					int slider_val = aLeft->verticalScrollBar()->value();
					slider_val = slider_val > aRight->verticalScrollBar()->maximum() ?
							aRight->verticalScrollBar()->maximum() : slider_val;
					aRight->verticalScrollBar()->setValue(slider_val);
				};

				QObject::connect(left->verticalScrollBar(), &QScrollBar::actionTriggered, [slider_sync_fun, left,right](){
					slider_sync_fun(left, right);
				});

				QObject::connect(right->verticalScrollBar(), &QScrollBar::actionTriggered, [slider_sync_fun, left,right](){
					slider_sync_fun(right, left );
				});

				return;
			}
		}
	}

	QWidget* show_content(fsdiff::diff_t* aDiff)
	{
		using namespace fsdiff;

		QWidget* mainWidget = new QWidget;
		QGridLayout * gridLayout = new QGridLayout;
		mainWidget->setLayout(gridLayout);
		std::array<QWidget*, 2> loaded_widgets = {nullptr, nullptr};

		for(int iSide=0; iSide<SIDES; iSide++) {
			if( (cause_t::ADDED == aDiff->cause && iSide != diff_t::RIGHT)
				|| (cause_t::DELETED == aDiff->cause && iSide != diff_t::LEFT) )
			{
				loaded_widgets[iSide] = new QLabel( fsdiff::cause_t_str(aDiff->cause).c_str());
			}
			else
			{
				loaded_widgets[iSide] = impl_load_content(aDiff->fullpath.at(iSide));
			}

			gridLayout->setColumnStretch(iSide, 1);
		}

		//add to layout
		gridLayout->addWidget(loaded_widgets[0], 0, 0);
		gridLayout->addWidget(loaded_widgets[1], 0, 1);

		 impl_after_content(loaded_widgets);


		return mainWidget;
	}
}

