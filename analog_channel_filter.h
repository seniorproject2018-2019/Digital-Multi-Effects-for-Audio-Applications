/***** analog_channel_filter.h *****/

// Channel_filter stores values from analog inputs.  Filtering is run on these inputs 
// and stored in the filtered_reading array.
// Also includes state variables for detecting whether there was a position change 
// then a timer to set how long after detecting initial change that it will continue
// to set the value to the active effect defined by control context
// Also maintains state variables for switch function detection.

#ifndef ANALOG_CHANNEL_FILTER_H
#define ANALOG_CHANNEL_FILTER_H

#define CF_CUTOFF		8
#define SCAN_PERIOD		50
#define NUM_CHANNELS	8

struct Analog_channel_filter
{
	//Store values read from the analog inputs
	float current_reading;
	float filtered_reading[2];
	float *filtered_buf;
	
	//Evaluate whether a control position has changed
	unsigned int scan_cycle;
	unsigned int scan_timer;
	unsigned int active_timer;
	unsigned int active_reset;
	float last_reading;
	float dVdt;
	
	//first order filter coefficients
	float a;  
	float b;
	float c;
	float d;
	
	//switch function
	bool switch_state;
	bool edge;
	unsigned int knob_toggle_timer;
	
	Analog_channel_filter();
	
	void setup_analog_CF(int numChannels, int frames, int sampleRate, float dpfc);
};


#endif //ANALOG_CHANNEL_FILTER_H