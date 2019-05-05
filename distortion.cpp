#include "distortion.h"
#include <cmath>

void
Distortion::distortion_clipSignal(unsigned int type)
{
	boostedSig = cleanSig+.005;
	boostedSig *= gain*range;
	
	switch (type)
	{
		case INV_TAN:
			distSig = ((2/3.14159)*std::atan(boostedSig));
			distSig *= 2.0/3.0;
			break;
		
		case CUB_NONLIN:
			if (boostedSig<=-1.0)
				distSig = -2.0/3.0;
			else if (boostedSig>-1 && boostedSig<1)
					distSig = (boostedSig) - ((1.0/3.0) * (boostedSig*boostedSig*boostedSig));
			else if (boostedSig>=1)
					distSig = 2.0/3.0;
			break;
			
		case SOFT_CLIP:
			if (fabs(boostedSig)<thresh)
				distSig = 2.0*boostedSig;
			if (fabs(boostedSig)>=thresh)
			{
				if (boostedSig>0)
					distSig = (3.0-((2.0-(3.0*boostedSig))*(2.0-(3.0*boostedSig))))/3.0;
				if (boostedSig<0)
					distSig = -(3.0-((2.0-(3.0*fabs(boostedSig)))*(2.0-(3.0*fabs(boostedSig)))))/3.0;
			}
			if (fabs(boostedSig)>2*thresh)
			{
				if (boostedSig>0)
					distSig = 1;
				if (boostedSig<0)
					distSig = -1;
			}
			distSig *= 2.0/3.0;
			break;
			
		case HARD_CLIP:
			q = boostedSig/fabs(boostedSig);
			distSig = -q*(1-exp(q*boostedSig));
			break;
	}
}

void
Distortion::distortion_tick_n (float* x, int n)
{
	if (!bypass)
	{
		for (int i = 0; i<n; i++)
		{
			cleanSig = x[i];
			distortion_clipSignal(type);
			output = (((distSig*blend)+(cleanSig*(1-blend)))/2.0)*volume;
			x[i] = output;
		}
	}
	
	else
		return;
}