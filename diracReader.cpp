/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: diracReader.cpp,v 1.2 2008-03-10 10:20:42 jrosser Exp $
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

#include <common/dirac.h>

#include <parser/Parser.hpp>
#include <parser/AccessUnit.hpp>
#include <parser/IInput.hpp>
#include <parser/StreamReader.hpp>

#include <decoder/Schrodec.hpp>

#include <decoder/FileReader.hpp>
#include <decoder/IWriter.hpp>

#include <decoder/DefaultFrameMemoryManager.hpp>
#include <decoder/FrameManagerMaker.hpp>
#include <decoder/IFrameManager.hpp>

#include <schroedinger/schroframe.h>

#include "diracReader.h"


#include <iostream>
using namespace ::dirac::parser_project;
using namespace ::dirac::decoder;

#define DEBUG 0

//-----------------------------------------------------------------------
class SomeWriter : public IWriter<VideoData> {
public:
	SomeWriter(DiracReader& dr) : dr(&dr) {}
private:
    //from IWriter
    virtual bool        output  ( const_pointer begin, size_type count
                                , VideoData* vid );
    virtual bool        available() const { return true; }		//WTF???

private:
	DiracReader   *dr;
};


//for IWriter (the producer)
#define MAX 10
bool SomeWriter::output ( const_pointer begin, size_type count
                        , VideoData* video ) 
{
  count=count;
	if(DEBUG) printf("(SomeWriter) pushed frame %p\n", video);
	
	//wait for space in the list
	dr->frameMutex.lock();
	int size = dr->frameList.size();
	if(size == MAX) {
	  if(DEBUG)printf("(SomeWriter) list is full - waiting...\n");
	  dr->bufferNotFull.wait(&dr->frameMutex);
  }

	//add to list
	size=dr->frameList.size();

	//printf("Pixels %x %x %x : %x %x %x\n", video->data[500], video->data[501], video->data[502], video->data[1280*50+500], video->data[1280*50+501],video->data[1280*50+502]);

	if(DEBUG) printf("(SomeWriter) adding frame %p, %dx%d, listSize=%d\n", video, video->Ywidth, video->Yheight, size);
	dr->frameList.append(video);
	
	//wake consumer if list was previously empty
	if(dr->frameList.size() == 1) {
	    if(DEBUG)printf("(SomeWriter) list has become not empty - waking DiracReader...\n");
	  dr->bufferNotEmpty.wakeAll();
  }
  
  dr->frameMutex.unlock();
  
  if(DEBUG) printf("(SomeWriter) Done\n");
  
	return true;
}

class SomeFrameMemoryManager : public IFrameMemoryManager<VideoData>
{
public:
  SomeFrameMemoryManager ( FrameQueue& frameQ ) 
  : frameQueue ( frameQ ), decoder ( 0 ) {}
  virtual ~SomeFrameMemoryManager() {}
  
public:
  void setDecoder ( IDecoder* dec ) { decoder = dec; }
  
private:
  SomeFrameMemoryManager (SomeFrameMemoryManager const& );
  SomeFrameMemoryManager& operator= ( SomeFrameMemoryManager const& );

private:
  virtual unsigned char* allocateFrame ( size_t bufferSize, VideoData*& v );
  virtual void releaseFrame ( unsigned char*, VideoData* v ) {}
  
private:
  FrameQueue& frameQueue;
  IDecoder*   decoder;
};

VideoData::DataFmt translateChromaFormat ( ChromaFormat c ) 
{
  return VideoData::V8P0;
}

unsigned char* SomeFrameMemoryManager::allocateFrame ( size_t dummy
                                                     , VideoData*& v)
{
  VideoData* vid = frameQueue.allocateFrame();
  assert ( vid );
  vid->resize (decoder->frameWidth(), decoder->frameHeight()
              , translateChromaFormat ( decoder->chromaFormat() ) );
              
  if(DEBUG) printf("Allocate frame %p, %dx%d\n", vid, vid->Ywidth, vid->Yheight);            
  v = vid;
  return v->data;
}



//called from the transport controller to get frame data for display
//frame number is bogus, as the dirac wrapper cannot seek by frame number
void DiracReader::pullFrame(int frameNumber /*ignored*/, VideoData*& dst)
{
	if(DEBUG) printf("(DiracReader) request for frame number %d\n", frameNumber);
	
	//see if there is a frame on the list
	frameMutex.lock();
	if (frameList.empty()) {
	  if(DEBUG) printf("(DiracReader) frame list is empty - waiting...\n");
		bufferNotEmpty.wait(&frameMutex);
  }
    
  dst = frameList.takeLast();
	
	int size = frameList.size();
  if(DEBUG) printf("(DiracReader) taken frame %p from list, listSize=%d\n", dst, size);
	
	if(frameList.size() == MAX-1) {
	  if(DEBUG) printf("(DiracReader) list has become not full - waking SomeWriter..\n");
	  bufferNotFull.wakeAll();
	}
	
	frameMutex.unlock();
	
  if(DEBUG) printf("(DiracReader) Done\n");
	
}

DiracReader::DiracReader( FrameQueue& frameQ ) : ReaderInterface ( frameQ )
{
	randomAccess = false;
	frameData = NULL;
	go = true;
}

void DiracReader::setFileName(const QString &fn)
{
	fileName = fn;
}

void DiracReader::stop()
{
	go=false;
	wait();
}

void DiracReader::run()
{	
	try {
	    std::auto_ptr<IInput> input ( new FileReader ( std::string((const char *)fileName.toAscii()) ) );
	    std::auto_ptr<StreamReader> reader ( new StreamReader ( input, 50 ) );
	    std::auto_ptr<Parser> parser ( new Parser ( reader ) );
	    std::auto_ptr<IWriter<VideoData> > writer ( new SomeWriter(*this) );
      IFrameMemoryManager<VideoData>* tmp = new SomeFrameMemoryManager ( frameQueue );
	    std::auto_ptr<IFrameMemoryManager<VideoData> > frameMemMgr ( tmp );
	    std::auto_ptr<IFrameManager> manager ( makeFrameManager (writer, frameMemMgr ) );		
	    std::auto_ptr<IDecoder> decoder ( new Schrodec ( parser, manager ) );
      static_cast<SomeFrameMemoryManager*> ( tmp )->setDecoder ( decoder.get() ); // I know it's dodgy, FIXME!!
		
		if ( !decoder.get() ) {
			std::cout << "Programming error: failed to create decoder object" << std::endl;
			return;
		}
	  		
		// If we don't need to interrupt the decoding process we can just invoke
		// decoder->decode();
		std::auto_ptr<IDecoderState> decoderState;
		do {
			
			//decode frames in sequence
			do {
				std::auto_ptr<IDecoderState> tmp ( decoder->getState() );
				decoderState = tmp; // the previous DecoderState gets deleted here

        if(DEBUG) printf("Decoder Iterate...\n");

				decoderState->doNextAction();				
			} while(!decoderState->isEndOfSequence() && go == true);
				
			//loop back to the start of the sequence when we get to the end
			if(go == true)
			{
				printf("Got End of Sequence\n");
				std::auto_ptr<Parser> parser = decoder->releaseParser();
				parser->rewind();
				decoder->reacquireParser(parser);
			}
			
		} while (go == true);    
		//frameCount = decoder->numberOfDecodedFrames();
	}  
	catch ( std::runtime_error& exc ) {
		//std::cout << "Error while decoding " << fileName;
		std::cout << std::endl;
		std::cout << exc.what() << std::endl;
		return;
	}
	catch (...) {
		std::cout << "Unexpected exception" << std::endl;
		return;
	}
}
