
#include "treemodel.h"
#include <QStringList>
#include <QBrush>

enum class column_e {
    ITEM_NAME=0,
    ITEM_CAUSE,
    DIFF_SIZE,
	HASH,
    LEN,
};

static QString column_e_str(column_e aColumn)
{
	switch(aColumn)
	{
	case column_e::ITEM_NAME:		return "Filename";
	case column_e::ITEM_CAUSE:	return "Cause";
	case column_e::DIFF_SIZE:		return "Size Difference";
	case column_e::HASH:			return "Hashvalue";
	case column_e::LEN:			return "Length";
	}

	return "";
}

TreeModel::TreeModel(QObject *parent, std::shared_ptr<fsdiff::diff_t> aDiffTree)
    : QAbstractItemModel(parent)
{
    rootItem = aDiffTree;
}

TreeModel::~TreeModel()
{

}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    return static_cast<int>(column_e::LEN);
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
	using namespace fsdiff;

    if (!index.isValid())
        return QVariant();



    if (role == Qt::DisplayRole) {
        if( nullptr == index.internalPointer() )
        	return QVariant();

        fsdiff::diff_t* item = static_cast<fsdiff::diff_t*>(index.internalPointer());

        const bool is_added = fsdiff::cause_t::ADDED == item->cause;
        const diff_t::idx_t idx_side = is_added ? diff_t::RIGHT : diff_t::LEFT;

        if( index.column() == static_cast<int>(column_e::ITEM_NAME) ) {
            return QString( item->getLastName(idx_side).c_str() );
        }
        else if(  index.column() == static_cast<int>(column_e::ITEM_CAUSE)  ) {
			return fsdiff::cause_t_str( item->cause ).c_str();
		 }
        else if(  index.column() == static_cast<int>(column_e::DIFF_SIZE)  ) {
            auto sdiff = fsdiff::diff_size(*item);
        	return QString("%1").arg( pretty_print_size(sdiff).c_str() );
        }
        else {
            return QVariant();
        }
    }
    else if( role == Qt::BackgroundRole ) {
    	using namespace fsdiff;

    	fsdiff::diff_t* item = static_cast<fsdiff::diff_t*>(index.internalPointer());

    	switch( item->cause )
    	{
    	case cause_t::ADDED: 		return QBrush( QColor(194, 255, 168) );
    	case cause_t::DELETED:		return QBrush( QColor(214, 255, 250) );
    	case cause_t::DIR_TO_FILE:	return QBrush( QColor(255, 245, 186) );
    	case cause_t::FILE_TO_DIR:	return QBrush( QColor(255, 245, 186) );
    	case cause_t::CONTENT:		return QBrush( QColor(255, 209, 209) );
    	default:					return QVariant();
    	}
    }


    return QVariant();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
	if (role != Qt::DisplayRole)
	    return QVariant();

	return column_e_str( static_cast<column_e>(section));
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    fsdiff::diff_t* parentItem = rootItem.get();

    if (parent.isValid()) {
        parentItem = static_cast<fsdiff::diff_t*>(parent.internalPointer());
    }

    if( (size_t)row >= parentItem->childs.size() ) {
    	throw "TreeModel::index index rownumber to height";
    }

    return createIndex(row, column, (void*)(parentItem->childs[row].get()));
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    fsdiff::diff_t* childItem = static_cast<fsdiff::diff_t*>(index.internalPointer());
    if( nullptr == childItem )
    	return QModelIndex();
    fsdiff::diff_t* parentItem = childItem->parent;

    if (parentItem == rootItem.get() || nullptr == parentItem) {
    	return QModelIndex();
    }

    int row = -1;
    for(size_t i=0; i<parentItem->parent->childs.size(); i++) {
        if( parentItem->parent->childs[i].get() == parentItem  ) {
            row = i;
        }
    }

    if( row < 0 )
    	throw "TreeModel::parent: cannot find parent index";

    return createIndex(row, 0, (void*)parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    fsdiff::diff_t* parentItem = rootItem.get();
    if (parent.column() > 0)
        return 0;

    if (parent.isValid())
        parentItem = static_cast<fsdiff::diff_t*>(parent.internalPointer());

    return parentItem->childs.size();
}

void TreeModel::setupModelData()
{
    path  left("/home/martin/Dropbox/Programming/tools_and_snippets/cpp_snippets/");
    path right("/home/martin/Dropbox/Programming/tools_and_snippets/cpp_snippets_copy/");

    rootItem = fsdiff::compare(left, right);
    //rootItem = fsdiff::list_dir_rekursive(left);
}

void TreeModel::iterate_over_all_inner(std::function<void(QModelIndex)> aFunc, QModelIndex aModelIndex)
{
	if( aModelIndex.isValid() ) {
			aFunc(aModelIndex);
		}

		if( !hasChildren(aModelIndex) ) {
			return; }

		const auto rows = rowCount(aModelIndex);

		for(int iRow=0; iRow<rows; iRow++) {
			iterate_over_all_inner( aFunc, index(iRow, 0, aModelIndex) );
	}
}

void TreeModel::iterate_over_all(std::function<void(QModelIndex)> aFunc)
{
	iterate_over_all_inner(aFunc, QModelIndex() );
}

void TreeModel::startFileHash( std::function<void()> aOnReady, std::function<void(int,int,int)> aStep  )
{
	QThread* thread = new QThread;
	FilehashWorker* worker = new FilehashWorker(rootItem);

	worker->moveToThread(thread);
	connect(this, &TreeModel::operateFilehash, worker, &FilehashWorker::hashAllFiles);
	connect(worker, &FilehashWorker::resultReady, this, aOnReady );
	connect(worker, &FilehashWorker::stepReady, this, aStep );

	connect(worker, SIGNAL (finished()), thread, SLOT (quit()));
	connect(worker, SIGNAL (finished()), worker, SLOT (deleteLater()));
	connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));


	thread->start();
	emit operateFilehash();
}

void TreeModel::refresh()
{
	beginResetModel();
	endResetModel();
}



FilehashWorker::FilehashWorker(shared_ptr<fsdiff::diff_t>& aTree)
	: m_tree(aTree)
{

}

FilehashWorker::~FilehashWorker()
{
	cout<<"FilehashWorker deleted";
}

void FilehashWorker::hashAllFiles()
{
	m_tree->createFileHashes([this](int aMin, int aMax, int aCurr) {
		emit stepReady(aMin, aMax, aCurr);
	});

	emit resultReady();
}

