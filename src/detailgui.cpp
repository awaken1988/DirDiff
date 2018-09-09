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
#include <QGroupBox>
#include <QProcess>
#include "detailgui.h"
#include "duplicatemodel.h"
#include "dtl/dtl/dtl.hpp"
#include "dtl/dtl/variables.hpp"
#include "findapp.h"
#include "logger.h"



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
						fsdiff::diff_t::idx_t aIdx,
						int aRow,
						int aCol)
	{
		using namespace boost::filesystem;

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

			aGrid->addLayout(header_layout, aRow, aCol, 1 , 1);
			aRow++;
		}



		QGridLayout* text_info_table = new QGridLayout;

		//last name
		{
			QString path = aDiff->getLastName(aIdx).string().c_str();
			QLabel* fullPathText 	= new QLabel("Name:");
			QLabel* fullPath 		= new QLabel(path);
			text_info_table->addWidget(fullPathText, 0, 0);
			text_info_table->addWidget(fullPath, 0, 1, 1, 2);
		}

		//full path
		{
			QString path = aDiff->fullpath[aIdx].string().c_str();
			QLabel* fullPathText 	= new QLabel("Full Path:");
			QLabel* fullPath 		= new QLabel(path);
			fullPath->setWordWrap(true);
			text_info_table->addWidget(fullPathText, 1, 0);
			text_info_table->addWidget(fullPath, 1, 1, 1, 2);
		}

		//base dir
		{
			QString path = aDiff->baseDir[aIdx].string().c_str();
			QLabel* fullPathText 	= new QLabel("Base Path:");
			QLabel* fullPath 		= new QLabel(path);
			text_info_table->addWidget(fullPathText, 2, 0);
			text_info_table->addWidget(fullPath, 2, 1, 1, 2);
		}

		//mime
		QString mime_type = "";
		{
			QLabel* fullPathText 	= new QLabel("Type:");
			QLabel* fullPath 		= new QLabel(impl_get_mime(aDiff->fullpath[aIdx]));
			text_info_table->addWidget(fullPathText, 3, 0);
			text_info_table->addWidget(fullPath, 3, 1, 1, 2);
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



			text_info_table->addWidget(fullPathText, 4, 0);
			text_info_table->addWidget(fullPath, 4, 1, 1, 2);
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
				text_info_table->addWidget(fullPathText, 5, 0);
				text_info_table->addWidget(fullPath, 5, 1, 1, 2);
			}

			aGrid->addLayout(text_info_table, aRow++, aCol);
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
		int row = 0;

		//show diff tools
		if( cause_t::CONTENT == aDiff->cause || cause_t::SAME == aDiff->cause)
		{
			auto diff_apps = FindApp::get_app_from_settings(true);

			auto diff_app_widget = new QGroupBox;
			auto diff_app_lyt = new QHBoxLayout;
			diff_app_widget->setLayout(diff_app_lyt);

			for (auto iKey : diff_apps.keys()) {
				const app_t& curr_app = diff_apps[iKey];

				auto app_btn = new QPushButton(curr_app.name);

				QObject::connect(app_btn, &QPushButton::clicked, [curr_app, aDiff]() {
					QString cmd_line = curr_app.cmd_line;
					cmd_line.replace(app_t::WILDCARD_EXEC, QString("\"%1\"").arg(curr_app.path));
					cmd_line.replace(app_t::WILDCARD_LEFT, QString("\"%1\"").arg(aDiff->fullpath[0].string().c_str()));
					cmd_line.replace(app_t::WILDCARD_RIGHT, QString("\"%1\"").arg(aDiff->fullpath[1].string().c_str()));

					LoggerInfo((QString("Executing ") + cmd_line).toStdString() );

					QProcess process;
					//process.setProgram("C:\\Program Files (x86)\\WinMerge\\WinMergeU.exe \"C:\\Program Files (x86)\\WinMerge\\Files.txt\" \"C:\\Program Files (x86)\\WinMerge\\Contributors.txt\"");
					process.setProgram(cmd_line);
					process.startDetached();		//FIXME: not working on linux
				});

				diff_app_widget->setTitle("Compare Tools");
				diff_app_lyt->addWidget(app_btn);
			}

			gridLayout->addWidget(diff_app_widget, row, 0, 1, 2, Qt::AlignHCenter);
		}

		for(int iSide=0; iSide<SIDES; iSide++) {
			if( (cause_t::ADDED == aDiff->cause && iSide != diff_t::RIGHT)
				|| (cause_t::DELETED == aDiff->cause && iSide != diff_t::LEFT) )
			{
				QLabel* lbl = new QLabel( fsdiff::cause_t_str(aDiff->cause).c_str());
				QLabel* lblEmpty = new QLabel( fsdiff::cause_t_str(aDiff->cause).c_str());
				gridLayout->addWidget(lbl, row, iSide);
				gridLayout->addWidget(lbl, row, iSide);
			}
			else
			{
				impl_show_path(aDiff, gridLayout, static_cast<diff_t::idx_t>(iSide), 1, iSide);
			}
		}

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

	static int slider_print_count = 0;

	template<typename TTextWidget>
	static void impl_after_content(std::array<QWidget*, 2>& aWidgets)
	{
		//QPlainTextEdit
		{
			TTextWidget* left = dynamic_cast<TTextWidget*>(aWidgets[0]);
			TTextWidget* right = dynamic_cast<TTextWidget*>(aWidgets[1]);

			if( nullptr != left && nullptr != right) {
				auto slider_sync_fun = [](TTextWidget* aLeft, TTextWidget* aRight) {
					auto left_value = aLeft->verticalScrollBar()->value();

					int slider_val = left_value;
					slider_val = slider_val > aRight->verticalScrollBar()->maximum() ?
							aRight->verticalScrollBar()->maximum() : slider_val;
					aRight->verticalScrollBar()->setValue(slider_val);

					std::cout<<"slider_val="<<slider_val<<std::endl;
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
			QFile file( aDiff->fullpath[iSide].string().c_str() );
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

			//simply prepend a line to the output -> if we are not in diff mode
			auto add_normal_text = [&orig_text, &ret, &iSide](int aIndexTxt){
				std::string curr_text = orig_text[aIndexTxt];

				if( curr_text.size() < 1 ) {
					curr_text = "...";
				}

				ret[iSide] = "<div style=\"background-color: white;\">" + curr_text + "</div>"+ ret[iSide];
			};

			for(int iTxt=orig_text.size()-1; iTxt>=0;) {
				//special case: there are no differences
				if( last_unihunk_idx < 0 ) {
					add_normal_text(iTxt--);
					continue;
				}

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
					add_normal_text(iTxt--);
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

