/*
 * fsdiff.cpp
 *
 *  Created on: Jan 2, 2018
 *      Author: martin
 */

#include "fsdiff.h"
#include "sys/stat.h"
#include "logger.h"
#include <tuple>
#include <array>
#include <cmath>
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
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
			cause_t::DIR_TO_FILE,
			cause_t::CONTENT,
			cause_t::UNKNOWN,
		};

		return values;
	}

	path diff_t::getLastName(idx_t aIdx)
	{
		return fullpath[aIdx].filename();
	}

	bool diff_t::isBase()
	{
		if( fullpath[LEFT] == baseDir[LEFT] )
			return true;

		return false;
	}

	void diff_t::createFileHashes(std::function<void(int)> aStep)
	{
		file_hashes = shared_ptr<file_hash_t>(new file_hash_t);

		size_t filesize_sum = 0;

		aStep(0);

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
					QFile hashFile( (aTree.fullpath[iSide]).string().c_str() ) ;
					if( !hashFile.open(QIODevice::ReadOnly) ) {
						LoggerWarning( string("FileHash: cannot open ") + aTree.fullpath[iSide].string().c_str() );
						continue;
					}


					QCryptographicHash hashFunction(QCryptographicHash::Sha3_512);
					if( !hashFunction.addData(&hashFile)) {
						LoggerWarning( string("FileHash: cannot generate hash from file (open readonly)") + aTree.fullpath[iSide].string().c_str() );
						continue;
					}

					QByteArray result = hashFunction.result();

					auto result_vector = vector<unsigned char>(result.begin(), result.end());

					file_hashes->path_hash[aTree.fullpath[iSide]] = result_vector;
					file_hashes->hash_path[result_vector].push_back(aTree.fullpath[iSide]);
					file_hashes->path_diff[aTree.fullpath[iSide]] = &aTree;

					filesize_hashed += file_size(aTree.fullpath[iSide]);
					aStep((filesize_hashed*1000.0)/filesize_sum);
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

			auto hashLeft = file_hashes->path_hash.find(aTree.fullpath[0]);
			auto hashRight = file_hashes->path_hash.find(aTree.fullpath[1]);

			if (hashLeft == file_hashes->path_hash.end() || hashRight == file_hashes->path_hash.end()) {
				return;
			}

			if( hashLeft->second != hashRight->second )
				aTree.cause = cause_t::CONTENT;
		});
	}

	bool filter_item_t::is_included(const std::vector<filter_item_t>& aFilter, const std::string& aText)
	{
		int allow_count = 0;

		for(auto iFilter: aFilter) {
			allow_count += iFilter.exclude ? 0 : 1;
			if( -1 != QRegExp(iFilter.regex.c_str()).indexIn(QString(aText.c_str())) ) {

				if( iFilter.exclude ) {
					return false;
				} else {
					return true;
				}
			}
		}

		return !(allow_count > 0);
	}

	static bool impl_check_access(const path& aPath)
	{
		using namespace boost::filesystem;

		try {
			if( is_regular_file(aPath) ) {
				//check if we have access permissions to read
				file_status result = status(aPath);
				if( !(result.permissions() & (owner_read|group_read|others_read)) )
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
														std::function<void(string)> aFunction,
														const std::vector<filter_item_t>& aFilter)
	{
		shared_ptr<diff_t> ret = make_shared<diff_t>();

		ret->fullpath[diff_t::LEFT] = aOwnPath;
		ret->baseDir[diff_t::LEFT]  = aAbsoluteBase;
		ret->cause = cause_t::SAME;
		ret->parent = aParent;
		ret->debug_id = ++next_debug_id;

		aFunction(aOwnPath.string().c_str());

		if( !is_directory( ret->fullpath[diff_t::LEFT] ) )
			return ret;
		if( !impl_check_access( ret->fullpath[diff_t::LEFT] ) )
			return ret;

		for(directory_entry iEntry: directory_iterator( ret->fullpath[diff_t::LEFT] ) ) {

			if( !impl_check_access(iEntry.path() ) ) {
				LoggerWarning( string("no access rights for ") + iEntry.path().string() );
				continue;
			}

			//filter
			if( !filter_item_t::is_included(aFilter, iEntry.path().string().c_str()) )
				continue;

			ret->childs.push_back( impl_list_dir_rekursive(aAbsoluteBase, iEntry.path(), ret.get(), aFunction, aFilter) );
		}


		return ret;
	}

	shared_ptr<diff_t> list_dir_rekursive(	path aAbsoluteBase,
											std::function<void(string)> aFunction,
											const std::vector<filter_item_t>& aFilter)
	{
		return impl_list_dir_rekursive(aAbsoluteBase, aAbsoluteBase, nullptr, aFunction, aFilter);
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
		for(auto& iChild: aLeft->childs) {

			path child_lastname = iChild->getLastName();

			auto right_iter =  find_if(aRight->childs.begin(), aRight->childs.end(), [&iChild, &child_lastname](shared_ptr<diff_t>& aDiff) {
				return aDiff->getLastName() == child_lastname;
			});

			const bool isInRight = right_iter != aRight->childs.end();
			const bool isDirLeft = is_directory( iChild->fullpath[diff_t::LEFT] );

			//aFunction( std::string("compare left side: ")
			//					+ iChild->fullpath[diff_t::LEFT].string().c_str() );

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
			else {
				boost::system::error_code err0, err1;
				auto filesize_left = boost::filesystem::file_size(iChild->fullpath[0], err0);
				auto filesize_right = boost::filesystem::file_size(iChild->fullpath[1], err1);
				bool is_no_err = boost::system::errc::success == err0.value() && boost::system::errc::success == err1.value();

				if (is_no_err && filesize_left == filesize_right) {
					iChild->cause = cause_t::SAME;
				}
				else if (is_no_err && filesize_left != filesize_right) {
					iChild->cause = cause_t::CONTENT;
				}
				else {
					iChild->cause = cause_t::UNKNOWN;
				}
			
			}
		}

		for(auto& iChild: aRight->childs) {

			path child_lastname = iChild->getLastName();

			auto found =  find_if(aLeft->childs.begin(), aLeft->childs.end(), [&iChild, &child_lastname](shared_ptr<diff_t>& aDiff) {
				return aDiff->getLastName() == child_lastname;
			});

			//aFunction( std::string("compare right side: ")
			//								+ iChild->fullpath[diff_t::LEFT].string().c_str() );

			if( found == aLeft->childs.end() ) {
				impl_set_cause_rekurively(iChild, cause_t::ADDED);
				iChild->parent = aLeft.get();
				impl_move(iChild, diff_t::LEFT, diff_t::RIGHT, true);
				aLeft->childs.push_back( iChild );
			}
		};

	}

	shared_ptr<diff_t> compare(	path aAbsoluteLeft,
								path aAbsoluteRight,
								std::function<void(string)> aFunction,
								const std::vector<filter_item_t>& aFilter)
	{
		shared_ptr<diff_t> left = impl_list_dir_rekursive(aAbsoluteLeft, aAbsoluteLeft, nullptr, aFunction, aFilter);
		shared_ptr<diff_t> right = impl_list_dir_rekursive(aAbsoluteRight, aAbsoluteRight, nullptr, aFunction, aFilter);

		cout<<"*** compare ***"<<std::endl;

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

	string pretty_print_size(int64_t aSize, std::string aPrintStyle)
	{
		bool isNeg = aSize < 0;

		const int count = sizeof(pretty_print_styles) / sizeof(pretty_print_styles[0]);
		size_t log = static_cast<size_t>( log2(abs(aSize)) / log2(1024) );
		string byte_output = (boost::format("( %1% %2% )") % aSize % pretty_print_styles[1]).str();

		int iLog = 0;
		for( auto iStyle: pretty_print_styles) {
			if( iStyle == pretty_print_styles[0] ) {
				continue;
			}
			if( iStyle == aPrintStyle ) {
				byte_output = "";
				log = iLog;
				break;
			}
			iLog++;
		}

		if( log >= count-1 )
			log = count-1;

		return (boost::format("%|1$.1f| %|2$| %|3$|")
			% (aSize/pow(1024, log))
			% pretty_print_styles[log+1]
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
