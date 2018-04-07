/*
 * sortfilterproxy.h
 *
 *  Created on: Dec 31, 2017
 *      Author: martin
 */

#ifndef SORTFILTERPROXY_H_
#define SORTFILTERPROXY_H_

#include <QSortFilterProxyModel>
#include "fsdiff.h"
#include <map>
#include <set>

class SortFilterProxy : public QSortFilterProxyModel {
public:
	SortFilterProxy(QObject* aParent);
	virtual ~SortFilterProxy();

	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

	void set_cause_filter(fsdiff::cause_t aCause, bool aEnabled);

	void setExpressions(std::vector<fsdiff::filter_item_t> aExpressions);

protected:
	std::set<fsdiff::cause_t> m_items_show;
	mutable std::map<fsdiff::diff_t*, std::set<fsdiff::cause_t> > m_cause_cache;

	std::vector<fsdiff::filter_item_t> m_expressions;
};

#endif /* SORTFILTERPROXY_H_ */
