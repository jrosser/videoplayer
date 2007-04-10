#include <QtGui>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "readThread.h"
#include "videoRead.h"

#define DEBUG 1
#define LISTLEN 10

ReadThread::ReadThread(VideoRead &v) 
      : QThread(), vr(v)
{
	m_doReading = true;
}
    
void ReadThread::stop()
{
	m_doReading = false;
}

void ReadThread::addPastFrames()
{
	//add extra frames to the past frames list
	while(numPastFrames < LISTLEN) {
								
		int lastPastFrame;								
		QMutexLocker listLocker(&vr.listMutex);
				
		if(vr.transportSpeed != speed) {								//the playback speed has changed - bail out early
			if(DEBUG) printf("Transport speed changed - aborting read\n");
			break;
		}
								
		if(vr.pastFrames.size() > 0)
			lastPastFrame = vr.pastFrames.last()->frameNum;				//get the number of the last future frame
		else
			lastPastFrame = vr.currentFrameNum;
					
		listLocker.unlock();
																
		int wantedFrame = lastPastFrame - speed;
		if(wantedFrame < 0) wantedFrame += (lastFrame + 1);
		wantedFrame %= lastFrame + 1;
				
		if(DEBUG) printf("Getting past frame %d\n", wantedFrame);
												
		VideoData *newFrame = new VideoData(videoWidth, videoHeight, dataFormat);				
		newFrame->frameNum = wantedFrame;
								
		off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;								
		off64_t sret = lseek64(fd, offset, SEEK_SET);
		if(!sret && offset != 0) perror("LSEEK");
																				
		int rret = read(fd, newFrame->data, newFrame->dataSize);						
		if(!rret) perror("READ");
									
		listLocker.relock();
		vr.pastFrames.append(newFrame);
		numPastFrames = vr.pastFrames.size();
		listLocker.unlock();				
	}		
}

void ReadThread::addFutureFrames()
{
	while(numFutureFrames < LISTLEN) {
				
		int lastFutureFrame;		
		QMutexLocker listLocker(&vr.listMutex);

		if(vr.transportSpeed != speed) {								//the playback speed has changed - bail out early
			if(DEBUG) printf("Transport speed changed - aborting read\n");
			break;
		}

		if(vr.futureFrames.size() > 0)
			lastFutureFrame = vr.futureFrames.last()->frameNum;		//get the number of the last future frame
		else
			lastFutureFrame = vr.currentFrameNum - 1;

		listLocker.unlock();
				
		int wantedFrame = lastFutureFrame + speed;			//work out the number of the next future frame				
		wantedFrame %= lastFrame + 1;						//wrap from the end of the file to the start
								
		if(DEBUG) printf("Getting future frame %d\n", wantedFrame);
		VideoData *newFrame = new VideoData(videoWidth, videoHeight, dataFormat);	//allocate for a new frame
		newFrame->frameNum = wantedFrame;												

		off64_t offset = (off_t)newFrame->dataSize * (off_t)wantedFrame;	//seek to the wanted frame								
		off64_t sret = lseek64(fd, offset, SEEK_SET);
		if(!sret && offset != 0) perror("LSEEK");
												
		int rret = read(fd, newFrame->data, newFrame->dataSize);						
		if(!rret) perror("READ");
										
		listLocker.relock();
		vr.futureFrames.append(newFrame);
		numFutureFrames = vr.futureFrames.size();
		listLocker.unlock();				
	}	
}
    
void ReadThread::run()
{
	printf("Starting readThread\n");
	fd = 0;
	    
	while(m_doReading) {
						
		//get the transport status
		QMutexLocker transportLocker(&vr.transportMutex);
		VideoRead::TransportControls ts = vr.transportStatus;
		speed = vr.transportSpeed;
		transportLocker.unlock();

		if(fd <= 0) {
			if(DEBUG) printf("Opening file %s\n", vr.fileName.toLatin1().data());
			fd = open(vr.fileName.toLatin1().data(), O_RDONLY | O_DIRECT);
			
			if(fd < 0) { 
				printf("[%s]\n", vr.fileName.toLatin1().data());
				perror("OPEN");
			}
		}

		if(fd > 0) {

			//------------------------------------------------------------------------------------------------
			//trash the contents of the frame lists when we change speed
			//this could be made MUCH cleverer - to keep past and future frames that we need when changing speed
			if(speed != lastSpeed) {
				
				printf("Trashing frame lists - ts=%d speed=%d, last=%d\n", ts, speed, lastSpeed);

				QMutexLocker listLocker(&vr.listMutex);
								
				while(vr.futureFrames.size()) {
					VideoData *frame = vr.futureFrames.takeLast();
					delete frame;	
				}

				while(vr.pastFrames.size()) {
					VideoData *frame = vr.pastFrames.takeLast();
					delete frame;	
				}
								
				listLocker.unlock();
				
			}

			lastSpeed = speed;
			
			//------------------------------------------------------------------------------------------------
			//get information about the length of the lists
			{
				QMutexLocker listLocker(&vr.listMutex);
			
				//info about the lists of frames		
				numFutureFrames = vr.futureFrames.size();
				numPastFrames = vr.pastFrames.size();
		
				//the extents of the sequence
				firstFrame = vr.firstFrameNum;
				lastFrame = vr.lastFrameNum;
							
				//the format and dimensions of the sequence  - which mutex should protect theses properly???
				dataFormat = vr.dataFormat;
				videoWidth = vr.videoWidth;
				videoHeight = vr.videoHeight;
			
				listLocker.unlock();
			}

			//------------------------------------------------------------------------------------------------
			//make sure the lists are long enough
			
			//add extra frames to the future frames list when playing forward
			if(ts == VideoRead::Fwd100 || ts == VideoRead::Fwd50 || ts == VideoRead::Fwd20 || ts == VideoRead::Fwd10 || 
			   ts == VideoRead::Fwd5   || ts == VideoRead::Fwd2  || ts == VideoRead::Fwd1) {
				addFutureFrames();
			   }
			 
			//add extra frames to the past frame list when playing backward   
			if(ts == VideoRead::Rev100 || ts == VideoRead::Rev50 || ts == VideoRead::Rev20 || ts == VideoRead::Rev10 || 
			   ts == VideoRead::Rev5   || ts == VideoRead::Rev2  || ts == VideoRead::Rev1) {
				addPastFrames();
			   }

			//when pasued or stopped fill both lists for nice jog-wheel response
			if(ts == VideoRead::Pause || ts == VideoRead::Stop) {
				addFutureFrames();
				addPastFrames();
			}
				
			//------------------------------------------------------------------------------------------------
			//make sure the lists are not too long			
			//remove excess frames from the future list, which accumulate when playing backwards
			while(numFutureFrames > LISTLEN) {
				
				QMutexLocker listLocker(&vr.listMutex);
				VideoData *oldFrame = vr.futureFrames.takeLast();
				numFutureFrames = vr.futureFrames.size();
				delete oldFrame;
				listLocker.unlock();				
			}
			
			//remove excess frames from the past list, which accumulate when playing forward					
			while(numPastFrames > LISTLEN) {
								
				QMutexLocker listLocker(&vr.listMutex);
				VideoData *oldFrame = vr.pastFrames.takeLast();
				numPastFrames = vr.pastFrames.size();
				delete oldFrame;
				listLocker.unlock();							
			}
		}

		vr.frameConsumed.wait(&(vr.frameMutex));						
	}
}
