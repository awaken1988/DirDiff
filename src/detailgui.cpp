/*
 * detailgui.cpp
 *
 *  Created on: Jan 13, 2018
 *      Author: martin
 */

#include <type_traits>
#include <array>
#include <sstream>
#include <string>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/exception.hpp>
#include <QGridLayout>
#include <QLabel>
#include <QMimeDatabase>
#include <QFileIconProvider>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QFrame>
#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>
#include <QClipboard>
#include <QApplication>
#include <QScrollBar>
#include <QTreeView>
#include <QLabel>
#include <QTextStream>
#include "detailgui.h"
#include "duplicatemodel.h"
#include "dtl/dtl/dtl.hpp"
#include "dtl/dtl/variables.hpp"



namespace detailgui
{
	constexpr int SIDES = 2;

	using dtl::Diff;
	using dtl::elemInfo;
	using dtl::uniHunk;

	typedef string                 elem;
	typedef vector< elem >         sequence;
	typedef pair< elem, elemInfo > sesElem;

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

		//header
		{
			auto header_layout = new QHBoxLayout();
			const auto full_path = aDiff->fullpath[aIdx];

			//symbol
			{
				QFileIconProvider icon_provider;
				QIcon icon = icon_provider.icon( QFileInfo(aDiff->fullpath[aIdx].string().c_str()) );

				QLabel* icon_label = new QLabel;
				icon_label->setPixmap(icon.pixmap(80,80));

				header_layout->addWidget(icon_label);
			}
			//header_layout->setSpacing(1);

			//open file
			if( boost::filesystem::is_regular_file( aDiff->fullpath[aIdx] ) )
			{

				auto btnExplorer = new QPushButton("Open File");
				btnExplorer->setSizePolicy(QSizePolicy::Fixed , QSizePolicy::Fixed);

				QObject::connect(btnExplorer, &QPushButton::clicked, [full_path](bool aChecked) {

					QDesktopServices::openUrl(QUrl(full_path.generic_string().c_str()));

				});

				header_layout->addWidget(btnExplorer);
			}

			//open directory
			{
				auto btnExplorer = new QPushButton("Open Directory");
				btnExplorer->setSizePolicy(QSizePolicy::Fixed , QSizePolicy::Fixed);


				QObject::connect(btnExplorer, &QPushButton::clicked, [full_path](bool aChecked) {

					if( is_regular_file(full_path) ) {
						QDesktopServices::openUrl(QUrl(full_path.parent_path().generic_string().c_str()));
					}
					else {
						QDesktopServices::openUrl(QUrl(full_path.generic_string().c_str()));
					}

				});

				header_layout->addWidget(btnExplorer);
			}

			//copy full path
			{
				auto btnExplorer = new QPushButton("Path to Clipboard");
				btnExplorer->setSizePolicy(QSizePolicy::Fixed , QSizePolicy::Fixed);
				QObject::connect(btnExplorer, &QPushButton::clicked, [full_path](bool aChecked) {

					QClipboard *clipboard = QApplication::clipboard();
					QString originalText = clipboard->text();
					clipboard->setText(full_path.generic_string().c_str());
				});

				header_layout->addWidget(btnExplorer);
			}

			aGrid->addLayout(header_layout, row++, 1+col_offset, 1 , 1);
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

			QPixmap pxmp( aFilePath.string().c_str() );
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

	template<typename TTextWidget>
	static void impl_after_content(std::array<QWidget*, 2>& aWidgets)
	{
		//QPlainTextEdit
		{
			TTextWidget* left = dynamic_cast<TTextWidget*>(aWidgets[0]);
			TTextWidget* right = dynamic_cast<TTextWidget*>(aWidgets[1]);

			if( nullptr != left && nullptr != right) {

				auto slider_sync_fun = [](TTextWidget* aLeft, TTextWidget* aRight) {
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

		 impl_after_content<QPlainTextEdit>(loaded_widgets);


		return mainWidget;
	}

	std::string impl_show_diff_colorize(dtl::edit_t aEdit)
	{
		std::string color = "0xFFFFFFFF";

		switch(aEdit) {
			case dtl::SES_DELETE: {
				color = "#fc6f4b";
			} break;
			case dtl::SES_COMMON: {
				color = "#ffffff";
			} break;
			case dtl::SES_ADD: {
				color = "#7bfc67";

			} break;
		}

		return color;
	}

	std::vector<string> impl_show_diff(fsdiff::diff_t* aDiff)
	{
		using namespace fsdiff;


		//open files
		std::array<sequence, 2> diff_side;

		for(int iSide=0; iSide<SIDES; iSide++) {
			QFile file( aDiff->fullpath[iSide].c_str() );
			if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
				continue;
			}

			QTextStream fileStream(&file);
			QString line;
			while( fileStream.readLineInto(&line) ) {
				diff_side[iSide].push_back(line.toStdString());
			}
		}

		//diff files
		Diff< elem > diff(diff_side[0], diff_side[1]);
		diff.onHuge();
		diff.compose();
		diff.composeUnifiedHunks();
		auto uni_hunks = diff.getUniHunks();

		auto ret = std::vector<string>{std::string(""), std::string("") };


		for(int iSide=0; iSide<2; iSide++) {
			const auto orig_text = diff_side[iSide];
			int last_unihunk_idx = uni_hunks.size()-1;

			for(int iTxt=orig_text.size()-1; iTxt>=0;) {
				auto& curr_hunk = uni_hunks[last_unihunk_idx];
				int hunk_start_line = iSide == 0 ? curr_hunk.a : curr_hunk.c;
				int hunk_end_line = hunk_start_line + (iSide == 0 ? curr_hunk.b : curr_hunk.d);
				if( hunk_end_line-1 == iTxt) {
					auto& changes = curr_hunk.change;

					for(int iChange=changes.size()-1; iChange>=0; iChange--) {
						auto& curr_change = changes[iChange];
						auto color = impl_show_diff_colorize(curr_change.second.type);

						std::string curr_text = curr_change.first;
						switch(curr_change.second.type)
						{
							case dtl::SES_DELETE: {
								if( iSide == 1 ) {
									curr_text = "";
								}
							} break;
							case dtl::SES_COMMON: {

							} break;
							case dtl::SES_ADD: {
								if( iSide == 0 ) {
									curr_text = "";
								}

							} break;
						}

						if( curr_text.size() < 1 ) {
							curr_text = "...";
						}

						ret[iSide] = "<div style=\"background-color: "+color+";\">" + curr_text + "</div>"+ ret[iSide];
					}

					iTxt -= hunk_end_line - hunk_start_line;
					if( last_unihunk_idx > 0 ) {
						last_unihunk_idx--;
					}
				}
				else {
					std::string curr_text = orig_text[iTxt];

					if( curr_text.size() < 1 ) {
						curr_text = "...";
					}

					ret[iSide] = "<div style=\"background-color: white;\">" + curr_text + "</div>"+ ret[iSide];
					iTxt--;
				}


			}
		}



//		for(auto iHunk: diff.getUniHunks()) {
//
//			for(int iPost=0; iPost<2; iPost++) {
//				for(auto iChange: iHunk.common[iPost]) {
//						ret[iPost] += iChange.first + "<br/>";
//				}
//			}
//
//			for(auto iChange: iHunk.change) {
//					ret[0] += iChange.first + ":"+ std::to_string(iChange.second.type) + "<br/>";
//					ret[1] += iChange.first + ":"+ std::to_string(iChange.second.type) + "<br/>";
//			}
//
//
//			ret[0] += "---<br/>";
//			ret[1] += "---<br/>";
//		}

		return ret;
	}


	QWidget* show_diff(fsdiff::diff_t* aDiff)
	{
		using namespace fsdiff;

		QWidget* mainWidget = new QWidget;
		QGridLayout * gridLayout = new QGridLayout;
		mainWidget->setLayout(gridLayout);
		std::array<QWidget*, 2> loaded_widgets = {nullptr, nullptr};

		auto results = impl_show_diff(aDiff);

		for(int iSide=0; iSide<SIDES; iSide++) {
			if( (cause_t::ADDED == aDiff->cause && iSide != diff_t::RIGHT)
				|| (cause_t::DELETED == aDiff->cause && iSide != diff_t::LEFT) )
			{
				loaded_widgets[iSide] = new QLabel( fsdiff::cause_t_str(aDiff->cause).c_str());
			}
			else
			{
				auto content = new QTextEdit();

				content->append( results[iSide].c_str() );

				loaded_widgets[iSide] = content;
			}

			gridLayout->setColumnStretch(iSide, 1);
		}

		//add to layout
		gridLayout->addWidget(loaded_widgets[0], 0, 0);
		gridLayout->addWidget(loaded_widgets[1], 0, 1);

		impl_after_content<QTextEdit>(loaded_widgets);


		return mainWidget;
	}

	QWidget* show_duplicates(fsdiff::diff_t* aDiff)
	{
		QWidget* mainWidget = new QWidget;
		QGridLayout * gridLayout = new QGridLayout;
		mainWidget->setLayout(gridLayout);

		if( nullptr == aDiff->file_hashes) {
			QLabel* lblHint = new QLabel("press \"Compare Files\" to find duplicates");
			gridLayout->addWidget(lblHint, 0, 0);
			return mainWidget;
		}

		for(int iSide=0; iSide<SIDES; iSide++) {
			auto iSideEnum = static_cast<fsdiff::diff_t::idx_t>(iSide);
			QTreeView* dup_view = new QTreeView();

			if( is_regular_file(aDiff->fullpath[iSide]) ) {
				DuplicateModel* dup_model = new DuplicateModel(iSideEnum, DuplicateModel::create_onefile(aDiff, iSideEnum));
				dup_view->setModel(dup_model);
				dup_view->expandAll();
			}

			gridLayout->addWidget(dup_view, 0, iSide);
		}

		return mainWidget;
	}
}

