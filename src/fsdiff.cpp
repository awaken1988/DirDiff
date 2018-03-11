/*
 * fsdiff.cpp
 *
 *  Created on: Jan 2, 2018
 *      Author: martin
 */

#include "fsdiff.h"
#include "sys/stat.h"
#include <tuple>
#include <array>
#include <cmath>
#include <boost/format.hpp>
#include <QFile>
#include <QCryptographicHash>

namespace fsdiff
{

	static string indent_str(int aLevel)
	{
		string ret;

		for(int i=0; i<aLevel; i++)
			ret += "    ";

		return ret;
	}

	string cause_t_str(cause_t aCause)
	{
		switch(aCause)
		{
			case cause_t::SAME:			return "SAME";
			case cause_t::ADDED:		return "ADDED";
			case cause_t::DELETED:		return "DELETED";
			case cause_t::FILE_TO_DIR:	return "FILE_TO_DIR";
			case cause_t::DIR_TO_FILE:	return "DIR_TO_FILE";
			case cause_t::CONTENT:		return "CONTENT";
			default: return "UNKNOWN";
		}
	}

	const set<cause_t>& cause_t_list()
	{
		static set<cause_t> values = { cause_t::SAME,
			cause_t::ADDED,
			cause_t::DELETED,
			cause_t::FILE_TO_DIR,
			cause_t::DIR_TO_FILE, };

		return values;
	}

	path diff_t::getLastName(idx_t aIdx)
	{
	   path ret;

	   for(auto iElements: fullpath[aIdx]) {
		   ret = iElements;
	   }

	   return ret;
	}

	bool diff_t::isBase()
	{
		if( fullpath[LEFT] == baseDir[LEFT] )
			return true;

		return false;
	}

	void diff_t::createFileHashes(std::function<void(int,int,int)> aStep)
	{
		file_hashes = shared_ptr<file_hash_t>(new file_hash_t);

		size_t filesize_sum = 0;

		foreach_diff_item(*this, [this, &filesize_sum](diff_t& aTree) {
			for(int iSide=0; iSide<2; iSide++) {
				if( is_regular_file(aTree.fullpath[iSide]) ) {
					filesize_sum += file_size(aTree.fullpath[iSide]);
				}
			}
		});

		//calculate hashsum
		size_t filesize_hashed = 0;
		foreach_diff_item(*this, [this,&filesize_hashed,filesize_sum, aStep](diff_t& aTree) {
			for(int iSide=0; iSide<2; iSide++) {
				if( is_regular_file(aTree.fullpath[iSide]) ) {
					QFile hashFile(aTree.fullpath[iSide].c_str());
					if( !hashFile.open(QIODevice::ReadOnly) )
						continue;

					QCryptographicHash hashFunction(QCryptographicHash::Sha3_512);
					if( !hashFunction.addData(&hashFile))
						continue;

					QByteArray result = hashFunction.result();

					auto result_vector = vector<unsigned char>(result.begin(), result.end());

					file_hashes->path_hash[aTree.fullpath[iSide]] = result_vector;
					file_hashes->hash_path[result_vector].push_back(aTree.fullpath[iSide]);
					file_hashes->path_diff[aTree.fullpath[iSide]] = &aTree;

					filesize_hashed += file_size(aTree.fullpath[iSide]);
					aStep(0, filesize_sum, filesize_hashed);
				}
			}
		});

		//set file_hashes for every child
		//TODO: is this thread save?
		foreach_diff_item(*this, [this](diff_t& aTree) {
			aTree.file_hashes = this->file_hashes;
		});

		//update diffcause
		foreach_diff_item(*this, [this,&filesize_hashed,filesize_sum, aStep](diff_t& aTree) {

			if( cause_t::SAME != aTree.cause )
				return;

			auto hashLeft = file_hashes->path_hash[aTree.fullpath[0]];
			auto hashRight = file_hashes->path_hash[aTree.fullpath[1]];

			if( hashLeft != hashRight )
				aTree.cause = cause_t::CONTENT;
		});
	}

	static bool impl_check_access(const path& aPath)
	{
		try {
			if( is_regular_file(aPath) ) {
				//check if we have access permissions to read
				file_status result = status(aPath);
				if( !(result.permissions() & (S_IRUSR|S_IRGRP|S_IROTH)) )
					return false;

				file_size(aPath);
			} else if( is_directory(aPath) ) {
				for(directory_entry iEntry: directory_iterator( aPath ) ) {
					//do nothing
				}
			}
			else {
				return false;	// until now we only handly normal files an directory
			}
		}
		catch(boost::filesystem::filesystem_error& e) {
			cout<<"cannot access: "<<aPath<<endl;
			return false;
		}

		return true;
	}

	int next_debug_id = 1000000;
	static shared_ptr<diff_t> impl_list_dir_rekursive(	path aAbsoluteBase,
														path aOwnPath,
														diff_t* aParent,
														std::function<void(string)> aFunction)
	{
		shared_ptr<diff_t> ret = make_shared<diff_t>();

		ret->fullpath[diff_t::LEFT] = aOwnPath;
		ret->baseDir[diff_t::LEFT]  = aAbsoluteBase;
		ret->cause = cause_t::SAME;
		ret->parent = aParent;
		ret->debug_id = ++next_debug_id;

		aFunction(aOwnPath.c_str());

		if( !is_directory( ret->fullpath[diff_t::LEFT] ) )
			return ret;
		if( !impl_check_access( ret->fullpath[diff_t::LEFT] ) )
			return ret;

		for(directory_entry iEntry: directory_iterator( ret->fullpath[diff_t::LEFT] ) ) {

			if( !impl_check_access(iEntry.path() ) )
				continue;

			ret->childs.push_back( impl_list_dir_rekursive(aAbsoluteBase, iEntry.path(), ret.get(), aFunction) );
		}


		return ret;
	}

	shared_ptr<diff_t> list_dir_rekursive(path aAbsoluteBase, std::function<void(string)> aFunction)
	{
		return impl_list_dir_rekursive(aAbsoluteBase, aAbsoluteBase, nullptr, aFunction);
	}


	static void impl_copy_diff(shared_ptr<diff_t>& aLeft, shared_ptr<diff_t>& aRight)
	{
		aLeft->fullpath[diff_t::RIGHT] = aRight->fullpath[diff_t::LEFT];
		aLeft->baseDir[diff_t::RIGHT] = aRight->baseDir[diff_t::LEFT];
	}

	static void impl_set_cause_rekurively(shared_ptr<diff_t>& aDiff, cause_t aCause)
	{
		aDiff->cause = aCause;
		for(auto& iChild: aDiff->childs) {
			impl_set_cause_rekurively(iChild, aCause);
		}
	}

	static void impl_move(shared_ptr<diff_t> aDiff, diff_t::idx_t aIdxFrom, diff_t::idx_t aIdxTo, bool aIsRek)
	{
		aDiff->fullpath[aIdxTo] = aDiff->fullpath[aIdxFrom];
		aDiff->fullpath[aIdxFrom] = path();

		aDiff->baseDir[aIdxTo] = aDiff->baseDir[aIdxFrom];
		aDiff->baseDir[aIdxFrom] = path();

		if( aIsRek ) {
			for(auto& iChild: aDiff->childs) {
				impl_move(iChild, aIdxFrom, aIdxTo, aIsRek);
			}
		}
	}

	static void impl_compare(shared_ptr<diff_t>& aLeft, shared_ptr<diff_t>& aRight, std::function<void(string)> aFunction)
	{
		aFunction( std::string("compare ")
					+ aLeft->getLastName(diff_t::LEFT).c_str()
					+ std::string(" --- ")
					+ aRight->getLastName(diff_t::LEFT).c_str() );

		for(auto& iChild: aLeft->childs) {
			auto right_iter =  find_if(aRight->childs.begin(), aRight->childs.end(), [&iChild](shared_ptr<diff_t>& aDiff) {
				return aDiff->getLastName() == iChild->getLastName();
			});

			const bool isInRight = right_iter != aRight->childs.end();
			const bool isDirLeft = is_directory( iChild->fullpath[diff_t::LEFT] );

			if( !isInRight ) {
				iChild->cause = cause_t::DELETED;
				impl_set_cause_rekurively(iChild, iChild->cause);
				continue;
			}

			const bool isDirRight = is_directory( (*right_iter)->fullpath[diff_t::LEFT] );

			if( isDirLeft && !isDirRight ) {
				iChild->cause = cause_t::DIR_TO_FILE;
				impl_set_cause_rekurively(iChild, iChild->cause);
				impl_copy_diff(iChild, *right_iter);
				continue;
			}

			if( !isDirLeft && isDirRight ) {
				iChild->cause = cause_t::FILE_TO_DIR;
				iChild->childs.clear();
				iChild->childs = (*right_iter)->childs;
				for(auto& iChildChild: iChild->childs) {
					iChildChild->parent = iChild.get();
				}

				impl_set_cause_rekurively(iChild, iChild->cause);
				impl_copy_diff(iChild, *right_iter);
				continue;
			}

			impl_copy_diff(iChild, *right_iter);

			if( isDirLeft ) {
				impl_compare(iChild, *right_iter, aFunction);
			}
		}

		for(auto& iChild: aRight->childs) {
			auto found =  find_if(aLeft->childs.begin(), aLeft->childs.end(), [&iChild](shared_ptr<diff_t>& aDiff) {
				return aDiff->getLastName() == iChild->getLastName();
			});

			if( found == aLeft->childs.end() ) {
				impl_set_cause_rekurively(iChild, cause_t::ADDED);
				iChild->parent = aLeft.get();
				impl_move(iChild, diff_t::LEFT, diff_t::RIGHT, true);
				aLeft->childs.push_back( iChild );
			}
		};

	}

	shared_ptr<diff_t> compare(path aAbsoluteLeft, path aAbsoluteRight, std::function<void(string)> aFunction)
	{
		shared_ptr<diff_t> left = impl_list_dir_rekursive(aAbsoluteLeft, aAbsoluteLeft, nullptr, aFunction);
		shared_ptr<diff_t> right = impl_list_dir_rekursive(aAbsoluteRight, aAbsoluteRight, nullptr, aFunction);

		impl_compare(left, right, aFunction);

		return left;
	}

	int64_t diff_size(diff_t& aTree)
	{
		uintmax_t file_sizes[] = {0, 0};

		for(int iSide=0; iSide<2; iSide++) {
			if(! is_regular_file(aTree.fullpath.at(iSide)) )
				continue;
			file_sizes[iSide] = file_size(aTree.fullpath.at(iSide));
		}

		return static_cast<int64_t>(file_sizes[1])-static_cast<int64_t>(file_sizes[0]);
	}

	string pretty_print_size(int64_t aSize)
	{
		bool isNeg = aSize < 0;

		vector<string> sizes = {"bytes", "KiB", "MiB", "GiB"};

		int log = static_cast<int>( log2(abs(aSize)) / log2(1024) );
		const string byte_output = (boost::format("%1% %2%") % aSize % sizes[0]).str();

		if( log < 1) {
			return byte_output;
		}

		if( log >= sizes.size() )
			log = sizes.size()-1;

		return (boost::format("%|1$.1f| %|2$| ( %|3$| )")
			% (aSize/pow(1024, log))
			% sizes[log]
			% byte_output).str();
	}

	void dump(shared_ptr<diff_t> &aTree, int aDepth)
	{
		using namespace std;

		cout<<indent_str(aDepth)<<"* "<<endl;
		cout<<indent_str(aDepth)<<"debug_id="<<aTree->debug_id<<endl;
		cout<<indent_str(aDepth)<<"parent="<<aTree->parent<<endl;
		cout<<indent_str(aDepth)<<"self="<<aTree.get()<<endl;
		cout<<indent_str(aDepth)<<"left  fullpath"<<aTree->fullpath[diff_t::LEFT]<<endl;
		cout<<indent_str(aDepth)<<"right fullpath"<<aTree->fullpath[diff_t::RIGHT]<<endl;
		cout<<indent_str(aDepth)<<"left  baseDir"<<aTree->baseDir[diff_t::LEFT]<<endl;
		cout<<indent_str(aDepth)<<"right baseDir"<<aTree->baseDir[diff_t::RIGHT]<<endl;


		for(auto& iChild: aTree->childs) {
			dump(iChild, aDepth+1);
		}
	}

	void foreach_diff_item(diff_t& aTree, std::function<void(diff_t& aTree)> aFunction)
	{
		aFunction(aTree);

		for(auto iChild: aTree.childs) {
			foreach_diff_item(*iChild, aFunction);
		}
	}



} /* namespace fsdiff */
