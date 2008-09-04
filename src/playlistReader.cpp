/* ***** BEGIN LICENSE BLOCK *****
 *
 * The MIT License
 *
 * Copyright (c) 2008 BBC Research
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

#include <QtGui>

#include <cassert>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <string.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#ifdef Q_OS_WIN32
# define strtok_r(str, delim, saveptr) strtok(str, delim)
#endif

#include "playlistReader.h"
#include "yuvReader.h"
#include "stats.h"

#define DEBUG 0

struct PlayListItem {
	unsigned int frame_num;
	unsigned int num_frames;
	YUVReader *reader;
	unsigned int reader_start;
	QString osdtext;
};

PlayListReader::PlayListReader()
{
	randomAccess = true;
}

void PlayListReader::setFileName(const QString &fn)
{
	/* read in the play list */
	FILE *fd = fopen(fn.toLatin1().data(), "r");
	if (fd < 0)
		perror("Open");

	last_frame_num = 0;
	char buf[1024];
	while (fgets(buf, 1024, fd)) {
		if (buf[0] == '#')
			continue;
		/* XXX handle spaces in file names */
		/* filename type width height fps firstframe lastframe osdtext */
		char *ptr;
		char *name = strtok_r(buf, " ", &ptr);
		char *type = strtok_r(NULL, " ", &ptr);
		char *width = strtok_r(NULL, " ", &ptr);
		char *height = strtok_r(NULL, " ", &ptr);
		char *fps = strtok_r(NULL, " ", &ptr);
		char *start = strtok_r(NULL, " ", &ptr);
		char *end = strtok_r(NULL, " ", &ptr);
		char *osdtext = strtok_r(NULL, " \n", &ptr);

		if (!name || !type || !width || !height || !fps || !start || !end || !type)
			continue;

		PlayListItem *p = new PlayListItem;
		p->frame_num = last_frame_num;
		p->reader_start = atoi(start);
		p->osdtext = osdtext;
		last_frame_num += p->num_frames = atoi(end) - p->reader_start;
		p->num_frames++;
		printf("-> %d: %s %d %d\n", last_frame_num, name, p->reader_start, p->num_frames);

		YUVReader *r = new YUVReader();
		r->setVideoWidth(atoi(width));
		r->setVideoHeight(atoi(height));
		r->setForceFileType(true);
		r->setFileType(type);
		r->setFileName(name);
		r->setFPS(atof(fps));

		p->reader = r;

		play_list.push_back(p);
	}

	fclose(fd);
}

//called from the frame queue controller to get frame data for display
VideoData* PlayListReader::pullFrame(int frame_number_raw)
{
	unsigned frame_number = frame_number_raw % (last_frame_num+1);

	/* find which playlist item that framenumber is in */
	typedef std::list<PlayListItem*> PL;
	for (PL::iterator li = play_list.begin(); li != play_list.end(); li++) {
		if ((frame_number >= (*li)->frame_num)
		&&  (frame_number < (*li)->frame_num + (*li)->num_frames))
		{
			VideoData *ret = (*li)->reader->pullFrame((*li)->reader_start + frame_number - (*li)->frame_num);
			ret->frame_number = frame_number;
			ret->is_first_frame = frame_number == 0;
			ret->is_last_frame = frame_number == last_frame_num;
			return ret;
		}
	}
	assert(0);
}

double PlayListReader::getFPS(int frame_number_raw)
{
	unsigned frame_number = frame_number_raw % (last_frame_num+1);

	/* find which playlist item that framenumber is in */
	typedef std::list<PlayListItem*> PL;
	for (PL::iterator li = play_list.begin(); li != play_list.end(); li++) {
		if ((frame_number >= (*li)->frame_num)
		&&  (frame_number < (*li)->frame_num + (*li)->num_frames))
		{
			return (*li)->reader->getFPS((*li)->reader_start + frame_number - (*li)->frame_num);
		}
	}
	assert(0);
}
