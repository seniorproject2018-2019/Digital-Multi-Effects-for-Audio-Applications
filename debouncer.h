/***** debouncer.h *****/
#ifndef DEBOUNCER_H
#define DEBOUNCER_H

struct Debouncer
{
	unsigned int integrator;
	unsigned int trigger;
	bool state;
	bool toggle;
	bool rising_edge;
	bool falling_edge;
	void debounce_inputs(unsigned int input);
	Debouncer();
};

#endif //DEBOUNCER_H