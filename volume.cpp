/***** volume.cpp *****/

#include "volume.h"
#include <cmath>

void
Volume::volume_pedal_tick_n (float *x, float *exp_Pedal, int n)
{
	if (!bypass)
	{
		for (int i=0; i<n; i++)
		{
			float *a = exp_Pedal;
			a1 = 1/(std::exp(shape)-1);
			gain = (std::exp(a[i]*shape)-1)*a1;
			x[i] *= gain;
		}
	}
	
	else
		return;
	
}