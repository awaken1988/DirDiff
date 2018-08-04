#ifndef filter_h
#define filter_h

#include <QWidget>
#include <QLineEdit>
#include <QAbstractTableModel>
#include <QTableView>
#include <QPushButton>
#include <QShortcut>
#include <vector>
#include "fsdiff.h"

class FilterModel;

class Filter : public QWidget
{
	Q_OBJECT

public:
	Filter(QWidget* parent = nullptr);
	virtual ~Filter();

	void addExpression(QString aExpression, bool aExclude);

	FilterModel& getModel();

protected:
	QLineEdit* m_search_input;
	QTableView* m_table;
	FilterModel* m_model;

	QPushButton* m_btn_include;
	QPushButton* m_btn_exclude;
	QPushButton* m_btn_clear;

	QShortcut* m_shortcut;


};

class FilterModel : public QAbstractTableModel
{
	Q_OBJECT
public:

protected:
	std::vector<fsdiff::filter_item_t> m_expressions;

signals:
	void changed_filter();

public:
	FilterModel();
	virtual ~FilterModel();

	const decltype(m_expressions)& getExpressions();

	int rowCount(const QModelIndex & parent = QModelIndex()) const;
	int columnCount(const QModelIndex & parent = QModelIndex()) const;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
	Qt::ItemFlags flags(const QModelIndex & index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	void appendData(const fsdiff::filter_item_t& aData);
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
