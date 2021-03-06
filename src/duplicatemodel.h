#ifndef DUPLICATEMODEL_H
#define DUPLICATEMODEL_H

#include <functional>
#include <vector>
#include <tuple>
#include <set>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QThread>
#include <QPair>
#include "fsdiff.h"



class DuplicateModel : public QAbstractItemModel
{
    Q_OBJECT

public:
	using duplicate_t = std::vector<std::vector<fsdiff::diff_t*> >;

	//! search all duplicates of a side
	static duplicate_t create_summary(	fsdiff::diff_t* aDiffTree,
										fsdiff::diff_t::idx_t aSide);

	//! search all duplicates for diff a specific diff item
	static duplicate_t create_onefile(	fsdiff::diff_t* aDiff,
										fsdiff::diff_t::idx_t aSide);

	using diff_sptr = std::shared_ptr<fsdiff::diff_t>;

	DuplicateModel(	fsdiff::diff_t::idx_t iSide,
					std::vector<std::vector<fsdiff::diff_t*>> aData);

    ~DuplicateModel();

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    const std::tuple<int,int>* find_create_indice(std::tuple<int,int> aIndex) const;

private:
    std::vector<std::vector<fsdiff::diff_t*> > m_duplicate_items;
    mutable std::set<std::tuple<int,int>> m_indices;

    fsdiff::diff_t::idx_t m_side;
};

#endif // DUPLICATEMODEL_H
