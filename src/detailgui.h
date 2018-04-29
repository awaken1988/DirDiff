/*
 * detaildui.h
 *
 *  Created on: Jan 13, 2018
 *      Author: martin
 */

#ifndef DETAILGUI_H_
#define DETAILGUI_H_

#include <QWidget>
#include "fsdiff.h"

namespace detailgui
{
	QWidget* show_detail(fsdiff::diff_t* aDiff);
	QWidget* show_content(fsdiff::diff_t* aDiff);
	QWidget* show_diff(fsdiff::diff_t* aDiff);
	QWidget* show_duplicates(fsdiff::diff_t* aDiff);
}



#endif /* DETAILGUI_H_ */
