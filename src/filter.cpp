#include "filter.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QKeyEvent>
#include <QItemSelectionModel>
#include <QModelIndexList>
#include <iostream>

enum class filter_col_e : int
{
	FILTER=0,
	EXPRESSION=1,
	COUNT,
};

Filter::Filter(QWidget* parent)
	: QWidget(parent)
{
	auto* mainLayout = new QVBoxLayout();
	auto* topLayout = new QHBoxLayout();
	setLayout(mainLayout);
	mainLayout->addLayout(topLayout);

	//top: label, search input field, buttons
	{
		auto* lbl = new QLabel("Filter:");
		m_search_input = new QLineEdit();
		m_btn_include = new QPushButton("Include");
		m_btn_exclude = new QPushButton("Exclude");

		//TODO: merge these two functions
		connect(m_btn_include, &QPushButton::clicked, [this](bool aChecked) -> void {
			addExpression(m_search_input->text(), false);

		});

		connect(m_btn_exclude, &QPushButton::clicked, [this](bool aChecked) -> void {
			addExpression(m_search_input->text(), true);
		});

		topLayout->addStretch(1);
		topLayout->addWidget(lbl);
		topLayout->addWidget(m_search_input);
		topLayout->addWidget(m_btn_include);
		topLayout->addWidget(m_btn_exclude);
		topLayout->addStretch(1);
	}

	//list of filters
	{
		m_model = new FilterModel();
		m_table = new QTableView();
		m_table->setMinimumSize(0, 300);
		m_table->setModel( m_model );
		mainLayout->addWidget(m_table);

		m_table->horizontalHeader()->setSectionResizeMode( (int)filter_col_e::EXPRESSION, QHeaderView::Stretch);
		m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_table->setSelectionMode(QAbstractItemView::ContiguousSelection);

		m_shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), m_table);
	    connect(m_shortcut, &QShortcut::activated, [this]() -> void {
	    	QItemSelectionModel* select = m_table->selectionModel();

	    	std::cout<<"delete filter item:"<<std::endl;

	    	auto selection_list = select->selectedRows();

	    	if( selection_list.count() > 0  ) {
	    		m_model->deleteData(selection_list[0].row(), selection_list.last().row());
	    	}
	    });
	}

}

Filter::~Filter()
{

}

FilterModel& Filter::getModel()
{
	return *m_model;
}

void Filter::addExpression(QString aExpression, bool aExclude)
{
	fsdiff::filter_item_t item = {aExpression.toStdString(), aExclude};

	for(auto iData: *m_model ) {
		if( item.regex == iData.regex && item.exclude == iData.exclude )
			return;
	}

	m_model->appendData( item );
	m_search_input->clear();
	m_search_input->setFocus();
}




FilterModel::FilterModel()
	: QAbstractTableModel(nullptr)
{

}

FilterModel::~FilterModel()
{

}

const decltype(FilterModel::m_expressions)& FilterModel::getExpressions()
{
	return m_expressions;
}

int FilterModel::rowCount(const QModelIndex & parent) const
{
	return m_expressions.size();
}

int FilterModel::columnCount(const QModelIndex & parent) const
{
	return (int)filter_col_e::COUNT;
}

QVariant FilterModel::data(const QModelIndex & index, int role) const
{
	if( !index.isValid() )
		return QVariant();

	if (role == Qt::DisplayRole) {
		switch(index.column()) {
			case (int)filter_col_e::FILTER:
			{
				if( m_expressions[index.row()].exclude  )
					return QString("Exclude");
				return QString("Include");
			} break;
			case (int)filter_col_e::EXPRESSION:
			{
				return QString(m_expressions[index.row()].regex.c_str());
			} break;
			default: break;
		}
	}

	return QVariant();
}

bool FilterModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	return false;
}

QVariant FilterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal) {
		switch(section) {
		case (int)filter_col_e::FILTER:
			{
				return QString("");
			} break;
		case (int)filter_col_e::EXPRESSION:
			{
				return QString("regular expression");
			} break;
		default: break;
		}
	}
	return QVariant();
}

void FilterModel::appendData(const fsdiff::filter_item_t& aData)
{
	beginInsertRows(QModelIndex(), m_expressions.size(), m_expressions.size());
	{
		m_expressions.push_back(aData);
	}
	endInsertRows();

	emit changed_filter();
}

void FilterModel::deleteData(int aStartRow, int aEndRow)
{
	beginRemoveRows(QModelIndex(), aStartRow, aEndRow);
	{
		for(int i=0; i< (aEndRow-aStartRow+1); i++) {
			m_expressions.erase( m_expressions.begin()+aStartRow );
		}
	}
	endRemoveRows();

	emit changed_filter();
}

Qt::ItemFlags FilterModel::flags(const QModelIndex & index) const
{
	return Qt::ItemIsSelectable;
}
