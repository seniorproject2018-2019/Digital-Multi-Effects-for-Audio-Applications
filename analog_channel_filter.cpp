/***** analog_channel_filter.cpp *****/
#include "analog_channel_filter.h"
#include <math.h>
#include <stdlib.h>
#include <Bela.h>

Analog_channel_filter::Analog_channel_filter()
{

};

void
Analog_channel_filter::setup_analog_CF(int numChannels, int frames, int sampleRate, float dpfc)
{
	
	current_reading = 0.25;
	filtered_reading[0] = 0.5;
	filtered_reading[1] = 0.5;
	filtered_buf = (float*) malloc(sizeof(float)*frames);
	a = dpfc/(dpfc + 1.0);
	b = 1.0 - a;
	c = 0.5*dpfc/(dpfc + 1.0);
	d = 1.0 - c;
		
		//Knob switch functions
	switch_state = true;
	edge = false;
	knob_toggle_timer = 0;

		//State change detection
	active_timer = 0;
	active_reset = (int)(2*sampleRate)/(numChannels * frames);
	scan_cycle = ((int) sampleRate) * SCAN_PERIOD / (1000 * numChannels * frames);
	scan_timer = 0;
	last_reading = 0.5;
	dVdt = 0.0175;
}