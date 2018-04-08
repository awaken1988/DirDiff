
#include "duplicatemodel.h"
#include <QStringList>
#include <QBrush>
#include <QFileIconProvider>
#include <QFileInfo>

enum class dup_data_e {
	ICON=0,
	PATH=1,
	COUNT,
};

DuplicateModel::duplicate_t DuplicateModel::create_summary(	fsdiff::diff_t* aDiffTree,
															fsdiff::diff_t::idx_t aSide)
{
	set<vector<unsigned char>> hashes_used;
	std::vector<std::vector<fsdiff::diff_t*> > duplicate_items;

	fsdiff::foreach_diff_item(*aDiffTree, [aSide, &hashes_used, &duplicate_items](fsdiff::diff_t& aTree) {

		if( !is_regular_file(aTree.fullpath[aSide]) )
			return;
		if( nullptr == aTree.file_hashes )
			return;

		auto findItem = aTree.file_hashes->path_hash.find(aTree.fullpath[aSide]);
		if( findItem ==  aTree.file_hashes->path_hash.end() )
			return;

		auto hash = findItem->second;
		if( hashes_used.find(hash) != hashes_used.end() )
			return;

		auto findHash = aTree.file_hashes->hash_path.find(hash);

		std::vector<fsdiff::diff_t*> result;

		cout<<"duplicate hash="<<hash.size()<<endl;
		for(auto iPath: findHash->second) {

			auto diffPtr = aTree.file_hashes->path_diff[iPath];

			if( iPath == diffPtr->fullpath[aSide?0:1] )
				continue;

			result.push_back(aTree.file_hashes->path_diff[iPath]);
			cout<<"	duplicate file"<<iPath<<endl;
		}

		if( result.size() > 1 ) {
			duplicate_items.push_back(result);
		}

		hashes_used.insert(hash);
	});

	for(const auto& iArr: duplicate_items) {
		std::cout<<"bla"<<std::endl;
		for(const auto& iElement: iArr) {
			std::cout<<iElement->fullpath[aSide].c_str()<<std::endl;
		}
	}

	return duplicate_items;
}

//! search all duplicates for diff a specific diff item
DuplicateModel::duplicate_t DuplicateModel::create_onefile(	fsdiff::diff_t* aDiff,
															fsdiff::diff_t::idx_t aSide)
{
	const auto hash_value = aDiff->file_hashes->path_hash[aDiff->fullpath[aSide]];
	std::vector<fsdiff::diff_t*> items;

	for(const auto& iPath: aDiff->file_hashes->hash_path[hash_value]) {
		auto curr_item = aDiff->file_hashes->path_diff[iPath];
		items.push_back( curr_item );
	}

	return std::vector<std::vector<fsdiff::diff_t*> >{items};
}

DuplicateModel::DuplicateModel(fsdiff::diff_t::idx_t aSide, std::vector<std::vector<fsdiff::diff_t*>> aData)
    : QAbstractItemModel(nullptr), m_side(aSide), m_duplicate_items(aData)
{

}

DuplicateModel::~DuplicateModel()
{

}

int DuplicateModel::columnCount(const QModelIndex &parent) const
{
    return static_cast<int>(dup_data_e::COUNT);
}

QVariant DuplicateModel::data(const QModelIndex &index, int role) const
{
	using namespace fsdiff;

    if (!index.isValid())
        return QVariant();

    auto indice = static_cast<std::tuple<int,int>*>(index.internalPointer());

	//TODO: try tie operator
	int idx0 = std::get<0>(*indice);
	int idx1 = std::get<1>(*indice);

    if (role == Qt::DisplayRole) {

    	if( -1 == idx1 ) {
    		if( 1 != index.column() ) {
    			return QVariant();
    		}

    		fsdiff::diff_t* diff_item = m_duplicate_items[idx0][0];

    		const auto filesize = boost::filesystem::file_size(diff_item->fullpath[m_side]);

    		return QString("Filesize=%1")
    				.arg(pretty_print_size(filesize, "auto").c_str());
    	}
    	else {
    		switch(static_cast<dup_data_e>(index.column()))
    		{
    		case dup_data_e::PATH:
    			return QString("%1").arg(m_duplicate_items[idx0][idx1]->fullpath[m_side].c_str());
    		case dup_data_e::ICON:
    			return QVariant();
    		default:
    			return QVariant();
    		};
    	}
    }
    else if( role == Qt::DecorationRole ) {

    	if( -1 == idx1 ) {
    		return QVariant();
    	}


    	switch(static_cast<dup_data_e>(index.column()))
		{
		case dup_data_e::PATH:
			return QVariant();
		case dup_data_e::ICON:
			return QFileIconProvider().icon( QFileInfo(m_duplicate_items[idx0][idx1]->fullpath[m_side].c_str()) );
		default:
			return QVariant();
		};
    }


    return QVariant();
}

Qt::ItemFlags DuplicateModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QVariant DuplicateModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
	if (role != Qt::DisplayRole)
	    return QVariant();

	return QVariant();
}

QModelIndex DuplicateModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const std::tuple<int,int>* indice = nullptr;

    if (!parent.isValid()) {
    	indice = find_create_indice(std::tuple<int,int>(-1, -1));
    }
    else {
    	indice = static_cast<std::tuple<int,int>*>(parent.internalPointer());
    }

    const int idx0 = std::get<0>(*indice);
    const int idx1 = std::get<1>(*indice);

    if( -1 != idx0 && -1 != idx1) {
    	return QModelIndex();
    }
    if( -1 != idx0 && -1 == idx1 ) {

    	if( row >= m_duplicate_items[idx0].size() )
    		return QModelIndex();

    	return createIndex(row, column, (void*)find_create_indice(std::tuple<int,int>(idx0, row)));
    }

    if( row >= m_duplicate_items.size() )
        		return QModelIndex();

    return createIndex(row, column, (void*)find_create_indice(std::tuple<int,int>(row, -1)));
}

QModelIndex DuplicateModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    auto indice = static_cast<std::tuple<int,int>*>(index.internalPointer());

    if( nullptr == indice )
    	return QModelIndex();

    //TODO: try tie operator
    int idx0 = std::get<0>(*indice);
    int idx1 = std::get<1>(*indice);

    if( -1 == idx0 )
    	return QModelIndex();

    if( -1 == idx1 )
    	return createIndex(idx0, 0, (void*)find_create_indice(std::tuple<int,int>(-1, -1)));

    return createIndex(idx1, 0, (void*)find_create_indice(std::tuple<int,int>(idx0, -1)));
}

int DuplicateModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    std::tuple<int,int>* indice = static_cast<std::tuple<int,int>*>(parent.internalPointer());

    if( nullptr == indice || !parent.isValid() )
    	return m_duplicate_items.size();

    //TODO: try tie operator
	int idx0 = std::get<0>(*indice);
	int idx1 = std::get<1>(*indice);

	if( idx0 == -1 ) {
		return m_duplicate_items.size();
	}

	if( idx1 == -1 ) {
		return m_duplicate_items[idx0].size();
	}

	return 0;
}

const std::tuple<int,int>* DuplicateModel::find_create_indice(std::tuple<int,int> aIndex) const
{
	auto item = m_indices.find(aIndex);

	if( item == m_indices.end() ) {
		m_indices.insert(aIndex);
	}

	return &(*(m_indices.find(aIndex)));
}

