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

#include <cassert>
#include <QtGlobal>

#include <stdlib.h>
#include <stdio.h>

#ifdef Q_OS_WIN32
#include <malloc.h>
#define valloc malloc
#endif

#include "videoData.h"
#include "fourcc.h"

using namespace GLvideo;

DataPtr_valloc::DataPtr_valloc(unsigned length)
{
  ptr = valloc(length);
}

DataPtr_valloc::~DataPtr_valloc()
{
  free(ptr);
}

DataPtr_alias::DataPtr_alias(void* in_ptr)
{
  ptr = in_ptr;
}

PackingFmt fourccToPacking(FourCC fourcc)
{
  switch (fourcc)
  {
    case FOURCC_8p0:
    case FOURCC_420p:
    case FOURCC_i420:
    case FOURCC_yv12:
    case FOURCC_8p2:
    case FOURCC_422p:
    case FOURCC_8p4:
    case FOURCC_444p:
      return V8P;

    case FOURCC_10p0:
    case FOURCC_10p2:
    case FOURCC_10p4:
    case FOURCC_16p0:
    case FOURCC_16p2:
    case FOURCC_16p4:
      return V16P;

    case FOURCC_uyvy:
      return UYVY;

    case FOURCC_v216:
      return V216;

    case FOURCC_v210:
      return V210;
  }

  assert(!"unspported fourcc");
}

ChromaFmt fourccToChroma(FourCC fourcc)
{
  switch (fourcc)
  {
    case FOURCC_8p0:
    case FOURCC_420p:
    case FOURCC_i420:
    case FOURCC_yv12:
    case FOURCC_16p0:
    case FOURCC_10p0:
      return Cr420;

    case FOURCC_8p2:
    case FOURCC_422p:
    case FOURCC_16p2:
    case FOURCC_10p2:
    case FOURCC_uyvy:
    case FOURCC_v216:
    case FOURCC_v210:
      return Cr422;

    case FOURCC_8p4:
    case FOURCC_444p:
    case FOURCC_10p4:
    case FOURCC_16p4:
      return Cr444;
  }

  assert(!"unspported fourcc");
}

unsigned int fourCCToNumberOfInactiveBits(FourCC fourCC)
{
  unsigned int nInactiveBits = 0;

  switch (fourCC)
  {

    case FOURCC_8p0:
    case FOURCC_420p:
    case FOURCC_i420:
    case FOURCC_yv12:
    case FOURCC_16p0:
    case FOURCC_8p2:
    case FOURCC_422p:
    case FOURCC_16p2:
    case FOURCC_uyvy:
    case FOURCC_v216:
    case FOURCC_v210:
    case FOURCC_8p4:
    case FOURCC_444p:
    case FOURCC_16p4:
    {
      nInactiveBits = 0;
      break;
    }

    case FOURCC_10p0:
    case FOURCC_10p2:
    case FOURCC_10p4:
    {
      nInactiveBits = 6;
      break;
    }
  }

  return nInactiveBits;
}

unsigned sizeofFrame(PackingFmt packing, ChromaFmt chroma, unsigned width,
    unsigned height)
{
  switch (packing)
  {
    case V8P:
      switch (chroma)
      {
        case Cr420:
          return width * height * 3 / 2;
        case Cr422:
          return width * height * 2;
        case Cr444:
          return width * height * 3;
        default:
          assert(!"unsupported V8P chroma format");
      }

    case V16P:
      return sizeofFrame(V8P, chroma, width, height) * 2;

    case UYVY:
      assert(chroma == Cr422);
      return width * height * 2;

    case V216:
      return sizeofFrame(UYVY, chroma, width, height) * 2;

    case V210:
    {
      assert(chroma == Cr422);
      /* v210:
       *  - samples are packed into blocks of 48 samples
       *    (the picture is padded to a multiple of this)
       *  - blocks are packed into 128 bytes.
       */
      unsigned padded_w = ((width + 47) / 48) * 48;
      unsigned padded_line_length = (2 * padded_w * 4) / 3;
      return height * padded_line_length;
    }
    default:
      assert(!"unsupported packing format");
  }
}

/* set geometry and data length for all V8P packing formats.
 * modifies pd, returns reference to pd */
PictureData<void>&
setPlaneDimensionsV8P(PictureData<void>& pd, ChromaFmt chroma, unsigned width,
    unsigned height)
{
  pd.plane.resize(3);

  pd.plane[0].width = width;
  pd.plane[0].height = height;

  switch (chroma)
  {
    case Cr420:
      pd.plane[1].width = pd.plane[2].width = width / 2;
      pd.plane[1].height = pd.plane[2].height = height / 2;
      break;
    case Cr422:
      pd.plane[1].width = pd.plane[2].width = width / 2;
      pd.plane[1].height = pd.plane[2].height = height;
      break;
    case Cr444:
      pd.plane[1].width = pd.plane[2].width = width;
      pd.plane[1].height = pd.plane[2].height = height;
      break;
  }

  pd.plane[0].length = pd.plane[0].width * pd.plane[0].height;
  pd.plane[1].length = pd.plane[1].width * pd.plane[1].height;
  pd.plane[2].length = pd.plane[2].width * pd.plane[2].height;

  return pd;
}

/* set geometry and data length for all V16P packing formats.
 * modifies pd, returns reference to pd */
PictureData<void>&
setPlaneDimensionsV16P(PictureData<void>& pd, ChromaFmt chroma, unsigned width,
    unsigned height)
{
  setPlaneDimensionsV8P(pd, chroma, width, height);

  pd.plane[0].length *= 2;
  pd.plane[1].length *= 2;
  pd.plane[2].length *= 2;

  return pd;
}

/* set geometry and data length for all UYVY packing formats.
 * modifies pd, returns reference to pd */
PictureData<void>&
setPlaneDimensionsUYVY(PictureData<void>& pd, ChromaFmt chroma, unsigned width,
    unsigned height)
{
  assert(chroma == Cr422);

  pd.plane.resize(1);

  pd.plane[0].width = width;
  pd.plane[0].height = height;
  pd.plane[0].length = sizeofFrame(UYVY, Cr422, width, height);

  return pd;
}

/* set geometry and data length for all V216 packing formats.
 * modifies pd, returns reference to pd */
PictureData<void>&
setPlaneDimensionsV216(PictureData<void>& pd, ChromaFmt chroma, unsigned width,
    unsigned height)
{
  assert(chroma == Cr422);

  setPlaneDimensionsUYVY(pd, Cr422, width, height);

  pd.plane[0].length *= 2;

  return pd;
}

/* set geometry and data length for all V210 packing formats.
 * modifies pd, returns reference to pd */
PictureData<void>&
setPlaneDimensionsV210(PictureData<void>& pd, ChromaFmt chroma, unsigned width,
    unsigned height)
{
  assert(chroma == Cr422);

  pd.plane.resize(1);

  pd.plane[0].width = width;
  pd.plane[0].height = height;
  pd.plane[0].length = sizeofFrame(V210, Cr422, width, height);

  return pd;
}

PictureData<void>&
setPlaneDimensions(PictureData<void>& pd, PackingFmt packing, ChromaFmt chroma,
    unsigned width, unsigned height)
{
  switch (packing)
  {
    case V8P:
      return setPlaneDimensionsV8P(pd, chroma, width, height);
    case V16P:
      return setPlaneDimensionsV16P(pd, chroma, width, height);
    case UYVY:
      return setPlaneDimensionsUYVY(pd, chroma, width, height);
    case V216:
      return setPlaneDimensionsV216(pd, chroma, width, height);
    case V210:
      return setPlaneDimensionsV210(pd, chroma, width, height);
  }

  assert(!"fail");
}

typedef unsigned int uint_t;
typedef unsigned char uint8_t;

//read the first 10 bit sample from the least significant bits of the 4 bytes pointed to by 'data'
inline uint_t readv210sample_pos2of3(uint8_t* data)
{
  const uint_t lsb = (data[2] & 0xf0) >> 4;
  const uint_t msb = (data[3] & 0x3f) << 4;
  return msb | lsb;
}

//read the second 10 bit sample from the middle bits of the 4 bytes pointed to by 'data'
inline uint_t readv210sample_pos1of3(uint8_t* data)
{
  const uint_t lsb = (data[1] & 0xfc) >> 2;
  const uint_t msb = (data[2] & 0x0f) << 6;
  return msb | lsb;
}

//read the third 10 bit sample from the more significant bits of the 4 bytes pointed to by 'data'
inline uint_t readv210sample_pos0of3(uint8_t* data)
{
  const uint_t lsb = (data[0] & 0xff);
  const uint_t msb = (data[1] & 0x03) << 8;
  return msb | lsb;
}

void unpackv210line(uint8_t* dst, uint8_t* src, uint_t luma_width)
{
  /* number of blocks completely filled with active samples (6 per block) */
  const uint_t num_firstpass_samples = 2 * (luma_width / 6) * 6;
  uint_t x;
  for (x = 0; x < num_firstpass_samples; src += 16)
  {
    dst[x++] = readv210sample_pos0of3(src + 0) >> 2; /* Cb */
    dst[x++] = readv210sample_pos1of3(src + 0) >> 2; /* Y' */
    dst[x++] = readv210sample_pos2of3(src + 0) >> 2; /* Cr */
    dst[x++] = readv210sample_pos0of3(src + 4) >> 2; /* Y' */

    dst[x++] = readv210sample_pos1of3(src + 4) >> 2;
    dst[x++] = readv210sample_pos2of3(src + 4) >> 2;
    dst[x++] = readv210sample_pos0of3(src + 8) >> 2;
    dst[x++] = readv210sample_pos1of3(src + 8) >> 2;

    dst[x++] = readv210sample_pos2of3(src + 8) >> 2;
    dst[x++] = readv210sample_pos0of3(src + 12) >> 2;
    dst[x++] = readv210sample_pos1of3(src + 12) >> 2;
    dst[x++] = readv210sample_pos2of3(src + 12) >> 2;
  }
  /* mop up last subblock (4 or 2 active samples) */

  if (x < 2 * luma_width)
  {
    dst[x++] = readv210sample_pos0of3(src + 0) >> 2; /* Cb */
    dst[x++] = readv210sample_pos1of3(src + 0) >> 2; /* Y' */
    dst[x++] = readv210sample_pos2of3(src + 0) >> 2; /* Cr */
    dst[x++] = readv210sample_pos0of3(src + 4) >> 2; /* Y' */
  }

  if (x < 2 * luma_width)
  {
    dst[x++] = readv210sample_pos1of3(src + 4) >> 2;
    dst[x++] = readv210sample_pos2of3(src + 4) >> 2;
    dst[x++] = readv210sample_pos0of3(src + 8) >> 2;
    dst[x++] = readv210sample_pos1of3(src + 8) >> 2;
  }
}

//utility function to convert V210 data into something easier to display
//it is too difficult to do this in the openGL shader
void convertV210toUYVY(PictureData<DataPtr>& pd)
{
  const int Ywidth = pd.plane[0].width;
  const int Yheight = pd.plane[0].height;

  /* pad to a multiple of 48 luma samples */
  /* nb, 48 luma samples becomes 128 bytes */
  const uint_t padded_w = ((Ywidth + 47) / 48) * 48; //number of luma samples, padded
  const uint_t padded_line_length = (2 * padded_w * 4) / 3; //number of bytes on each line in the file, including padding data

  void* data = pd.plane[0].data->ptr;
  uint8_t *dst = (uint8_t*) data; /* destination for 8bit uyvy, may = src */
  uint8_t *src = (uint8_t*) data; /* source of v210 data */

  for (int y = 0; y < Yheight; y++)
  {
    unpackv210line(dst, src, Ywidth);
    src += padded_line_length;
    dst += Ywidth * 2;
  }

  pd.packing_format = UYVY;
  setPlaneDimensionsUYVY(*(PictureData<void>*) &pd, pd.chroma_format,
      pd.plane[0].width, pd.plane[0].height);
  /* xxx */
}

void convertPlanar16toPlanar8(PictureData<DataPtr>& pd)
{
  assert(pd.packing_format == V16P);

  for (unsigned i = 0; i < pd.plane.size(); i++)
  {
    uint8_t *src = (uint8_t *) pd.plane[i].data->ptr;
    uint8_t *dst = src;
    const uint8_t *end = src + pd.plane[i].length;

    //convert the 16 bit source to 8 bit destination, in place
    //just throw away the LSBs
    while (src < end)
    {
      *dst++ = *src;
      src += 2;
    }
  }

  pd.packing_format = V8P;
  setPlaneDimensionsV8P(*(PictureData<void>*) &pd, pd.chroma_format,
      pd.plane[0].width, pd.plane[0].height);
  /* xxx */
  /* no need to setPlaneData(),
   * since the plane base pointers have not been changed */
}

void shiftPlanar16PixelData(PictureData<DataPtr>& pd, unsigned int bit_shift)
{
  assert(pd.packing_format == V16P);
  //for each plane in the VideoData struct
  for (unsigned int plane_idx = 0; plane_idx < pd.plane.size(); plane_idx++)
  {
    uint16_t *pPixelData = (uint16_t*) pd.plane[plane_idx].data->ptr;
    unsigned int npixels = pd.plane[plane_idx].length / 2;
    //shift each pixel value left by bit_shift
    for (unsigned int pixel = 0; pixel < npixels; pixel++)
    {
      *pPixelData++ <<= bit_shift;
    }
  }
}


void convertV216toUYVY(PictureData<DataPtr>& pd)
{
  assert(pd.packing_format == V216);

  uint8_t *src = (uint8_t *) pd.plane[0].data->ptr;
  uint8_t *dst = src;
  const uint8_t *end = src + pd.plane[0].length;

  //convert the 16 bit source to 8 bit destination, in place
  //just throw away the LSBs
  while (src < end)
  {
    *dst++ = src[1];
    src += 2;
  }

  pd.packing_format = UYVY;
  setPlaneDimensionsUYVY(*(PictureData<void>*) &pd, pd.chroma_format,
      pd.plane[0].width, pd.plane[0].height);
  /* xxx */
  /* no need to setPlaneData(),
   * since the plane base pointers have not been changed */
}
