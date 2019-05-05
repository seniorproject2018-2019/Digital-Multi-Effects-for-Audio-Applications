/***** debouncer.cpp *****/
#include "debouncer.h"


Debouncer::Debouncer()
{
	integrator = 0;
	trigger = 10;
	state = false;
	toggle = false;
	rising_edge = false;
	falling_edge = false;
}


void 
Debouncer::debounce_inputs(unsigned int input)
{
	if(input) 
	{
		if(integrator < trigger) 
			integrator += 1;
	} 
	else 
	{
		if(integrator > 0) 
			integrator -= 1;
	}
	
	if(integrator == trigger)
	{
		rising_edge = false;
		if(state == false)
		{
			rising_edge = true;
			toggle = !(toggle);
			
			if(toggle == true) 
			{
			}
			if(toggle == false) 
			{
			}
		}
		state = true;
	}
		
	if(integrator == 0) 
	{
		falling_edge = false;
		if(state == true)
		{
			falling_edge = true;
		}
		state = false;
	}
}