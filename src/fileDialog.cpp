/* ***** BEGIN LICENSE BLOCK *****
 *
 * The MIT License
 *
 * Copyright (c) 2010 BBC Research
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ***** END LICENSE BLOCK ***** */

#include <memory>

#include "fileDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>

using namespace std;

VideoFileDialog::VideoFileDialog(QWidget* parent)
	: QFileDialog(parent, tr("Select file to play"))
{
}

int VideoFileDialog::exec(QString& fileName, QString& fileType, int& width, int& height, bool& interlaced)
{
	//call function when user clicks on a file to set file format (if possible)
	connect(this, SIGNAL(currentChanged(QString)), this, SLOT(selectionChanged(QString)));

	QStringList filters;
	filters << "All supported files (*.yuv *.i420 *.yv12 *.420p *.422p *.444p *.uyvy *.v210 *.v216 *.16p0 *.16p2 *.16p4 *.10p0 *.10p2 *.10p4 *.plst)"
	        << "Y'CbCr video files (*.yuv *.i420 *.yv12 *.420p *.422p *.444p *.uyvy *.v210 *.v216 *.16p0 *.16p2 *.16p4 *.10p0 *.10p2 *.10p4)"
	        << "Playlist (*.plst)"
	        << "All files (*)";
	setNameFilters(filters);

	setFileMode(QFileDialog::ExistingFile);

	QGridLayout gridExtras;
	QIntValidator validate_range(1, 8192, this);

	QLabel labelW(tr("Video Width"));
	gridExtras.addWidget(&labelW, 0,  0);

	QLineEdit editWidth;
	editWidth.setValidator(&validate_range);
	gridExtras.addWidget(&editWidth, 0, 1);
	editWidth.setText(QString().sprintf("%d", width));

	QLabel labelH(tr("Video Height"));
	gridExtras.addWidget(&labelH, 0,  2);

	QLineEdit editHeight;
	editHeight.setValidator(&validate_range);
	gridExtras.addWidget(&editHeight, 0, 3);
	editHeight.setText(QString().sprintf("%d", height));

	QLabel labelF(tr("Format"));
	gridExtras.addWidget(&labelF, 1,  0);

	auto_ptr<QComboBox> auto_combo_type(new QComboBox());
	comboType = auto_combo_type.get();
	comboType->setEditable(false);
	comboType->insertItem(999, "i420  (4:2:0 Y'CbCr 8 bit planar)");
	comboType->insertItem(999, "yv12  (4:2:0 Y'CrCb 8 bit planar)");
	comboType->insertItem(999, "420p  (4:2:0 Y'CbCr 8 bit planar)");
	comboType->insertItem(999, "422p  (4:2:2 Y'CbCr 8 bit planar)");
	comboType->insertItem(999, "444p  (4:4:4 Y'CbCr 8 bit planar)");
	comboType->insertItem(999, "uyvy  (4:2:2 Y'CbCr 8 bit packed)");
	comboType->insertItem(999, "v210  (4:2:2 Y'CbCr 10 bit packed)");
	comboType->insertItem(999, "v216  (4:2:2 Y'CbCr 16 bit packed)");
	comboType->insertItem(999, "16p0  (4:2:0 Y'CbCr 16 bit planar)");
	comboType->insertItem(999, "16p2  (4:2:2 Y'CbCr 16 bit planar)");
	comboType->insertItem(999, "16p4  (4:4:4 Y'CbCr 16 bit planar)");
    comboType->insertItem(999, "10p0  (4:2:0 Y'CbCr 10 bit planar)");
    comboType->insertItem(999, "10p2  (4:2:2 Y'CbCr 10 bit planar)");
    comboType->insertItem(999, "10p4  (4:4:4 Y'CbCr 10 bit planar)");
	comboType->insertItem(999, "plst  (Playlist)");
	gridExtras.addWidget(comboType, 1, 1, 1, 2);

	QCheckBox checkInterlaced(tr("Interlaced"));
	checkInterlaced.setChecked(interlaced);
	gridExtras.addWidget(&checkInterlaced, 1, 3);

	//get hold of layout manager for standard FileOpen dialog so we can add our
	//extra widgets
	QGridLayout *dlgLayout = (QGridLayout*)layout();
	dlgLayout->addLayout(&gridExtras, dlgLayout->rowCount(), 0, 1, 3);

	//display the dialog to user
	int result = QDialog::exec();

	//extract results from controls
	QStringList names = selectedFiles();

	if (names.empty()) {
		result = QDialog::Rejected; //no name specified -> cancel dialog
	} else {
		fileName = names.first();
	}

	width = editWidth.text().toInt();
	height = editHeight.text().toInt();
	interlaced = checkInterlaced.isChecked();

	//keep just file format extension, discard description
	fileType = comboType->currentText();
	fileType = fileType.left(fileType.indexOf(' '));

	return result;
}


// When the user clicks on a file, this selects the appropriate file format
// from the ComboBox if there is a match for the file extension
void VideoFileDialog::selectionChanged(const QString& newName)
{
	QFileInfo fi(newName);

	if (fi.suffix().isEmpty() == false) {
		int indx = comboType->findText(fi.suffix(), Qt::MatchStartsWith);

		if (indx != -1)
			comboType->setCurrentIndex(indx);
	}
}

