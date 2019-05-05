/***** Looper.h *****/
#ifndef LOOPER_H
#define LOOPER_H

#define LOOP_BUFFER_SIZE 7938000


class Looper {
	
	public:
	void looper_tick_n (float *x, int n);
	void looper_set_volume(float volumeControl);
	int loopRec = 0;
	int loopClr = 0;
	int loopCase = 0;

	private:
	int lastPress = 0;
	float loopBuffer[LOOP_BUFFER_SIZE] = {0};
	int loopLength = 0;
	int loopPointer = 0;
	float input = 0.0;
	float output = 0.0;
	float gain = 1.0;
	float shape = 2.0;
	float x = 1.0;
	float x1 = 0.0;

};

#endif //LOOPER_H