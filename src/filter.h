#ifndef filter_h
#define filter_h

#include <QWidget>
#include <QLineEdit>
#include <QAbstractTableModel>
#include <QTableView>
#include <QPushButton>
#include <QShortcut>
#include <vector>

class FilterModel;

class Filter : public QWidget
{
	Q_OBJECT

public:
	Filter(QWidget* parent = nullptr);
	virtual ~Filter();

protected:
	QLineEdit* m_search_input;
	QTableView* m_table;
	FilterModel* m_model;

	QPushButton* m_btn_include;
	QPushButton* m_btn_exclude;

	QShortcut* m_shortcut;

	void addExpression(QString aExpression, bool aExclude);

};

class FilterModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	struct filter_item_t
	{
		QString regex;
		bool	exclude;	//true=(filter out items match the regex);	false=(display items matches regex)
	};

protected:
	std::vector<filter_item_t> m_expressions;

public:
	FilterModel();
	virtual ~FilterModel();



	int rowCount(const QModelIndex & parent = QModelIndex()) const;
	int columnCount(const QModelIndex & parent = QModelIndex()) const;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
	Qt::ItemFlags flags(const QModelIndex & index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	void appendData(const filter_item_t& aData);
	void deleteData(int aStartRow, int aEndRow);

	//TODO: move implementation to cpp file
	decltype(m_expressions)::const_iterator begin()
	{
		return m_expressions.cbegin();
	}

	decltype(m_expressions)::const_iterator end()
	{
		return m_expressions.cend();
	}



};

#endif /* filter_h */
