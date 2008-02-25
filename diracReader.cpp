/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: diracReader.cpp,v 1.1 2008-02-25 15:08:05 jrosser Exp $
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

#include <samples/Schrodec.hpp>
#include <samples/FileReader.hpp>
#include <samples/FileWriter.hpp>

#include <schroedinger/schroframe.h>

#include "diracReader.h"

#include <iostream>
using namespace ::dirac::parser_project;
using namespace ::dirac::samples;

#define DEBUG 0

class SomeWriter : public IWriter {
public:
	SomeWriter(DiracReader& dr) : dr(&dr) {}
private:
    //from IWriter
    virtual bool        output ( const_pointer begin, size_type count );
    virtual bool        available() const { return true; }		//WTF???

private:
	DiracReader *dr;
};

//for IWriter (the producer)
bool SomeWriter::output ( const_pointer begin, size_type count ) 
{
	if(DEBUG) printf("(schro) pushed frame %p\n", begin);
	
	dr->frameMutex.lock();
	if(DEBUG) printf("(SomeWriter) adding frame %p\n", begin);
	dr->frameData = (unsigned char *)begin;
	dr->bufferNotEmpty.wakeAll();

	dr->bufferNotFull.wait(&dr->frameMutex);
	dr->frameMutex.unlock();
	
	return true;
}

//called from the transport controller to get frame data for display
//frame number is bogus, as the dirac wrapper cannot seek by frame number
void DiracReader::getFrame(int frameNumber, VideoData *dst)
{
	if(DEBUG) printf("(DiracReader) request for frame number %d\n", frameNumber);
	
	frameMutex.lock();
	if (frameData == NULL)
		bufferNotEmpty.wait(&frameMutex);

	/* some copying */
	dst->resize(frameWidth, frameHeight, VideoData::V8P0);
	memcpy(dst->data, frameData, dst->dataSize);
	dst->frameNum = 0;
	frameData = NULL;
	if(DEBUG) printf("(DiracReader) got frame number %d\n", frameNumber);
	
	bufferNotFull.wakeAll();
	frameMutex.unlock();
}

DiracReader::DiracReader()
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
	    std::auto_ptr<IWriter> writer ( new SomeWriter(*this) );		
		std::auto_ptr<IDecoder> decoder ( new Schrodec ( parser, writer ) );
		
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

				//this should go somewhere else
				frameWidth = decoder->frameWidth();
				frameHeight = decoder->frameHeight();		
				
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
