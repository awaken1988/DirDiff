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
	};

	string cause_t_str(cause_t aCause);

	const set<cause_t>& cause_t_list();

	struct diff_t
	{
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

		int debug_id;
	};

	shared_ptr<diff_t> list_dir_rekursive(path aAbsoluteBase);

	shared_ptr<diff_t> compare(path aAbsoluteLeft, path aAbsoluteRight);

	int64_t diff_size(diff_t& aTree);

	string pretty_print_size(int64_t aSize);

	void dump(shared_ptr<diff_t> & aTree, int aDepth=0);

	void foreach_diff_item(const diff_t& aTree, std::function<void(const diff_t& aTree)> aFunction);

} /* namespace fsdiff */

#endif /* FSDIFF_H_ */
