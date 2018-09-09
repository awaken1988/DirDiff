/*
 * fsdiff.h
 *
 *  Created on: Jan 2, 2018
 *      Author: martin
 */

#ifndef FSDIFF_H_
#define FSDIFF_H_

#include <iostream>
#include <algorithm>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <cassert>
#include <functional>
#include <vector>
#include <mutex>
#include <array>
#include <boost/filesystem.hpp>
#include "boost/format.hpp"

using namespace std;
using namespace boost::filesystem;

namespace fsdiff
{
	enum class cause_t
	{
		SAME,
		ADDED,
		DELETED,
		FILE_TO_DIR,
		DIR_TO_FILE,
		CONTENT,
		UNKNOWN,
	};

	string cause_t_str(cause_t aCause);

	const set<cause_t>& cause_t_list();

	struct diff_t;

	struct file_hash_t {
		map<path, vector<unsigned char>> path_hash;
		map<vector<unsigned char>, vector<path>> hash_path;
		map<path, diff_t*> path_diff;

	};

	struct diff_t
	{
		//TODO: rename this to side_t
		enum idx_t {
			LEFT=0,
			RIGHT=1,
		};

		array<path, 2> baseDir;
		array<path, 2> fullpath;
		cause_t cause;

		diff_t* parent;
		vector<shared_ptr<diff_t>>	childs;

		path getLastName(idx_t aIdx=LEFT);
		bool isBase();

		//optional filehashes
		void createFileHashes(std::function<void(int)> aStep);
		shared_ptr<file_hash_t> file_hashes;

		int debug_id;
	};

	struct filter_item_t
	{
		std::string regex;
		bool		exclude;	//true=(filter out items match the regex);	false=(display items matches regex)

		static bool is_included(const std::vector<filter_item_t>& aFilter, const std::string& aText);
	};

	shared_ptr<diff_t> list_dir_rekursive(	path aAbsoluteBase,
											std::function<void(string)> aFunction,
											const std::vector<filter_item_t>& aFilter);

	shared_ptr<diff_t> compare(	path aAbsoluteLeft,
								path aAbsoluteRight,
								std::function<void(string)> aFunction,
								const std::vector<filter_item_t>& aFilter);

	int64_t diff_size(diff_t& aTree);

	static string pretty_print_styles[] = {
		"auto",
		"bytes",
		"KiB",
		"MiB",
		"GiB",
	};

	string pretty_print_size(int64_t aSize, std::string aPrintStyle);

	void dump(shared_ptr<diff_t> & aTree, int aDepth=0);

	void foreach_diff_item(diff_t& aTree, std::function<void(diff_t& aTree)> aFunction);

	file_hash_t create_hash(diff_t& aTree);

} /* namespace fsdiff */

#endif /* FSDIFF_H_ */
