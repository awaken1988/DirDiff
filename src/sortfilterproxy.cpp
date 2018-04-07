/*
 * sortfilterproxy.cpp
 *
 *  Created on: Dec 31, 2017
 *      Author: martin
 */

#include "sortfilterproxy.h"
#include "treemodel.h"
#include <iostream>

SortFilterProxy::SortFilterProxy(QObject* aParent)
	: 	QSortFilterProxyModel(aParent),
		m_items_show(fsdiff::cause_t_list())
{

}

SortFilterProxy::~SortFilterProxy() {
	// TODO Auto-generated destructor stub
}

bool SortFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	using namespace std;
	using namespace fsdiff;
	bool ret_diff = false;

	QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
	diff_t* left_ptr = static_cast<diff_t*>(index0.internalPointer());

//	cout<<"sourceRow="<<sourceRow<<"; row="<<sourceParent.row()<<"; col="<<sourceParent.column()
//				<<"; ptr="<<sourceParent.internalPointer()<<" debug_id="<<left_ptr->debug_id<<endl;


	if( m_cause_cache.find(left_ptr) == m_cause_cache.end() ) {
		fsdiff::foreach_diff_item(*left_ptr, [this,left_ptr](const diff_t& aTree) {
			m_cause_cache[left_ptr].insert(aTree.cause);
		});
	}

	for(auto iCause: m_items_show) {

		auto& curr = m_cause_cache[left_ptr];

		if( curr.find(iCause) != curr.end() ) {
			ret_diff = true;
			break;
		}
	}

	return ret_diff
			&& fsdiff::filter_item_t::is_included(m_expressions, left_ptr->fullpath[0].c_str())
			&& fsdiff::filter_item_t::is_included(m_expressions, left_ptr->fullpath[1].c_str());
}

bool SortFilterProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	using namespace fsdiff;

	diff_t* left_ptr = static_cast<diff_t*>(left.internalPointer());
	diff_t* right_ptr = static_cast<diff_t*>(right.internalPointer());

	return left_ptr->debug_id < right_ptr->debug_id;

}

void SortFilterProxy::set_cause_filter(fsdiff::cause_t aCause, bool aEnabled)
{
	if( aEnabled )
		m_items_show.insert(aCause);
	else
		m_items_show.erase(aCause);

	this->invalidateFilter();
}

void SortFilterProxy::setExpressions(std::vector<fsdiff::filter_item_t> aExpressions)
{
	std::cout<<"setExpression"<<std::endl;
	m_expressions = aExpressions;
	this->invalidateFilter();
}



