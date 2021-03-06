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

#include <stdint.h>
#include <QtCore>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#ifdef Q_OS_WIN32
#include <io.h>
#define lseek64 _lseeki64
#define read _read
#define open _open
#define off64_t qint64
#else
#define _O_BINARY 0
#endif

#ifdef Q_OS_MAC
#define lseek64 lseek
#define off64_t off_t
#endif

#include <errno.h>
#include <string.h>

#include "videoData.h"
#include "yuvReader.h"
#include "stats.h"

#include "util.h"

#define DEBUG 0

YUVReader::YUVReader()
{
  randomAccess = true;
  interlacedSource = false;
  bit_shift = 0;
}

void YUVReader::setFileName(const QString &fn)
{
  fileName = fn;
  QFileInfo info(fileName);

  fd = open(fileName.toLatin1().data(), O_RDONLY | _O_BINARY);
  if (fd < 0)
  {
    fprintf(stderr, "Failed to open '%s': %s\n", fileName.toLatin1().data(),
        strerror(errno));
    exit(1);
    return;
  }

  fileType = forceFileType ? fileType.toLower() : info.suffix().toLower();

  FourCC fourcc = qstringToFourCC(fileType);
  packing_format = fourccToPacking(fourcc);
  chroma_format = fourccToChroma(fourcc);
  bit_shift = fourCCToNumberOfInactiveBits(fourcc);
  frame_size = sizeofFrame(packing_format, chroma_format, videoWidth,
      videoHeight);

  firstFrameNum = 0;
  lastFrameNum = info.size() / frame_size;
  lastFrameNum--;
}

//called from the frame queue controller to get frame data for display
VideoData* YUVReader::pullFrame(int frameNumber)
{
  if (DEBUG)
    printf("Getting frame number %d\n", frameNumber);

  //deal with frame number wrapping
  if (frameNumber < 0)
  {
    frameNumber *= -1;
    frameNumber %= (lastFrameNum + 1);
    frameNumber = lastFrameNum - frameNumber;
  }

  if (frameNumber > lastFrameNum)
    frameNumber %= (lastFrameNum + 1);

  /* allocate new storage:
   *  1) work out dimensions for the planes
   *  2) create contiguous storage (we already know the frame_size)
   *  3) create aliases for any other planes
   */
  VideoData* frame = new VideoData();
  frame->data.packing_format = packing_format;
  frame->data.chroma_format = chroma_format;
  setPlaneDimensions(*(PictureData<void>*) &frame->data, packing_format,
      chroma_format, videoWidth, videoHeight);

  frame->data.plane[0].data = std::shared_ptr<DataPtr>(
      new DataPtr_valloc(frame_size));
  void* const data = frame->data.plane[0].data->ptr;
  uint8_t* ptr = (uint8_t*) data;
  for (unsigned i = 1; i < frame->data.plane.size(); i++)
  {
    ptr += frame->data.plane[i - 1].length;
    frame->data.plane[i].data = std::shared_ptr<DataPtr>(new DataPtr_alias(ptr));
  }
  /* xxx: fixup plane numbering if required (eg YV12 vs I420) */

  //set frame number and first/last flags
  frame->frame_number = frameNumber;
  frame->is_first_frame = (frameNumber == firstFrameNum);
  frame->is_last_frame = (frameNumber == lastFrameNum);
  frame->is_interlaced = interlacedSource;

  frame->sample_aspect_ratio_numerator = sample_aspect_ratio_numerator;
  frame->sample_aspect_ratio_denominator = sample_aspect_ratio_denominator;
  frame->data.sar = (float) sample_aspect_ratio_numerator
      / (float) sample_aspect_ratio_denominator;

  //seek to requested frame
  off64_t offset = (off64_t) frame_size * (off64_t) frameNumber;
  off64_t sret = lseek64(fd, offset, SEEK_SET);
  if (!sret && offset != 0)
    perror("LSEEK");

  QTime timer;
  timer.restart();

  for (unsigned done = 0; done < frame_size;)
  {
    int len = read(fd, (uint8_t*) data + done, frame_size - done);
    if (len <= 0)
    {
      std::stringstream ss;
      ss << "read() @ frame" << frameNumber << "(" << offset << "+" << done
          << " of " << frame_size << ")";
      perror(ss.str().c_str());
      break;
    }
    done += len;
  }

  if (bit_shift > 0)
  {
    shiftPlanar16PixelData(frame->data, bit_shift);
  }

  Stats::Section& stats = Stats::getSection("YUV Reader");
  addStat(stats, "Read", timer.elapsed(), "ms");
  addStat(stats, "VideoWidth", videoWidth);
  addStat(stats, "VideoHeight", videoHeight);
  addStat(stats, "FirstFrame", firstFrameNum);
  addStat(stats, "LastFrame", lastFrameNum);
  addStat(stats, "VideoFormat", fileType.toLatin1().data());

  return frame;
}
