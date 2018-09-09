#include <vector>
#include <boost/filesystem.hpp>
#include <QApplication>
#include <QtGui>
#include <QWidget>
#include <QTreeView>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QTimer>
#include "treemodel.h"
#include "sortfilterproxy.h"
#include "maingui.h"
#include "opengui.h"
#include "findapp.h"


auto getPathByArg(int argc, char **argv)
{
	std::vector<boost::filesystem::path> ret;

	for(int iArg=0; iArg<argc && ret.size() < 2; iArg++) {
		auto curr_path = boost::filesystem::path(argv[iArg]);

		if( boost::filesystem::is_directory(curr_path) ) {
			ret.push_back(curr_path);
		}
	}

	return ret;
}

int main(int argc, char **argv)
{
	QApplication::setSetuidAllowed(true);
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("awaken1988");
	QCoreApplication::setApplicationName("DirDiff");
	app_t::register_qt_types();


	MainGui* mainGui = nullptr;

	shared_ptr<fsdiff::diff_t> all_diff;

	QTimer::singleShot(0, [&](){

		auto diffpath = getPathByArg(argc, argv);
		bool ok = false;
		QMainWindow mainWindow;

		mainGui = new MainGui();
		mainGui->show();
		mainGui->resize(QDesktopWidget().availableGeometry(&mainWindow).size() * 0.7);

		if( diffpath.size() < 2 ) {
			OpenGui openPaths;

			if( QDialog::Accepted == openPaths.exec() ) {
				diffpath = openPaths.m_paths_str;
				all_diff = openPaths.m_diff;
				ok = true;
			}
			else {
				QApplication::exit(0);
			}
		}
		else {
			ok = true;
		}

		if( ok ) {
			mainGui->startDiff(all_diff);
			mainGui->show();

		}
	});

	return app.exec();
}

//path  left("/home/martin/Dropbox/Programming/tools_and_snippets/cpp_snippets/");
//path right("/home/martin/Dropbox/Programming/tools_and_snippets/cpp_snippets_copy/");
//path  left("/media/xc3po_root/.xc3po_snapshots/2017_07_03__20_30/");
//path right("/media/xc3po_root/.xc3po_snapshots/2018_01_26__22_37/");
