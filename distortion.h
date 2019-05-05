#ifndef DISTORTION_H
#define DISTORTION_H

#define INV_TAN 			0
#define CUB_NONLIN			1
#define SOFT_CLIP			2
#define HARD_CLIP			3
#define MAX_DISTORTIONS		3

class Distortion {
	
	public:
	void distortion_tick_n (float *x, int n);
	bool bypass = true;
	unsigned int clipType = 0;
	float gain = 0.5;
	float range = 100.0;
	float blend = 0.5;
	float volume = .33;
	int type = 0;
	
	private:
	void distortion_clipSignal(unsigned int type);
	float cleanSig = 0.0;
	float distSig = 0.0;
	float output = 0.0;
	float boostedSig = 0.0;
	float thresh = 1.0/3.0;
	float q = 0.0;
	
};

#endif //DISTORTION_H