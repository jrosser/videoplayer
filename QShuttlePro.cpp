/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: QShuttlePro.cpp,v 1.1 2007-04-10 11:18:52 jrosser Exp $
*
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in compliance
* with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
* the specific language governing rights and limitations under the License.
*
* The Original Code is BBC Research and Development code.
*
* The Initial Developer of the Original Code is the British Broadcasting
* Corporation.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s): Jonathan Rosser
* Inspired by IngexPlayer
* 
* Alternatively, the contents of this file may be used under the terms of
* the GNU General Public License Version 2 (the "GPL"), or the GNU Lesser
* Public License Version 2.1 (the "LGPL"), in which case the provisions of
* the GPL or the LGPL are applicable instead of those above. If you wish to
* allow use of your version of this file only under the terms of the either
* the GPL or LGPL and not to allow others to use your version of this file
* under the MPL, indicate your decision by deleting the provisions above
* and replace them with the notice and other provisions required by the GPL
* or LGPL. If you do not delete the provisions above, a recipient may use
* your version of this file under the terms of any one of the MPL, the GPL
* or the LGPL.
* ***** END LICENSE BLOCK ***** */

#include "QShuttlePro.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>

#define DEBUG 1

/* number of event device files to check */
#define EVENT_DEV_INDEX_MAX     32

/* jog values range is 1..255 */
#define MAX_JOG_VALUE           255

QShuttlePro::QShuttlePro() 
{
	if(DEBUG) printf("Starting ShuttlePro\n");
	int found = openShuttle();
	
	jogvalue = 0;
	shuttlevalue = 0;
	
	//start worker thread if we found the shuttle
	if(found) start();
}

int QShuttlePro::openShuttle()
{
	const char* g_eventDeviceTemplate = "/dev/input/event%d";

	const ShuttleData g_supportedShuttles[] = 
	{
    	{0x0b33, 0x0010, "Contour ShuttlePro"},
    	{0x0b33, 0x0030, "Contour ShuttlePro V2"}
	};

	int numSupportedShuttles = sizeof(g_supportedShuttles) / sizeof(ShuttleData);

    struct input_id deviceInfo;
    char eventDevName[256];
    
    int grab = 1;
    int foundIt=0;
    
    /* go through the event devices and check whether it is a supported shuttle */
    for (int i = 0; i < EVENT_DEV_INDEX_MAX; i++)
    {

        sprintf(eventDevName, g_eventDeviceTemplate, i);
    		        
        if ((fd = open(eventDevName, O_RDONLY)) != -1)
        {
            /* get the device information */
            if (ioctl(fd, EVIOCGID, &deviceInfo) >= 0)
            {
                for (int j = 0; j < numSupportedShuttles; j++)
                {               	
                    /* check the vendor and product id */
                    if (g_supportedShuttles[j].vendor == deviceInfo.vendor &&
                        g_supportedShuttles[j].product == deviceInfo.product)
                    {
                        /* try grab the device for exclusive access */
                        if (ioctl(fd, EVIOCGRAB, &grab) != 0)
                        {
                            printf("Failed to grab jog-shuttle device for exclusive access\n");
                        }
                        
                        if(DEBUG) printf("Found shuttle at %s\n", eventDevName);
                        
                        foundIt = 1;
                        goto exit;
                    }
                }
            }
            
        }
    }

exit:
    return foundIt;
}

void QShuttlePro::shuttle(int value)
{
	gettimeofday( &lastshuttle, 0 );
	need_synthetic_shuttle = value != 0;

	if( value == shuttlevalue )
		return;
		
	shuttlevalue = value;
	emit(shuttleChanged(shuttlevalue));

	switch(value) {
		case 7:	emit(shuttleRight7()); break;
		case 6: emit(shuttleRight6()); break;
		case 5: emit(shuttleRight5()); break;
		case 4: emit(shuttleRight4()); break;
		case 3: emit(shuttleRight3()); break;
		case 2: emit(shuttleRight2()); break;
		case 1: emit(shuttleRight1()); break;
		case 0: emit(shuttleCenter()); break;
		case -1:emit(shuttleLeft1()); break;
		case -2:emit(shuttleLeft2()); break;
		case -3:emit(shuttleLeft3()); break;
		case -4:emit(shuttleLeft4()); break;
		case -5:emit(shuttleLeft5()); break;
		case -6:emit(shuttleLeft6()); break;
		case -7:emit(shuttleLeft7()); break;
		default:
			break;
	}

	if(DEBUG) printf("Shuttle value is %d\n", shuttlevalue);
}


/*
 * Due to a bug (?) in the way Linux HID handles the ShuttlePro, the
 * center position is not reported for the shuttle wheel.  Instead,
 * a jog event is generated immediately when it returns.  We check to
 * see if the time since the last shuttle was more than a few ms ago
 * and generate a shuttle of 0 if so.
 */
void QShuttlePro::check_shuttle_center()
{
	if( !need_synthetic_shuttle )
		return;

	struct timeval now, delta;
	gettimeofday( &now, 0 );
	timersub( &now, &lastshuttle, &delta );

	if( delta.tv_sec < 1 && delta.tv_usec < 5000 )
		return;
	
	shuttle( 0 );
	need_synthetic_shuttle = 0;
}


void QShuttlePro::jog(unsigned int value)
{
	// We should generate a synthetic event for the shuttle going
	// to the home position if we have not seen one recently
	check_shuttle_center();

	if(value == jogvalue)
		return;

	if( jogvalue != 0xffff )
	{
		if ((value + 1 == jogvalue) || (jogvalue == 1 && value == MAX_JOG_VALUE)) {
			if(DEBUG) printf("Jog backward, %d\n", value);
			emit(jogBackward());
		}			
		else if ((value == jogvalue + 1) || (jogvalue == MAX_JOG_VALUE && value == 1)) {
			if(DEBUG) printf("Jog forward, %d\n", value);
			emit(jogForward());
		}
		else {
			if(DEBUG) printf("Jog value is confused!\n");
		}
	}

	jogvalue = value;
}


void QShuttlePro::process_event(input_event inEvent)
{
	switch(inEvent.type) {
		case EV_SYN:	//sync
			if(DEBUG) printf("Sync event\n");
			break;
			
		case EV_REL:	//jog or shuttle			
			if(inEvent.code == REL_DIAL)
				jog(inEvent.value);
				
			if(inEvent.code == REL_WHEEL)
				shuttle(inEvent.value);
				
			break;
			
		case EV_KEY:	//button
			if(inEvent.value)
				emit(keyPressed(inEvent.code));
			else
				emit(keyReleased(inEvent.code));
				
			emit(keyChanged(inEvent.value, inEvent.code));
			if(DEBUG) printf("Key Event, value=%d, code=%d\n", inEvent.value, inEvent.code);
			break;
			
		default:
			printf("Unhandled event %d\n", inEvent.type);	
	}	
}

void QShuttlePro::run()
{
	printf("StartShuttle");
	
    struct input_event inEvent; 
    fd_set rfds;
    struct timeval tv;
    ssize_t numRead;
        
    while (1 /*!stopped*/)
    {    	
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        /* TODO: is there a way to find out how many sec between sync events? */
        tv.tv_sec = 4; /* > sync events interval. */
        tv.tv_usec = 0;
            
        int retval = select(fd + 1, &rfds, NULL, NULL, &tv);

        if (retval == -1)
        {
            /* select error */
            continue;
        }
        else if (retval)
        {
            numRead = read(fd, &inEvent, sizeof(struct input_event));
            if (numRead > 0)
            {
  				process_event(inEvent);
            }
        }
        else
        {
            /* handle silence */            
            //if (handle_silence(&outEvent))
            //{
                ///* call the listeners */
                //for (i = 0; i < MAX_LISTENERS; i++)
                //{
                //    if (shuttle->listeners[i].func != NULL)
                //    {
                //        shuttle->listeners[i].func(shuttle->listeners[i].data, &outEvent);
                //    }
                //}
            //}
        }        
    }
}

