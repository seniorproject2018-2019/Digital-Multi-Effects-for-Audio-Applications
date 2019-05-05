/***** volume.h *****/
#ifndef VOLUME_H
#define VOLUME_H

class Volume {
	
	public:
	void volume_pedal_tick_n (float *x, float *exp_Pedal, int n);
	bool bypass = true;
	
	private:
	float gain = 1.0;
	float shape = 5.0;
	float a1 = 0.0;
	
};

#endif //VOLUME_H