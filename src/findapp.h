#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QRadioButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFrame>
#include <QDir>
#include <QRegexp>
#include <QMap>
#include <QDebug>
#include <QSettings>
#include <QDataStream>

struct app_t
{
	QString name;
	QString path;
	QString cmd_line;
	bool is_diff;

	static void register_qt_types();

	static const QString WILDCARD_EXEC;
	static const QString WILDCARD_LEFT;
	static const QString WILDCARD_RIGHT;
	static const QString WILDCARD_SINGLEFILE;
};



struct find_app_t : public app_t
{
	QRegExp search_name;
	app_t item;
};


class FindApp : public QWidget
{
	Q_OBJECT
public:
	
	static QMap <QString,FindApp::app_t> get_app_from_settings(bool aIsDiffApp);
	

public:
	FindApp(QWidget *parent=nullptr);
	~FindApp();

	void addItem();
	void addItem(app_t aApp);

private:
	void autoscan();
	void save();

private:
	QGridLayout* m_main_layout;
	QListWidget* m_app_list;

	QPushButton* m_add_app;
	QPushButton* m_autoscan_app;
	QPushButton* m_save;

	QList<find_app_t> m_search_items;
	QMap<QString, app_t> m_avail_items;
};

inline QDataStream& operator<<(QDataStream& out, const app_t& obj)
{
	out << obj.name << obj.path << obj.cmd_line << obj.is_diff;
	return out;
}

inline QDataStream& operator>>(QDataStream& in, app_t& obj)
{
	in >> obj.name >> obj.path >> obj.cmd_line >> obj.is_diff;
	return in;
}

Q_DECLARE_METATYPE(app_t)

class AppItem : public QWidget
{
	friend FindApp;

	Q_OBJECT
public:
	AppItem(QWidget *parent = nullptr);

protected:
	QVBoxLayout * m_main_layout;
	QWidget* m_app_list;
	QScrollArea* m_app_list_scroll;

	QLineEdit* m_add_app_name;
	QLineEdit* m_add_app_path;
	QLineEdit* m_add_app_cmdline;

	QPushButton* m_add_app_btn;
	QRadioButton* m_add_app_radio_diff;
	QRadioButton* m_add_app_radio_file;

	QPushButton* m_delete;
};

