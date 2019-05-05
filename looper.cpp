/***** Looper.cpp *****/

#include "looper.h"
#include "Bela.h"
#include <cmath>

void
Looper::looper_set_volume (float volumeControl)
{
	x = volumeControl;
	x1 = 1/(std::exp(shape)-1);
	gain = (std::exp(x*shape)-1)*x1;
}

void
Looper::looper_tick_n (float *x, int n)
{
	for (int i=0; i<n; i++){			//Cycles through audio effects buffer of size n (from context->audioFrames)
		
		input = x[i];					//Input is the current buffer sample x[i]
	
	
		if (loopRec != lastPress)			//Record number of times button has been pushed
			{
				if(loopRec)
					{
						loopCase++;
						if (loopCase==1)
							rt_printf("Recording");
						else if (loopCase==2)
							rt_printf("Recording some more");
						else if (loopCase==3)
							rt_printf("Playing output");
						else if (loopCase==4)
							rt_printf("Recording some more");
						else
							rt_printf("Something went wrong");
					}
			}
		lastPress = loopRec;
		
		
		if (loopClr)				//If loopclear button is pressed, resets buffer
			{
				loopLength = 0;
				loopPointer = 0;
				loopCase = 0;
			}
	
	
		switch(loopCase)	{
			case 0:					//do nothing to signal (default case)
				output = input;
				break;
			
			case 1:					//Record the initial input (sets loop length)
				if (++loopLength>=LOOP_BUFFER_SIZE)
					loopLength = 0;
				loopBuffer[loopLength] = input;
				output = input;
				break;
				
			case 2:					//Layer over initial recording that set loop length
				if (++loopPointer>=loopLength)
					loopPointer = 0;
				output = input + (loopBuffer[loopPointer]*gain);
				loopBuffer[loopPointer] += input;
				break;
				
			case 3:					//Playback input with recorded signal
				if (++loopPointer>=loopLength)
					loopPointer = 0;
				output = input + (loopBuffer[loopPointer]*gain);
				break;
			
			case 4:
				loopCase = 2;
				break;
				
			default:
				break;
		}
		
		x[i] = output;				//New value in effects buffer is sent out
	
		
	}
	
}