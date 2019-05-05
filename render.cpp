#include <Bela.h>
#include <stdlib.h>
#include <math.h>
#include <Scope.h>
#include <stdio.h>

#include "flange.h"
#include "lfo.h"
#include "wah.h"
#include "reverb.h"
#include "phaser.h"
#include "trem.h"
#include "Sustainer.h"
#include "fb_compressor.h"
#include "eq.h"
#include "envelope_filter.h"
#include "looper.h"
#include "volume.h"
#include "distortion.h"
#include "LiquidCrystal_I2C_Bela.h"
#include "debouncer.h"
#include "analog_channel_filter.h"

unsigned int gAudioFramesPerAnalogFrame;
float gSampleRate = 44100.0;				//Sample rate of the Bela
float gifs = 1.0/gSampleRate;				//Sample time of the Bela



// ------------  Effects Processing Entities  ------------  //

/*
*	Channel 0 effects
*/

	// Volume pedal
Volume volume;
	// Feedback Compressor
feedback_compressor* fbcompressor;
    // Graphic equalizer
eq_filters *geq1;
    // Guitar Sustainer
Sustainer sustainer(gSampleRate);
    // Envelope Filter
env_filter* ef;
float *gMaster_Envelope;
	// Wah-wah
iwah_coeffs* wah;
    // Tremolo effect
trem_coeffs* tremolo;


/*
*	Channel 1 effects
*/

	// Distortions
Distortion distortion;
    // Phaser
phaser_coeffs* phaser;
	// Chorus
tflanger* chorus;
    // Flanger
tflanger* flanger;
    // Delay/echo effect
tflanger* delay;
    // Reverb
Reverb reverb;
	// Looper
Looper looper;

//END effects processing entities




// ------------  Other useful entities  ------------  //

#define N_PBTNS 5
#define ANALOG_CHANNELS 8

	// LCD Screen
LiquidCrystal_I2C lcd(0x3F, 20, 4);
	// Bela oscilloscope
Scope scope;
	// Debouncer
Debouncer pushbuttons[N_PBTNS];
	//Analog channel Filter
Analog_channel_filter knobs[ANALOG_CHANNELS];

//END other useful entities




//Push button numbers
#define EFFECT_SCROLL_SWITCH  		0
#define EFFECT_BYPASS_SWITCH		1
#define PARAMETER_SCROLL_SWITCH		2
#define LOOP_REC_SWITCH				3
#define LOOP_CLR_SWITCH				4



//Which effect is actively recieving controls
#define FLANGER_FX_CNTRL			0
#define CHORUS_FX_CNTRL				1
#define DELAY_FX_CNTRL				2
#define PHASER_FX_CNTRL				3
#define REVERB_FX_CNTRL				4
#define TREMOLO_FX_CNTRL			5
#define WAH_FX_CNTRL				6
#define DISTORTION_FX_CNTRL			7
#define SUSTAINER_FX_CNTRL			8
#define EQ_FX_CNTRL					9
#define ENVF_FX_CNTRL				10
#define FBCOMP_FX_CNTRL				11
#define LOOPER_FX_CNTRL				12
#define VOLUME_FX_CNTRL				13
#define NUMBER_FX					14
//Update number of NUMBER_FX when adding effects
//Always max in list + 1


//Parameter control pages
#define PARAMETER_PG1				0
#define PARAMETER_PG2				1
#define PARAMETER_PG3				2
#define PARAMETER_PG4				3

unsigned int gRoundRobin = 0;
unsigned int gScanTimer = 0;

//Modulated delay line effects max delay times
#define T_ECHO		5.0
#define T_FLANGE	0.02
#define T_CHORUS	0.1

//LFO type for effect
unsigned int gDLY_lfo_type;
unsigned int gCHOR_lfo_type;
unsigned int gFLANGE_lfo_type;

//Buffers that holds samples that are being processed by effects
float *ch0, *ch1;
int gNframes = 0;



// ------------  Paramter control page mappings ------------  //

	// Modulated delay line control page 1
	// LFO control parameters
#define LFO_RATE 			0
#define LFO_DEPTH 			1
#define LFO_WIDTH 			2
#define MIX_WET 			3
#define REGEN 				4
#define DAMPING				5

	// Modulated delay line control page 2
	// Envelope control parameters
#define ENV_SNS_LFO_RATE	0 
#define ENV_SNS_LFO_DEPTH 	1
#define ENV_SNS_LFO_WIDTH 	2
#define ENV_SNS_MIX_WET 	3
#define ENV_SNS_REGEN 		4
#define LFO_TYPE			5

	// Modulated delay line control page 3
	// More envelope controls
#define ENV_SNS				0
#define ENV_ATK				1 
#define ENV_RLS				2


	// Wah-wah page 1
#define WAH_CKT				0
#define EXP_PEDAL			7


	// Reverb control page 1
#define	REVERB_RTLOW		0
#define REVERB_RTMID		1
#define REVERB_IDLY			2
#define REVERB_MIX			3
#define REVERB_XOV			4
#define REVERB_FDAMP		5

	//  Reverb control page 2
#define REVERB_EQL_F		0
#define REVERB_EQL_G		1
#define REVERB_EQH_F		2
#define REVERB_EQH_G		3


	//  Tremolo page 1 (uses LFO paramters from modulated delay line)
#define TREMOLO_LFO_TYPE	4


	//  Phaser page 1 (uses LFO paramters from modulated delay line)
#define PHASER_STAGES		5


	//  Sustainer page 1
#define SUSTAIN_GAIN		0
#define SUSTAIN_DRIVE		1


   //  Feedback Compressor page 1
#define FB_COMP_THRS		0
#define FB_COMP_RATIO		1
#define FB_COMP_LEVEL		2
#define FB_COMP_MIX			3
#define FB_COMP_ATK			4
#define FB_COMP_RLS			5


	//  Envelope filter page 1 extras
#define EF_DET_SNS			5

	//  Envelope filter page 2 
#define EF_MIX_LP			0
#define EF_MIX_BP			1
#define EF_MIX_HP			2
#define EF_ATK				3
#define EF_RLS				4
#define EF_DIST				5

	//  Envelope filter page 3  -- Envelope gating and sequenced modulator mixing
#define EF_GATE_THRS		0
#define EF_GATE_KNEE		1
#define EF_SH_TRANS			2  //Hard/soft transition when ADSR not active
#define EF_SH_MIX			3	//Mix sample/hold modulator
#define EF_SH_TYPE          5

	//  Envelope filter page 4 -- ADSR control
#define EF_ADSR_ATK			0
#define EF_ADSR_DCY			1
#define EF_ADSR_RLS			2
#define EF_ADSR_STN			3


	//  Looper page 1
#define LOOP_VOL			0


	//  Distortion page 1
#define DIST_GAIN			0
#define DIST_RANGE 			1
#define DIST_BLEND			2
#define DIST_VOLUME 		3
#define DIST_TYPE 			4

// ----------  End Control page context mappings ----------  //



//      Schmitt trigger thresholds
#define SCHMITT_LOW 0.02     //Twist knob all the way down
#define SCHMITT_HIGH 0.98    //Then twist it all the way back up to activate switch state

// Amount of time you have to twist all the way down and then back up
#define KNOB_SWITCH_TIMEOUT 2  // Seconds
unsigned int gKnobToggleTimeout = 44100/8;


#define T_SW_SCAN 5 //ms
unsigned int gSW_timer;
unsigned int gSW_timer_max;


// Controls what parameter control page the knobs are mapped to
struct paramter_control_pages
{
	unsigned char parameter_page;
	unsigned char parameter_page_max;
};



// Parameter page
paramter_control_pages parameter_control[NUMBER_FX];
unsigned char ain_control_effect = 0;
unsigned int startup_mask_timer;


unsigned int lcdPrintClock = 0;		//Used to keep LCD from printing too fast when receiving analog controls



/*
*	Auxiliary tasks
*/

char lcdFxStr[20] = {0};
char lcdParamStr[20] = {0}; 		// lcd.print only accepts const char*, these arrays are used to store
char lcdFloatStr[20] = {0};			// other data types when casted to const char* by sprintf()
char lastLcdFxStr[20] = {0};
char lastLcdParamStr[20] = {0};
char lastLcdFloatStr[20] = {0};	

AuxiliaryTask lcdPrintTask;
void lcdPrint(void*)
{
	if (strcmp(lastLcdFxStr,lcdFxStr))
		{
			lcd.clear();
			lcd.setCursor(3,0);
			lcd.print("Active Effect:");
			lcd.setCursor(0,1);
			lcd.print(lcdFxStr);
			for(int i = 0; i<=19; i++)
			{
				lastLcdFxStr[i] = lcdFxStr[i];
				lcdParamStr[i] = 0;
				lcdFloatStr[i] = 0;
			}
		}
		
	if (strcmp(lastLcdParamStr,lcdParamStr))
		{
			lcd.clearLine(2);
			lcd.clearLine(3);
			lcd.setCursor(0,2);
			lcd.print(lcdParamStr);
			for(int i = 0; i<=19; i++)
			{
				lastLcdParamStr[i] = lcdParamStr[i];
				lcdFloatStr[i] = 0;
			}
		}
		
	if (strcmp(lastLcdFloatStr,lcdFloatStr))
		{
			lcd.clearLine(3);
			lcd.setCursor(0,3);
			lcd.print(lcdFloatStr);
			for(int i = 0; i<=19; i++)
			{
				lastLcdFloatStr[i] = lcdFloatStr[i];
			}
		}
}

/*
*	End Auxiliary tasks
*/





// ------------------  Parameter inputs  ------------------  //


/*
*  Flanger parameter controls from knobs
*/
void set_efx_flange_pg1()
{
	float rate, depth, width, mix, fb, damp;
	rate = depth = width = mix = fb = damp = 0.0;
	bool doprint = false;
	
	if(knobs[LFO_RATE].active_timer != 0) {
		rate = knobs[LFO_RATE].filtered_reading[1];
		rate *= rate;
		rate = map(rate, 0, 1, 0.1, 10.0);
		tflanger_setLfoRate(flanger, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"       Rate:");
		doprint = true;
	}
	if(knobs[LFO_DEPTH].active_timer != 0) {
		depth = map(knobs[LFO_DEPTH].filtered_reading[1], 0, 1.0, 0.0, 0.01);
		tflanger_setLfoDepth(flanger, depth);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"       Depth:");
		doprint = true;
	}
	if(knobs[LFO_WIDTH].active_timer != 0) {
		width = map(knobs[LFO_WIDTH].filtered_reading[1], 0, 1, 0.0, 0.01);
		tflanger_setLfoWidth(flanger, width);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"       Width:");
		doprint = true;
	}
	if(knobs[MIX_WET].active_timer != 0) {
		if(knobs[MIX_WET].switch_state)
			mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, 0.0, 1.0);
		else
			mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, 0.0, -1.0);
		tflanger_setWetDry(flanger, mix);
		sprintf(lcdFloatStr,"      %f",mix);
		sprintf(lcdParamStr,"        Mix:");
		doprint = true;
	}
	if(knobs[REGEN].active_timer != 0) {
		if(knobs[REGEN].switch_state)
			fb = map(knobs[REGEN].filtered_reading[1], 0, 1, 0.0, 1.0);
		else
			fb = map(knobs[REGEN].filtered_reading[1], 0, 1, 0.0, -1.0);
		tflanger_setFeedBack(flanger, fb);
		sprintf(lcdFloatStr,"      %f",fb);
		sprintf(lcdParamStr,"      Feedback:");
		doprint = true;
	}
	if(knobs[DAMPING].active_timer != 0) {
		damp = knobs[DAMPING].filtered_reading[1];
		damp *= damp;
		damp = map(damp, 0, 1, 40.0, 7400.0);
		tflanger_setDamping(flanger, damp);
		sprintf(lcdFloatStr,"      %f",damp);
		sprintf(lcdParamStr,"      Damping:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
}

void set_efx_flange_pg2()
{

	float rate, depth, width, mix, fb, attack, release, ftype;
	rate = depth = width = mix = fb = attack = release = ftype = 0.0;
	int lfo_type = 0;
	bool doprint = false;
	
	if(knobs[ENV_SNS_LFO_RATE].active_timer != 0) {
		rate = map(knobs[LFO_RATE].filtered_reading[1], 0, 1, -10.0, 20.0);
		tflanger_setEnvelopeRateSkew(flanger, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"   Envelope Rate:");
		doprint = true;
	}
	if(knobs[ENV_SNS_LFO_DEPTH].active_timer != 0) {
		depth = map(knobs[LFO_DEPTH].filtered_reading[1], 0, 1, -0.005, 0.005);
		tflanger_setEnvelopeDepthSkew(flanger, depth);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"   Envelope Depth:");
		doprint = true;
	}
	if(knobs[ENV_SNS_LFO_WIDTH].active_timer != 0) {
		width = map(knobs[LFO_WIDTH].filtered_reading[1], 0, 1, -0.0025, 0.0025);
		tflanger_setEnvelopeWidthSkew(flanger, width);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"   Envelope Width:");
		doprint = true;
	}
	if(knobs[ENV_SNS_MIX_WET].active_timer != 0) {
		mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, -1.0, 1.0);
		tflanger_setEnvelopeMixSkew(flanger, mix);
		sprintf(lcdFloatStr,"      %f",mix);
		sprintf(lcdParamStr,"   Envelope Mix:");
		doprint = true;
	}
	if(knobs[ENV_SNS_REGEN].active_timer != 0) {
		fb = map(knobs[REGEN].filtered_reading[1], 0, 1, -1.0, 1.0);
		tflanger_setEnvelopeFbSkew(flanger, fb);
		sprintf(lcdFloatStr,"      %f",fb);
		sprintf(lcdParamStr," Envelope Feedback:");
		doprint = true;
	}
	if(knobs[LFO_TYPE].active_timer != 0) {
		ftype = knobs[LFO_TYPE].filtered_reading[1];
		ftype = floorf(ftype*(MAX_LFOS + 1.0));
		lfo_type = lrintf(ftype);
		tflanger_set_lfo_type(flanger, lfo_type);
		switch (lfo_type)
		{
			case 0:
				sprintf(lcdFloatStr,"Integrated Triangle");
				break;
			case 1:
				sprintf(lcdFloatStr,"      Triangle");
				break;
			case 2:
				sprintf(lcdFloatStr,"        Sine");
				break;
			case 3:
				sprintf(lcdFloatStr,"       Square");
				break;
			case 4:
				sprintf(lcdFloatStr,"     Exponential");
				break;
			case 5:
				sprintf(lcdFloatStr,"      Relaxed");
				break;
			case 6:
				sprintf(lcdFloatStr,"       Hyper");
				break;
			case 7:
				sprintf(lcdFloatStr,"     Hyper Sine");
				break;
			default:
				break;
		}
		sprintf(lcdParamStr,"      LFO Type:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_flange_pg3()
{

	float sns, attack, release;
	sns = attack = release = 0.0;
	bool doprint = false;
	

	if(knobs[ENV_ATK].active_timer != 0) {
		attack = map(knobs[ENV_ATK].filtered_reading[1], 0, 1, 0.0, 0.25);
		tflanger_setEnvelopeAttack(flanger, attack);
		sprintf(lcdFloatStr,"      %f",attack);
		sprintf(lcdParamStr,"  Envelope Attack:");
		doprint = true;
	}	
	if(knobs[ENV_RLS].active_timer != 0) {
		release = map(knobs[ENV_RLS].filtered_reading[1], 0, 1, 0.0, 2.0);
		tflanger_setEnvelopeRelease(flanger, release);
		sprintf(lcdFloatStr,"      %f",release);
		sprintf(lcdParamStr,"  Envelope Release:");
		doprint = true;
	}
	if(knobs[ENV_SNS].active_timer != 0) {
		sns = knobs[ENV_SNS].filtered_reading[1];
		sns *= 6.0;
		sns *= sns;
		tflanger_setEnvelopeRelease(flanger, sns);
		sprintf(lcdFloatStr,"      %f",sns);
		sprintf(lcdParamStr,"Envelope Sensitivy:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_flange()
{
	switch(parameter_control[FLANGER_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_flange_pg1();
			break;
		case PARAMETER_PG2:
			set_efx_flange_pg2();
			break;
		case PARAMETER_PG3:
			set_efx_flange_pg3();
			break;
		default:
			break;
	}
}

/*
*  END flange controls
*/


/*
*  Chorus parameter controls from knobs
*/
void set_efx_chorus_pg1()
{
	float rate, depth, width, mix, fb, damp;
	rate = depth = width = mix = fb = damp = 0.0;
	bool doprint = false;
	
	if(knobs[LFO_RATE].active_timer != 0) {
		rate = knobs[LFO_RATE].filtered_reading[1];
		rate *= rate;
		rate = map(rate, 0, 1, 0.1, 10.0);
		tflanger_setLfoRate(chorus, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"       Rate:");
		doprint = true;
	}
	if(knobs[LFO_DEPTH].active_timer != 0) {
		depth = map(knobs[LFO_DEPTH].filtered_reading[1], 0, 1.0, 0.005, 0.035);
		tflanger_setLfoDepth(chorus, depth);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"       Depth:");
		doprint = true;
	}
	if(knobs[LFO_WIDTH].active_timer != 0) {
		width = map(knobs[LFO_WIDTH].filtered_reading[1], 0, 1, 0.0, 0.005);
		tflanger_setLfoWidth(chorus, width);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"       Width:");
		doprint = true;
	}
	if(knobs[MIX_WET].active_timer != 0) {
		if(knobs[MIX_WET].switch_state)
			mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, 0.0, 1.0);
		else
			mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, 0.0, -1.0);
		tflanger_setWetDry(chorus, mix);
		sprintf(lcdFloatStr,"      %f",mix);
		sprintf(lcdParamStr,"        Mix:");
		doprint = true;
	}
	if(knobs[REGEN].active_timer != 0) {
		if(knobs[REGEN].switch_state)
			fb = map(knobs[REGEN].filtered_reading[1], 0, 1, 0.0, 1.0);
		else
			fb = map(knobs[REGEN].filtered_reading[1], 0, 1, 0.0, -1.0);
		tflanger_setFeedBack(chorus, fb);
		sprintf(lcdFloatStr,"      %f",fb);
		sprintf(lcdParamStr,"      Feedback:");
		doprint = true;
	}
	if(knobs[DAMPING].active_timer != 0) {
		damp = knobs[DAMPING].filtered_reading[1];
		damp *= damp;
		damp = map(damp, 0, 1, 40.0, 7400.0);
		tflanger_setDamping(chorus, damp);
		sprintf(lcdFloatStr,"      %f",damp);
		sprintf(lcdParamStr,"      Damping:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_chorus_pg2()
{

	float rate, depth, width, mix, fb, attack, release, ftype;
	rate = depth = width = mix = fb = attack = release = ftype = 0.0;
	int lfo_type = 0;
	bool doprint = false;
	
	if(knobs[ENV_SNS_LFO_RATE].active_timer != 0) {
		rate = map(knobs[LFO_RATE].filtered_reading[1], 0, 1, -10.0, 20.0);
		tflanger_setEnvelopeRateSkew(chorus, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"   Envelope Rate:");
		doprint = true;
	}
	if(knobs[ENV_SNS_LFO_DEPTH].active_timer != 0) {
		depth = map(knobs[LFO_DEPTH].filtered_reading[1], 0, 1, -0.005, 0.005);
		tflanger_setEnvelopeDepthSkew(chorus, depth);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"   Envelope Depth:");
		doprint = true;
	}
	if(knobs[ENV_SNS_LFO_WIDTH].active_timer != 0) {
		width = map(knobs[LFO_WIDTH].filtered_reading[1], 0, 1, -0.005, 0.005);
		tflanger_setEnvelopeWidthSkew(chorus, width);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"   Envelope Width:");
		doprint = true;
	}
	if(knobs[ENV_SNS_MIX_WET].active_timer != 0) {
		mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, -1.0, 1.0);
		tflanger_setEnvelopeMixSkew(chorus, mix);
		sprintf(lcdFloatStr,"      %f",mix);
		sprintf(lcdParamStr,"   Envelope Mix:");
		doprint = true;
	}
	if(knobs[ENV_SNS_REGEN].active_timer != 0) {
		fb = map(knobs[REGEN].filtered_reading[1], 0, 1, -1.0, 1.0);
		tflanger_setEnvelopeFbSkew(chorus, fb);
		sprintf(lcdFloatStr,"      %f",fb);
		sprintf(lcdParamStr," Envelope Feedback:");
		doprint = true;
	}
	if(knobs[LFO_TYPE].active_timer != 0) {
		ftype = knobs[LFO_TYPE].filtered_reading[1];
		ftype = floorf(ftype*(MAX_LFOS + 1.0));
		lfo_type = lrintf(ftype);
		tflanger_set_lfo_type(chorus, lfo_type);
		switch (lfo_type)
		{
			case 0:
				sprintf(lcdFloatStr,"Integrated Triangle");
				break;
			case 1:
				sprintf(lcdFloatStr,"      Triangle");
				break;
			case 2:
				sprintf(lcdFloatStr,"        Sine");
				break;
			case 3:
				sprintf(lcdFloatStr,"       Square");
				break;
			case 4:
				sprintf(lcdFloatStr,"     Exponential");
				break;
			case 5:
				sprintf(lcdFloatStr,"      Relaxed");
				break;
			case 6:
				sprintf(lcdFloatStr,"       Hyper");
				break;
			case 7:
				sprintf(lcdFloatStr,"     Hyper Sine");
				break;
			default:
				break;
		}
		sprintf(lcdParamStr,"      LFO Type:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_chorus_pg3()
{

	float sns, attack, release;
	sns = attack = release = 0.0;
	bool doprint = false;
	

	if(knobs[ENV_ATK].active_timer != 0) {
		attack = map(knobs[ENV_ATK].filtered_reading[1], 0, 1, 0.0, 0.25);
		tflanger_setEnvelopeAttack(chorus, attack);
		sprintf(lcdFloatStr,"      %f",attack);
		sprintf(lcdParamStr,"  Envelope Attack:");
		doprint = true;
	}	
	if(knobs[ENV_RLS].active_timer != 0) {
		release = map(knobs[ENV_RLS].filtered_reading[1], 0, 1, 0.0, 2.0);
		tflanger_setEnvelopeRelease(chorus, release);
		sprintf(lcdFloatStr,"      %f",release);
		sprintf(lcdParamStr,"  Envelope Release:");
		doprint = true;
	}
	if(knobs[ENV_SNS].active_timer != 0) {
		sns = knobs[ENV_SNS].filtered_reading[1];
		sns *= 6.0;
		sns *= sns;
		tflanger_setEnvelopeRelease(chorus, sns);
		sprintf(lcdFloatStr,"      %f",sns);
		sprintf(lcdParamStr,"Envelope Sensitivy:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_chorus()
{
	switch(parameter_control[CHORUS_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_chorus_pg1();
			break;
		case PARAMETER_PG2:
			set_efx_chorus_pg2();
			break;
		case PARAMETER_PG3:
			set_efx_chorus_pg3();
			break;
		default:
			break;
	}
}

/*
*  End chorus controls
*/


/*
*  Delay parameter controls from knobs
*/
void set_efx_delay_pg1()
{
	float rate, depth, width, mix, fb, damp;
	rate = depth = width = mix = fb = damp = 0.0;
	bool doprint = false;
	
	if(knobs[LFO_RATE].active_timer != 0) {
		rate = knobs[LFO_RATE].filtered_reading[1];
		rate *= rate;
		rate = map(rate, 0, 1, 0.1, 10.0);
		tflanger_setLfoRate(delay, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"       Rate:");
		doprint = true;
	}
	if(knobs[LFO_DEPTH].active_timer != 0) {
		depth = map(knobs[LFO_DEPTH].filtered_reading[1], 0, 1, 0.0005, 1.0);
		tflanger_setLfoDepth(delay, depth);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"       Depth:");
		doprint = true;
	}
	if(knobs[LFO_WIDTH].active_timer != 0) {
		width = map(knobs[LFO_WIDTH].filtered_reading[1], 0, 1, 0.0, 0.0025);
		tflanger_setLfoWidth(delay, width);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"       Width:");
		doprint = true;
	}
	if(knobs[MIX_WET].active_timer != 0) {
		if(knobs[MIX_WET].switch_state)
			mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, 0.0, 1.0);
		else
			mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, 0.0, -1.0);
		tflanger_setWetDry(delay, mix);
		sprintf(lcdFloatStr,"      %f",mix);
		sprintf(lcdParamStr,"        Mix:");
		doprint = true;
	}
	if(knobs[REGEN].active_timer != 0) {
		if(knobs[REGEN].switch_state)
			fb = map(knobs[REGEN].filtered_reading[1], 0, 1, 0.0, 1.0);
		else
			fb = map(knobs[REGEN].filtered_reading[1], 0, 1, 0.0, -1.0);
		tflanger_setFeedBack(delay, fb);
		sprintf(lcdFloatStr,"      %f",fb);
		sprintf(lcdParamStr,"      Feedback:");
		doprint = true;
	}
	if(knobs[DAMPING].active_timer != 0) {
		damp = knobs[DAMPING].filtered_reading[1];
		damp *= damp;
		damp = map(damp, 0, 1, 40.0, 7400.0);
		tflanger_setDamping(delay, damp);
		sprintf(lcdFloatStr,"      %f",damp);
		sprintf(lcdParamStr,"      Damping:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_delay_pg2()
{

	float rate, depth, width, mix, fb, damp, ftype;
	rate = depth = width = mix = fb = damp = ftype = 0.0;
	int lfo_type = 0;
	bool doprint = false;
	
	if(knobs[ENV_SNS_LFO_RATE].active_timer != 0) {
		rate = map(knobs[LFO_RATE].filtered_reading[1], 0, 1, -10.0, 20.0);
		tflanger_setEnvelopeRateSkew(delay, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"   Envelope Rate:");
		doprint = true;
	}
	if(knobs[ENV_SNS_LFO_DEPTH].active_timer != 0) {
		depth = map(knobs[LFO_DEPTH].filtered_reading[1], 0, 1, -1.0, 1.0);
		tflanger_setEnvelopeDepthSkew(delay, depth);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"   Envelope Depth:");
		doprint = true;
	}
	if(knobs[ENV_SNS_LFO_WIDTH].active_timer != 0) {
		width = map(knobs[LFO_WIDTH].filtered_reading[1], 0, 1, -0.0025, 0.0025);
		tflanger_setEnvelopeWidthSkew(delay, width);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"   Envelope Width:");
		doprint = true;
	}
	if(knobs[ENV_SNS_MIX_WET].active_timer != 0) {
		mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, -1.0, 1.0);
		tflanger_setEnvelopeMixSkew(delay, mix);
		sprintf(lcdFloatStr,"      %f",mix);
		sprintf(lcdParamStr,"   Envelope Mix:");
		doprint = true;
	}
	if(knobs[ENV_SNS_REGEN].active_timer != 0) {
		fb = map(knobs[REGEN].filtered_reading[1], 0, 1, -1.0, 1.0);
		tflanger_setEnvelopeFbSkew(delay, fb);
		sprintf(lcdFloatStr,"      %f",fb);
		sprintf(lcdParamStr," Envelope Feedback:");
		doprint = true;
	}
	if(knobs[LFO_TYPE].active_timer != 0) {
		ftype = knobs[LFO_TYPE].filtered_reading[1];
		ftype = floorf(ftype*(MAX_LFOS + 1.0));
		lfo_type = lrintf(ftype);
		tflanger_set_lfo_type(delay, lfo_type);
		switch (lfo_type)
		{
			case 0:
				sprintf(lcdFloatStr,"Integrated Triangle");
				break;
			case 1:
				sprintf(lcdFloatStr,"      Triangle");
				break;
			case 2:
				sprintf(lcdFloatStr,"        Sine");
				break;
			case 3:
				sprintf(lcdFloatStr,"       Square");
				break;
			case 4:
				sprintf(lcdFloatStr,"     Exponential");
				break;
			case 5:
				sprintf(lcdFloatStr,"      Relaxed");
				break;
			case 6:
				sprintf(lcdFloatStr,"       Hyper");
				break;
			case 7:
				sprintf(lcdFloatStr,"     Hyper Sine");
				break;
			default:
				break;
		}
		sprintf(lcdParamStr,"      LFO Type:");
		doprint = true;
	}
	
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_delay_pg3()
{

	float sns, attack, release;
	sns = attack = release = 0.0;
	bool doprint = false;
	

	if(knobs[ENV_ATK].active_timer != 0) 
	{
		attack = map(knobs[ENV_ATK].filtered_reading[1], 0, 1, 0.0, 0.25);
		tflanger_setEnvelopeAttack(delay, attack);
		sprintf(lcdFloatStr,"      %f",attack);
		sprintf(lcdParamStr,"  Envelope Attack:");
		doprint = true;
	}	
	if(knobs[ENV_RLS].active_timer != 0) 
	{
		release = map(knobs[ENV_RLS].filtered_reading[1], 0, 1, 0.0, 2.0);
		tflanger_setEnvelopeRelease(delay, release);
		sprintf(lcdFloatStr,"      %f",release);
		sprintf(lcdParamStr,"  Envelope Release:");
		doprint = true;
	}
	if(knobs[ENV_SNS].active_timer != 0) 
	{
		sns = knobs[ENV_SNS].filtered_reading[1];
		sns *= 6.0;
		sns *= sns;
		tflanger_setEnvelopeRelease(delay, sns);
		sprintf(lcdFloatStr,"      %f",sns);
		sprintf(lcdParamStr,"Envelope Sensitivy:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) )
	{
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_delay()
{
	switch(parameter_control[DELAY_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_delay_pg1();
			break;
		case PARAMETER_PG2:
			set_efx_delay_pg2();
			break;
		case PARAMETER_PG3:
			set_efx_delay_pg3();
			break;
		default:
			break;
	}
}

/*
*  End delay controls
*/


/*
*  Phaser parameter controls from knobs
*/

void set_efx_phaser_pg1()
{
	float rate, depth, width, mix, fb;
	rate = depth = width = mix = fb = 0.0;
	int ns = 0;
	bool doprint = false;
	
	if(knobs[LFO_RATE].active_timer != 0) {
		rate = knobs[LFO_RATE].filtered_reading[1];
		rate *= rate;
		rate = map(rate, 0, 1, 0.05, 10.0);
		phaser_set_lfo_rate(phaser, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"       Rate:");
		doprint = true;
	}
	if(knobs[LFO_DEPTH].active_timer != 0) {
		depth = knobs[LFO_DEPTH].filtered_reading[1];
		depth *= depth;
		depth = map(depth, 0, 1, 100.0, 1000.0);
		phaser_set_lfo_depth(phaser, depth, 0);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"       Depth:");
		doprint = true;
	}
	if(knobs[LFO_WIDTH].active_timer != 0) {
		width = knobs[LFO_WIDTH].filtered_reading[1];
		width *= width;
		width = map(width, 0, 1, 0.0, 5000.0);
		phaser_set_lfo_width(phaser, width, 0);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"       Width:");
		doprint = true;
	}
	if(knobs[MIX_WET].active_timer != 0) {
		mix =  map(knobs[MIX_WET].filtered_reading[1], 0, 1, -1.0, 1.0);
		phaser_set_mix(phaser, mix);
		sprintf(lcdFloatStr,"      %f",mix);
		sprintf(lcdParamStr,"        Mix:");
		doprint = true;
	}
	if(knobs[REGEN].active_timer != 0) {
		fb = map(knobs[REGEN].filtered_reading[1], 0, 1, -1.0, 1.0);
		phaser_set_feedback(phaser, fb, 3);
		sprintf(lcdFloatStr,"      %f",fb);
		sprintf(lcdParamStr,"      Feedback:");
		doprint = true;
	}
	if(knobs[PHASER_STAGES].active_timer != 0) {
		ns = lrintf( map(knobs[PHASER_STAGES].filtered_reading[1], 0, 1, 2.0, 24.0) );
		phaser_set_nstages(phaser, ns);
		sprintf(lcdFloatStr,"         %i",ns);
		sprintf(lcdParamStr,"       Stages:");
		doprint = true;
	}

	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}		
}


void set_efx_phase()
{
	switch(parameter_control[PHASER_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_phaser_pg1();
			break;
		default:
			break;
	}
}

/*
*  End phaser controls
*/


/*
*  Reverb parameter controls from knobs
*/
void set_efx_reverb_pg1()
{
	float opmix, xov, rtlo, rtmid, fdamp, eql, eqh, idly;
	xov = rtlo = rtmid = fdamp = eql = eqh = idly = opmix = 0.0;
	bool doprint = false;
	
	if(knobs[REVERB_MIX].active_timer != 0) 
	{
		opmix = knobs[REVERB_MIX].filtered_reading[1];
		reverb.set_opmix (opmix);
		sprintf(lcdFloatStr,"      %f",opmix);
		sprintf(lcdParamStr,"        Mix:");
		doprint = true;
	}
	else if(knobs[REVERB_XOV].active_timer != 0) 
	{
		xov = map(knobs[REVERB_XOV].filtered_reading[1], 0.0, 1.0, 50.0, 1000.0);
		reverb.set_xover(xov);
		sprintf(lcdFloatStr,"      %f",xov);
		sprintf(lcdParamStr,"      Crossover:");
		doprint = true;
	}
	else if(knobs[REVERB_RTLOW].active_timer != 0) 
	{
		rtlo = map(knobs[REVERB_RTLOW].filtered_reading[1], 0.0, 1.0, 1.0, 8.0);
		reverb.set_rtlow(rtlo);
		sprintf(lcdFloatStr,"      %f",rtlo);
		sprintf(lcdParamStr," Reverb Time (Low):");
		doprint = true;
	}	
	else if(knobs[REVERB_RTMID].active_timer != 0) 
	{
		rtmid = map(knobs[REVERB_RTMID].filtered_reading[1], 0.0, 1.0, 1.0, 8.0);
		reverb.set_rtmid(rtmid);
		sprintf(lcdFloatStr,"      %f",rtmid);
		sprintf(lcdParamStr," Reverb Time (Mid):");
		doprint = true;
	}
	else if(knobs[REVERB_IDLY].active_timer != 0) 
	{
		idly = map(knobs[REVERB_IDLY].filtered_reading[1], 0.0, 1.0, 0.02, 0.1);
		reverb.set_delay(idly);
		sprintf(lcdFloatStr,"      %f",idly);
		sprintf(lcdParamStr,"       Delay:");
		doprint = true;
	}	
	else if(knobs[REVERB_FDAMP].active_timer != 0) 
	{
		fdamp = knobs[REVERB_FDAMP].filtered_reading[1];
		fdamp = 1500.0 + 20000.0*fdamp*fdamp; //poor-man's log taper
		reverb.set_fdamp(fdamp);
		sprintf(lcdFloatStr,"      %f",fdamp);
		sprintf(lcdParamStr," Frequency Damping:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
}

void set_efx_reverb_pg2()
{
	float eqlf, eqlg, eqhf, eqhg;
	eqlf = eqlg = eqhf = eqhg = 0.0;
	bool doprint = false;
	
	if( (knobs[REVERB_EQL_F].active_timer != 0) || (knobs[REVERB_EQL_G].active_timer != 0) )
	{
		eqlf = knobs[REVERB_EQL_F].filtered_reading[1];
		eqlf = 40.0 + 460.0*eqlf*eqlf;
		eqlg = map(knobs[REVERB_EQL_G].filtered_reading[1], 0.0, 1.0, -12.0, 12.0);
		reverb.set_eq1(eqlf, eqlg);
		if (knobs[REVERB_EQL_F].active_timer != 0)
		{
			sprintf(lcdFloatStr,"      %f",eqlf);
			sprintf(lcdParamStr,"       EQLF:");
		}
		else
		{
			sprintf(lcdFloatStr,"      %f",eqlg);
			sprintf(lcdParamStr,"       EQLG:");
		}
		doprint = true;
	}
	
	else if( (knobs[REVERB_EQH_F].active_timer != 0) || (knobs[REVERB_EQH_G].active_timer != 0) )
	{
		eqhf = knobs[REVERB_EQH_F].filtered_reading[1];
		eqhf = 750.0 + 10000.0*eqhf*eqhf;
		eqhg = map(knobs[REVERB_EQH_G].filtered_reading[1], 0.0, 1.0, -12.0, 12.0);
		reverb.set_eq2(eqhf, eqhg);
		if (knobs[REVERB_EQH_F].active_timer != 0)
		{
			sprintf(lcdFloatStr,"      %f",eqhf);
			sprintf(lcdParamStr,"       EQHF:");
		}
		else
		{
			sprintf(lcdFloatStr,"      %f",eqhg);
			sprintf(lcdParamStr,"       EQHG:");
		}
		doprint = true;
	}
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_reverb()
{
	switch(parameter_control[REVERB_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_reverb_pg1();
			break;
		case PARAMETER_PG2:
			set_efx_reverb_pg2();
			break;
		default:
			break;
	}
}

/*
*  End reverb controls
*/


/*
*  Tremolo parameter controls from knobs
*/
void set_efx_tremolo_pg1()
{

	float rate, depth, width, ftype;
	unsigned int lfo_type = 0;
	char lfo_name[100];
	rate = depth = width = ftype = 0.0;
	bool doprint = false;
	
	if(knobs[LFO_RATE].active_timer != 0) {
		rate = knobs[LFO_RATE].filtered_reading[1];
		rate *= rate;
		rate = map(rate, 0, 1, 0.05, 20.0);
		trem_set_lfo_rate(tremolo, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"       Rate:");
		doprint = true;
	}
	if(knobs[LFO_DEPTH].active_timer != 0) {
		depth = knobs[LFO_DEPTH].filtered_reading[1];
		depth *= depth;
		depth = map(depth, 0, 1, 1.0, 2.0);
		trem_set_lfo_gain(tremolo, depth);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"       Depth:");
		doprint = true;
	}
	if(knobs[LFO_WIDTH].active_timer != 0) {
		width = knobs[LFO_WIDTH].filtered_reading[1];
		width = map(width, 0, 1, 0.33, 1.0);
		trem_set_lfo_depth(tremolo, width);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"       Width:");
		doprint = true;
	}
	if(knobs[TREMOLO_LFO_TYPE].active_timer != 0) {
		ftype = knobs[TREMOLO_LFO_TYPE].filtered_reading[1];
		ftype = floorf(ftype*(MAX_LFOS + 1.0));
		lfo_type = lrintf(ftype);
		trem_set_lfo_type(tremolo, lfo_type);
		switch (lfo_type)
		{
			case 0:
				sprintf(lcdFloatStr,"Integrated Triangle");
				break;
			case 1:
				sprintf(lcdFloatStr,"      Triangle");
				break;
			case 2:
				sprintf(lcdFloatStr,"        Sine");
				break;
			case 3:
				sprintf(lcdFloatStr,"       Square");
				break;
			case 4:
				sprintf(lcdFloatStr,"     Exponential");
				break;
			case 5:
				sprintf(lcdFloatStr,"      Relaxed");
				break;
			case 6:
				sprintf(lcdFloatStr,"       Hyper");
				break;
			case 7:
				sprintf(lcdFloatStr,"     Hyper Sine");
				break;
			default:
				break;
		}
		sprintf(lcdParamStr,"      LFO Type:");
		doprint = true;
	}

	if( (lcdPrintClock >= 5000) && (doprint) ){
		get_lfo_name(lfo_type, lfo_name);
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
}


void set_efx_tremolo()
{
	switch(parameter_control[TREMOLO_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_tremolo_pg1();
			break;
		default:
			break;
	}
}

/*
*  End tremolo controls
*/


/*
*  Wah parameter controls from knobs
*/
void set_efx_wah_pg1()
{
	unsigned int gWahCkt;
	bool doprint = false;
	float type = 0.0;
	
	if(knobs[WAH_CKT].active_timer != 0) {
		type = knobs[WAH_CKT].filtered_reading[1]*MAX_WAHS;
		gWahCkt = lrintf(floorf(type));
		iwah_circuit_preset(gWahCkt, wah, gSampleRate );
		sprintf(lcdParamStr,"    Model Type:");
		switch(gWahCkt)
		{
			case 0:
				sprintf(lcdFloatStr,"   Dunlop GCB-95");
				break;
			case 1:
				sprintf(lcdFloatStr,"      Vox V847");
				break;
			case 2:
				sprintf(lcdFloatStr,"   Dunlop Crybaby");
				break;
			case 3:
				sprintf(lcdFloatStr,"    Clyde McCoy");
				break;
			case 4:
				sprintf(lcdFloatStr,"  Vox Vocal Model");
				break;
			case 5:
				sprintf(lcdFloatStr,"    Crazy Synth");
				break;
			default:
				break;
		}
			
		doprint = true;
	}
	

	if( (lcdPrintClock >= 5000) && (doprint)){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
}


void set_efx_wah()
{
	switch(parameter_control[WAH_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_wah_pg1();
			break;
		default:
			break;
	}	
}
/*
*  End wah controls
*/


/*
*  Distortion parameter controls from knobs
*/

void set_efx_distortion_pg1()
{
	float gain, range, blend, volume, type;
	gain = range = blend = volume = type = 0;
	unsigned int dist_type = 0;
	bool doprint = false;
	
	if(knobs[DIST_GAIN].active_timer != 0) {
		gain = knobs[DIST_GAIN].filtered_reading[1];
		gain *= gain;
		gain = map(gain, 0, 1, 0.05, 1);
		distortion.gain = gain;
		sprintf(lcdFloatStr,"      %f",gain);
		sprintf(lcdParamStr,"       Gain:");
		doprint = true;
	}
	if(knobs[DIST_RANGE].active_timer != 0) {
		range = knobs[DIST_RANGE].filtered_reading[1];
		range *= range;
		range = map(range, 0, 1, 1.0, 250);
		distortion.range = range;
		sprintf(lcdFloatStr,"      %f",range);
		sprintf(lcdParamStr,"       Range:");
		doprint = true;
	}
	if(knobs[DIST_BLEND].active_timer != 0) {
		blend = knobs[DIST_BLEND].filtered_reading[1];
		distortion.blend = blend;
		sprintf(lcdFloatStr,"      %f",blend);
		sprintf(lcdParamStr,"       Blend:");
		doprint = true;
	}
	if(knobs[DIST_VOLUME].active_timer != 0) {
		volume = knobs[DIST_VOLUME].filtered_reading[1];
		volume *= volume;
		volume = map(volume, 0, 1, 0, .75);
		distortion.volume = volume;
		sprintf(lcdFloatStr,"      %f",volume*4/3);
		sprintf(lcdParamStr,"      Volume:");
		doprint = true;
	}
	if(knobs[DIST_TYPE].active_timer != 0) {
		type = knobs[DIST_TYPE].filtered_reading[1];
		type = floorf(type*(MAX_DISTORTIONS + 1.0));
		dist_type = lrintf(type);
		distortion.type = dist_type;
		switch (dist_type)
		{
			case 0:
				sprintf(lcdFloatStr,"  Inverse Tangent");
				break;
			case 1:
				sprintf(lcdFloatStr," Cubic Nonlinearity");
				break;
			case 2:
				sprintf(lcdFloatStr,"   Soft Clipping");
				break;
			case 3:
				sprintf(lcdFloatStr,"Exponential Clipper");
				break;
			default:
				break;
		}
		sprintf(lcdParamStr,"  Distortion Type:");
		doprint = true;
	}

	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
}

void set_efx_distortion()
{
	switch(parameter_control[TREMOLO_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_distortion_pg1();
			break;
		default:
			break;
	}
}

/*
*  End distortion controls
*/


/*
*  Sustainer parameter controls from knobs
*/
void set_efx_sustainer_pg1()
{

	float sustain, gain;

	gain = sustain = 1.0;
	bool doprint = false;
	
	if(knobs[SUSTAIN_GAIN].active_timer != 0) {
		gain = knobs[SUSTAIN_GAIN].filtered_reading[1];
		sustainer.setGain(gain);
		sprintf(lcdFloatStr,"      %f",gain);
		sprintf(lcdParamStr,"       Gain:");
		doprint = true;
	}
	if(knobs[SUSTAIN_DRIVE].active_timer != 0) {
		sustain = knobs[SUSTAIN_DRIVE].filtered_reading[1];
		sustainer.setSustain(sustain);
		sprintf(lcdFloatStr,"      %f",sustain);
		sprintf(lcdParamStr,"      Sustain:");
		doprint = true;
	}

	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
}



void set_efx_sustainer()
{
	switch(parameter_control[SUSTAINER_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_sustainer_pg1();
			break;
		default:
			break;
	}
}

/*
*  End sustainer controls
*/


/*
*  Graphic equalizers parameter controls from knobs
*/

void set_efx_geq1_pg1()
{
	float g = 0.0;
	int i = 0;
	bool doprint = false;
	int band = 0;
	
	for(i=0; i < geq1->nbands; i++)
	{	
		if(knobs[i].active_timer != 0) {
			g = map(knobs[i].filtered_reading[1], 0.0, 1.0, -12.0, 12.0);
			eq_update_gain(geq1->band[i], g);
			band = i;
			sprintf(lcdFloatStr,"      %f",g);
			sprintf(lcdParamStr,"    Band %i Gain:",band+1);
			doprint = true;
		}
		
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
	
}

void set_efx_geq1_pg2()
{
	//set high and low shelving filters
	float g = 0.0;
	bool doprint = false;
	if(knobs[0].active_timer != 0) 
	{
		g = map(knobs[0].filtered_reading[1], 0.0, 1.0, -12.0, 12.0);
		eq_update_gain(geq1->band[geq1->nbands], g);
		sprintf(lcdFloatStr,"      %f",g);
		sprintf(lcdParamStr,"High Shlf Flt Gain:");
		doprint = true;
	}
	else if(knobs[1].active_timer != 0) 
	{
		g = map(knobs[1].filtered_reading[1], 0.0, 1.0, -12.0, 12.0);
		eq_update_gain(geq1->band[geq1->nbands + 1], g);
		sprintf(lcdFloatStr,"      %f",g);
		sprintf(lcdParamStr," Low Shlf Flt Gain:");
		doprint = true;
	}

	if( (lcdPrintClock >= 5000) && (doprint) ){
	Bela_scheduleAuxiliaryTask(lcdPrintTask);
	lcdPrintClock = 0;
	}	
}

void set_efx_geq1()
{
	switch(parameter_control[EQ_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_geq1_pg1();
			break;
		case PARAMETER_PG2:
			set_efx_geq1_pg2();
			break;
		default:
			break;
	}	
}


/*
*  End EQ controls
*/


/*
*  Envelope Filter parameter controls from knobs
*/
void set_efx_envf_pg1()
{
	float rate, depth, width, mix, q, sns;
	rate = depth = width = mix = q = sns = 0.0;
	bool doprint = false;
	
	if(knobs[LFO_RATE].active_timer != 0) {
		rate = knobs[LFO_RATE].filtered_reading[1];
		rate *= rate;
		rate = map(rate, 0.0, 1.0, 0.1, 30.0);
		envf_set_lfo_rate(ef, rate);
		sprintf(lcdFloatStr,"      %f",rate);
		sprintf(lcdParamStr,"       Rate:");
		doprint = true;
	}
	if(knobs[LFO_DEPTH].active_timer != 0) {
		depth = knobs[LFO_DEPTH].filtered_reading[1];
		depth *= depth;
		depth = map(depth, 0.0, 1.0, 10.0, 1000.0);
		envf_set_depth(ef, depth);
		sprintf(lcdFloatStr,"      %f",depth);
		sprintf(lcdParamStr,"       Depth:");
		doprint = true;
	}
	if(knobs[LFO_WIDTH].active_timer != 0) {
		width = knobs[LFO_WIDTH].filtered_reading[1];
		width *= width;
		width = map(width, 0.0, 1.0, 0.0, 5000.0);
		envf_set_width(ef, width);
		sprintf(lcdFloatStr,"      %f",width);
		sprintf(lcdParamStr,"       Width:");
		doprint = true;
	}
	if(knobs[MIX_WET].active_timer != 0) {
		mix =  map(knobs[MIX_WET].filtered_reading[1], 0.0, 1.0, -1.0, 1.0);
		envf_set_mix(ef, mix);
		sprintf(lcdFloatStr,"      %f",mix);
		sprintf(lcdParamStr,"        Mix:");
		doprint = true;
	}
	if(knobs[REGEN].active_timer != 0) {
		q = knobs[REGEN].filtered_reading[1];
		q *= q;
		q = map(q, 0.0, 1.0, 0.5, 60.0);
		envf_set_q(ef, q);
		sprintf(lcdFloatStr,"      %f",q);
		sprintf(lcdParamStr,"     Feedback:");
		doprint = true;
	}
	if(knobs[EF_DET_SNS].active_timer != 0) {
		sns = knobs[EF_DET_SNS].filtered_reading[1];
		sns *= sns;
		sns = map(sns, 0.0, 1.0, -3.0, 3.0);
		envf_set_sensitivity(ef, sns);
		sprintf(lcdFloatStr,"      %f",sns);
		sprintf(lcdParamStr,"    Sensitivity:");
		doprint = true;
	}
	

	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_envf_pg2()
{
	bool doprint = false;
	float hmix, bmix, lmix, atk, rls, dist;
	hmix = bmix = lmix = atk = rls = dist = 0.0;

	if(knobs[EF_MIX_LP].active_timer != 0) {
		lmix = map(knobs[EF_MIX_LP].filtered_reading[1], 0.0, 1.0, -1.0, 1.0);
		envf_set_mix_lpf(ef, lmix);
		sprintf(lcdFloatStr,"      %f",lmix);
		sprintf(lcdParamStr,"      LPF Mix:");
		doprint = true;
	}
	if(knobs[EF_MIX_BP].active_timer != 0) {
		bmix = map(knobs[EF_MIX_BP].filtered_reading[1], 0.0, 1.0, -1.0, 1.0);
		envf_set_mix_bpf(ef, bmix);
		sprintf(lcdFloatStr,"      %f",bmix);
		sprintf(lcdParamStr,"      BPF Mix:");
		doprint = true;
	}
	if(knobs[EF_MIX_HP].active_timer != 0) {
		hmix = map(knobs[EF_MIX_HP].filtered_reading[1], 0.0, 1.0, -1.0, 1.0);
		envf_set_mix_hpf(ef, hmix);
		sprintf(lcdFloatStr,"      %f",hmix);
		sprintf(lcdParamStr,"      HPF Mix:");
		doprint = true;
	}
	if(knobs[EF_ATK].active_timer != 0) {
		atk = map(knobs[EF_ATK].filtered_reading[1], 0.0, 1.0, 0.001, 0.25);
		envf_set_atk(ef, atk);
		sprintf(lcdFloatStr,"      %f",atk);
		sprintf(lcdParamStr,"      Attack:");
		doprint = true;
	}
	if(knobs[EF_RLS].active_timer != 0) {
		rls = map(knobs[EF_RLS].filtered_reading[1], 0.0, 1.0, 0.01, 2.0);
		envf_set_rls(ef, rls);
		sprintf(lcdFloatStr,"      %f",rls);
		sprintf(lcdParamStr,"      Release:");
		doprint = true;
	}
	if(knobs[EF_DIST].active_timer != 0) {
		dist = map(knobs[EF_DIST].filtered_reading[1], 0.0, 1.0, 0.001, 2.0);
		envf_set_drive(ef, dist);
		sprintf(lcdFloatStr,"      %f",dist);
		sprintf(lcdParamStr,"       Drive:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
}

void set_efx_envf_pg3()
{
	float gate_thrs, gate_knee, shmix, sht;
	int nsht;
	bool doprint = false;
	gate_knee = gate_thrs = shmix = 0.0;
	
	if(knobs[EF_GATE_THRS].active_timer != 0) {
		gate_thrs = map(knobs[EF_GATE_THRS].filtered_reading[1], 0.0, 1.0, -60.0, -12.0);
		envf_set_gate(ef, gate_thrs);
		sprintf(lcdFloatStr,"      %f",gate_thrs);
		sprintf(lcdParamStr,"  Gate Threshold:");
		doprint = true;
	}
	if(knobs[EF_GATE_KNEE].active_timer != 0) {
		gate_knee = map(knobs[EF_GATE_KNEE].filtered_reading[1], 0.0, 1.0, 0.1, 12.0);
		envf_set_gate_knee(ef, gate_knee);
		sprintf(lcdFloatStr,"      %f",gate_knee);
		sprintf(lcdParamStr,"     Gate Knee:");
		doprint = true;
	}
	if(knobs[EF_SH_MIX].active_timer != 0) {
		shmix = knobs[EF_SH_MIX].filtered_reading[1];
		envf_set_mix_sh_modulator(ef, shmix);
		sprintf(lcdFloatStr,"      %f",shmix);
		sprintf(lcdParamStr,"Shlf Modulator Mix:");
		doprint = true;
	}
	if(knobs[EF_SH_TYPE].active_timer != 0) {
		sht = knobs[EF_SH_TYPE].filtered_reading[1];
		nsht = lrintf(sht*(ef->sh->max_types - 1.0));
		envf_set_sample_hold_type(ef, nsht);
		switch (nsht)
		{
			case 0:
				sprintf(lcdFloatStr,"  Random Sequencer");
				break;
			case 1:
				sprintf(lcdFloatStr,"  Ramp Oscillator");
				break;
			case 2:
				sprintf(lcdFloatStr,"     Sequencer");
				break;
			default:
				break;
		}
		sprintf(lcdParamStr," Sample Hold Type:");
		doprint = true;
	}
	//Select whether ADSR is active
	if(knobs[EF_SH_TYPE].edge)
	{
		knobs[EF_SH_TYPE].edge = false;
		
		if( envf_set_adsr_active(ef, true) == true )
		{
			rt_printf("ACTIVATED ADSR for filter Sample/Hold Effect\n");	
			sprintf(lcdParamStr,"Activated ADSR for");
			sprintf(lcdFloatStr,"  filter sample");
		} else
		{
			rt_printf("DISABLED ADSR for filter Sample/Hold Effect\n");
			sprintf(lcdParamStr,"Disabled ADSR for");
			sprintf(lcdFloatStr,"  filter sample");
		}
	}
	
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_envf_pg4()
{
	float atk, dcy, stn, rls;
	bool doprint = false;
	atk = dcy = stn = rls = 0.0;
	
	if(knobs[EF_ADSR_ATK].active_timer != 0) {
		atk = knobs[EF_ADSR_ATK].filtered_reading[1];
		atk *= atk*1000.0;
		envf_set_adsr_atk(ef, atk);
		sprintf(lcdFloatStr,"      %f",atk);
		sprintf(lcdParamStr,"    ADSR Attack:");
		doprint = true;
	}
	if(knobs[EF_ADSR_RLS].active_timer != 0) {
		rls = knobs[EF_ADSR_RLS].filtered_reading[1];
		rls *= rls*1000.0;
		envf_set_adsr_rls(ef, rls);
		sprintf(lcdFloatStr,"      %f",rls);
		sprintf(lcdParamStr,"   ADSR Release:");
		doprint = true;
	}
	if(knobs[EF_ADSR_DCY].active_timer != 0) {
		dcy = knobs[EF_ADSR_DCY].filtered_reading[1];
		dcy *= dcy*1000.0;
		envf_set_adsr_dcy(ef, dcy);
		sprintf(lcdFloatStr,"      %f",dcy);
		sprintf(lcdParamStr,"    ADSR Decay:");
		doprint = true;
	}
	if(knobs[EF_ADSR_STN].active_timer != 0) {
		stn = knobs[EF_ADSR_STN].filtered_reading[1];
		stn *= stn;
		envf_set_adsr_stn(ef, stn);
		sprintf(lcdFloatStr,"      %f",stn);
		sprintf(lcdParamStr,"   ADSR Sustain:");
		doprint = true;
	} 
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_envf()
{
	switch(parameter_control[ENVF_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_envf_pg1();
			break;
		case PARAMETER_PG2:
			set_efx_envf_pg2();
			break;
		case PARAMETER_PG3:
			set_efx_envf_pg3();
			break;
		case PARAMETER_PG4:
			set_efx_envf_pg4();
			break;
		default:
			break;
	}	
}


/*
*  End envelope filter controls
*/


/*
*  Feedback compressor parameter controls from knobs
*/ 
void set_efx_fbcomp_pg1()
{
	bool doprint = false;
	float thresh, atk, rls, ratio, olevel, wet;
	thresh = atk = rls = ratio = olevel = wet = 0.0;

	if(knobs[FB_COMP_THRS].active_timer != 0) {
		thresh = map(knobs[FB_COMP_THRS].filtered_reading[1], 0.0, 1.0, -60.0, -12.0);
		feedback_compressor_set_threshold(fbcompressor, thresh);
		sprintf(lcdFloatStr,"     %f",thresh);
		sprintf(lcdParamStr,"     Threshold:");
		doprint = true;
	}	
	if(knobs[FB_COMP_RATIO].active_timer != 0) {
		ratio = map(knobs[FB_COMP_RATIO].filtered_reading[1], 0.0, 1.0, 1.0, 24.0);
		feedback_compressor_set_ratio(fbcompressor, ratio);
		sprintf(lcdFloatStr,"      %f",ratio);
		sprintf(lcdParamStr,"       Ratio:");
		doprint = true;
	}	
	if(knobs[FB_COMP_LEVEL].active_timer != 0) {
		olevel = map(knobs[FB_COMP_LEVEL].filtered_reading[1], 0.0, 1.0, -24.0, 6.0);
		feedback_compressor_set_out_gain(fbcompressor, olevel);
		sprintf(lcdFloatStr,"      %f",olevel);
		sprintf(lcdParamStr,"        Gain:");
		doprint = true;
	}	
	if(knobs[FB_COMP_MIX].active_timer != 0) {
		wet = knobs[FB_COMP_MIX].filtered_reading[1];
		feedback_compressor_set_mix(fbcompressor, wet);
		sprintf(lcdFloatStr,"      %f",wet);
		sprintf(lcdParamStr,"        Mix:");
		doprint = true;
	}
	if(knobs[FB_COMP_ATK].active_timer != 0) {
		atk = knobs[FB_COMP_ATK].filtered_reading[1];
		atk *= atk;
		atk = map(knobs[FB_COMP_ATK].filtered_reading[1], 0.0, 1.0, 1.0, 1000.0);
		feedback_compressor_set_attack(fbcompressor, atk);
		sprintf(lcdFloatStr,"     %f",atk);
		sprintf(lcdParamStr,"      Attack:");
		doprint = true;
	}
	if(knobs[FB_COMP_RLS].active_timer != 0) {
		rls = knobs[FB_COMP_RLS].filtered_reading[1];
		rls *= rls;
		rls = map(rls, 0.0, 1.0, 20.0, 1000.0);
		feedback_compressor_set_release(fbcompressor, rls);
		sprintf(lcdFloatStr,"     %f",rls);
		sprintf(lcdParamStr,"      Release:");
		doprint = true;
	}	
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}	
}

void set_efx_fbcomp()
{
	switch(parameter_control[FBCOMP_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_fbcomp_pg1();
			break;
		default:
			break;
	}
}

/*
*  End feedback compressor controls
*/


/*
*	Looper pedal parameter controls from knobs
*/


void set_efx_looper_pg1()
{
	bool doprint = false;
	float volume = 0;
	
	if(knobs[LOOP_VOL].active_timer != 0) {
		volume = map(knobs[LOOP_VOL].filtered_reading[1], 0.0, 1.0, 0.0, 1.5);
		looper.looper_set_volume(volume);
		sprintf(lcdFloatStr,"      %f",volume);
		sprintf(lcdParamStr,"       Volume:");
		doprint = true;
	}
	
	if( (lcdPrintClock >= 5000) && (doprint) ){
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
		lcdPrintClock = 0;
	}
}


void set_efx_looper()
{
	switch(parameter_control[LOOPER_FX_CNTRL].parameter_page)
	{
		case PARAMETER_PG1:
			set_efx_looper_pg1();
			break;
		default:
			break;
	}
}

/*
*  End looper controls
*/


// ------------------  End parameter inputs  ------------------  //






void apply_settings()
{
	if(startup_mask_timer > 0) return;
	
	//Decide who gets analog inputs 
	switch(ain_control_effect)
	{
		case FLANGER_FX_CNTRL :
			set_efx_flange();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(flanger->outGain > 0.0) 
					{
						tflanger_setFinalGain(flanger, 0.0);
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						tflanger_setFinalGain(flanger, 1.0);
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}			
			break;


		case CHORUS_FX_CNTRL :
			set_efx_chorus();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(chorus->outGain > 0.0) {
						tflanger_setFinalGain(chorus, 0.0);
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						tflanger_setFinalGain(chorus, 1.0);
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
	
			}
			break;


		case DELAY_FX_CNTRL :
			set_efx_delay();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(delay->outGain >= 1.0)
					{
						tflanger_setFinalGain(delay, 0.0);
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						tflanger_setFinalGain(delay, 1.0);
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
			
		case WAH_FX_CNTRL :
			set_efx_wah();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(wah->bypass) 
					{
						iwah_bypass(wah, false);
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						iwah_bypass(wah, true);
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			
			
			break;
			
			
		case REVERB_FX_CNTRL :
			set_efx_reverb();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(!reverb.bypass) 
					{
						reverb.bypass = true;
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						reverb.bypass = false;
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
			
		case PHASER_FX_CNTRL :
			set_efx_phase();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(phaser_toggle_bypass(phaser)) 
					{
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
			
		case TREMOLO_FX_CNTRL :
			set_efx_tremolo();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(trem_toggle_bypass(tremolo)) 
					{
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
		case DISTORTION_FX_CNTRL :
			set_efx_distortion();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(distortion.bypass)
					{
						distortion.bypass = false;
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						distortion.bypass = true;
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
			
		case SUSTAINER_FX_CNTRL :	
			set_efx_sustainer();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(sustainer.setBypass()) 
					{
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
			
		case EQ_FX_CNTRL :
			set_efx_geq1();
			break;
			
			
		case ENVF_FX_CNTRL :
			set_efx_envf();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(envf_toggle_bypass(ef)) 
					{
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
			
		case FBCOMP_FX_CNTRL :
			set_efx_fbcomp();
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(feedback_compressor_set_bypass(fbcompressor, false)) 
					{
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
			
		case LOOPER_FX_CNTRL :
			set_efx_looper();
			break;
		case VOLUME_FX_CNTRL :
			if(pushbuttons[EFFECT_BYPASS_SWITCH].rising_edge == true)
			{
					if(volume.bypass)
					{
						volume.bypass = false;
						sprintf(lcdParamStr,"      Enabled");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
					else
					{
						volume.bypass = true;
						sprintf(lcdParamStr,"      Bypassed");
						Bela_scheduleAuxiliaryTask(lcdPrintTask);
					}
			}
			break;
			
			
		default :
			break;
	}

}

void scan_inputs(BelaContext* context)
{

	float tmp = 0.0;
	
	if(knobs[gRoundRobin].knob_toggle_timer > 0)
	{
		knobs[gRoundRobin].knob_toggle_timer--;
		if(knobs[gRoundRobin].knob_toggle_timer == 0) 
			rt_printf("KNOB toggle timer CH %d EXPIRED\n", gRoundRobin);
	}

	for(unsigned int n = 0; n < context->audioFrames; n++) 
	{
		if(!(n % gAudioFramesPerAnalogFrame))
		{
			tmp = analogRead(context, n/gAudioFramesPerAnalogFrame, gRoundRobin);
			knobs[gRoundRobin].current_reading = tmp;
		}
		
		//Filter the analog channel readings
		knobs[gRoundRobin].filtered_reading[0] = knobs[gRoundRobin].a*knobs[gRoundRobin].current_reading + knobs[gRoundRobin].b*knobs[gRoundRobin].filtered_reading[0];		
		knobs[gRoundRobin].filtered_reading[1] = knobs[gRoundRobin].c*knobs[gRoundRobin].filtered_reading[0] + knobs[gRoundRobin].d*knobs[gRoundRobin].filtered_reading[1];	
		knobs[gRoundRobin].filtered_buf[n] = knobs[gRoundRobin].filtered_reading[1];
		lcdPrintClock++;
	}
	
		//evaluate switch switch_state (schmitt trigger on analog input channel)
	// cranking the knob from low to high will set the switch state high and reset hysteresis 
	// then cranking knob from high to low will set the state low and set threshold high again.
	if( (knobs[gRoundRobin].filtered_reading[1] >= SCHMITT_HIGH) && (knobs[gRoundRobin].knob_toggle_timer > 0) ) 
	{
		knobs[gRoundRobin].knob_toggle_timer = 0;
		if(knobs[gRoundRobin].switch_state == false) 
		{
			knobs[gRoundRobin].switch_state = true;
			knobs[gRoundRobin].edge = true;
		} 
		else 
		{
			knobs[gRoundRobin].switch_state = false;
			knobs[gRoundRobin].edge = true;
		}
		
	} 
	else if (knobs[gRoundRobin].filtered_reading[1] <= SCHMITT_LOW)
	{
		knobs[gRoundRobin].knob_toggle_timer = gKnobToggleTimeout;
	}
	
	
	//Check whether a control state changed in order to activate settings
	if(knobs[gRoundRobin].active_timer > 0)
	{
		knobs[gRoundRobin].active_timer--;
	}
	
	if( (knobs[gRoundRobin].scan_timer++) > knobs[gRoundRobin].scan_cycle)
	{
		knobs[gRoundRobin].scan_timer = 0;
		
		if( fabs(knobs[gRoundRobin].last_reading - knobs[gRoundRobin].filtered_reading[1]) > knobs[gRoundRobin].dVdt) 
		{
			knobs[gRoundRobin].active_timer = knobs[gRoundRobin].active_reset;
			knobs[gRoundRobin].last_reading = knobs[gRoundRobin].filtered_reading[1];
		}
		
	}
	
	//Reset virtual multiplexer
	if(++gRoundRobin >= ANALOG_CHANNELS) 
	{
		gRoundRobin = 0;	
	}

	
}

// Extract a single channel from Bela Context and format for guitar sample inputs
void format_audio_buffer(BelaContext* context, float *outbuffer, int channel)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) 
	{
		// Read the audio input
		outbuffer[n] = audioRead(context, n, channel);
		
	}
}

void process_digital_inputs()
{
	// Effect parameters context selection
	if(pushbuttons[PARAMETER_SCROLL_SWITCH].rising_edge == true) 
	{
		 if( ++parameter_control[ain_control_effect].parameter_page >= parameter_control[ain_control_effect].parameter_page_max ) 
		 {
		 		parameter_control[ain_control_effect].parameter_page = 0;
		 }
		 
		 if(parameter_control[ain_control_effect].parameter_page == PARAMETER_PG1)
		 {
		 	sprintf(lcdParamStr,"   Control Page 1");
		 } 
		 else if (parameter_control[ain_control_effect].parameter_page == PARAMETER_PG2)
		 {
		 	sprintf(lcdParamStr,"   Control Page 2");
		 }
		 else if (parameter_control[ain_control_effect].parameter_page == PARAMETER_PG3)
		 {
		 	sprintf(lcdParamStr,"   Control Page 3");
		 }
		 else if (parameter_control[ain_control_effect].parameter_page == PARAMETER_PG4)
		 {
		 	sprintf(lcdParamStr,"   Control Page 4");
		 }
		 Bela_scheduleAuxiliaryTask(lcdPrintTask);
	}
	
	// Which effect is active 
	if(pushbuttons[EFFECT_SCROLL_SWITCH].rising_edge == true)
	{
		if(++ain_control_effect >= NUMBER_FX)
		{
			ain_control_effect = 0;
		}

		if(ain_control_effect == FLANGER_FX_CNTRL)
			sprintf(lcdFxStr, "      Flanger");
		else if (ain_control_effect == DELAY_FX_CNTRL)
			sprintf(lcdFxStr, "       Delay");
		else if (ain_control_effect == CHORUS_FX_CNTRL)
			sprintf(lcdFxStr, "       Chorus");
		else if (ain_control_effect == WAH_FX_CNTRL)
			sprintf(lcdFxStr, "      Wah-Wah");
		else if (ain_control_effect == REVERB_FX_CNTRL)
			sprintf(lcdFxStr, "       Reverb");
		else if (ain_control_effect == PHASER_FX_CNTRL)
			sprintf(lcdFxStr, "       Phaser");
		else if (ain_control_effect == TREMOLO_FX_CNTRL)
			sprintf(lcdFxStr, "      Tremolo");
		else if (ain_control_effect == DISTORTION_FX_CNTRL)
			sprintf(lcdFxStr, "     Distortion");
		else if (ain_control_effect == SUSTAINER_FX_CNTRL)
			sprintf(lcdFxStr, "     Sustainer");
		else if (ain_control_effect == EQ_FX_CNTRL)
			sprintf(lcdFxStr, "         EQ");
		else if (ain_control_effect == ENVF_FX_CNTRL)
			sprintf(lcdFxStr, "  Envelope Filter");
		else if (ain_control_effect == FBCOMP_FX_CNTRL)
			sprintf(lcdFxStr, "Feedback Compressor");
		else if (ain_control_effect == LOOPER_FX_CNTRL)
			sprintf(lcdFxStr, "       Looper");
		else if (ain_control_effect == VOLUME_FX_CNTRL)
			sprintf(lcdFxStr, "    Volume Pedal");
			
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
	}
	
	
	if (pushbuttons[LOOP_REC_SWITCH].rising_edge)
	{
		looper.loopRec = 1;
		if ((looper.loopCase+1) == 1)
			sprintf(lcdParamStr,"  Looper Recording");
		else if ((looper.loopCase+1) == 2)
			sprintf(lcdParamStr," Looper Overdubbing");
		else if ((looper.loopCase+1) == 3)
			sprintf(lcdParamStr,"Playing Loop Output");
		else if ((looper.loopCase+1) == 4)
			sprintf(lcdParamStr," Looper Overdubbing");
		else
			{};
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
	}
	
	else
		looper.loopRec = 0;


	if (pushbuttons[LOOP_CLR_SWITCH].rising_edge)
	{
		looper.loopClr = 1;
		sprintf(lcdParamStr,"   Looper Cleared");
		Bela_scheduleAuxiliaryTask(lcdPrintTask);
	}
	
	else
		looper.loopClr = 0;
}


inline void debounce_digital_inputs(BelaContext *context)
{
	for(int i = 0; i < N_PBTNS; i++) 
		pushbuttons[i].debounce_inputs(digitalRead(context, 0, i));
}


void setup_digital_inputs(BelaContext *context)
{
	parameter_control[FLANGER_FX_CNTRL].parameter_page_max = 3;	//Add the amount of control pages needed for
	parameter_control[CHORUS_FX_CNTRL].parameter_page_max = 3;	//new effects to this list
	parameter_control[DELAY_FX_CNTRL].parameter_page_max = 3;
	parameter_control[WAH_FX_CNTRL].parameter_page_max = 1;
	parameter_control[PHASER_FX_CNTRL].parameter_page_max = 1;
	parameter_control[TREMOLO_FX_CNTRL].parameter_page_max = 1;
	parameter_control[DISTORTION_FX_CNTRL].parameter_page_max = 1;
	parameter_control[SUSTAINER_FX_CNTRL].parameter_page_max = 1;
	parameter_control[ENVF_FX_CNTRL].parameter_page_max = 4;
	parameter_control[FBCOMP_FX_CNTRL].parameter_page_max = 1;
	parameter_control[LOOPER_FX_CNTRL].parameter_page_max = 1;
	parameter_control[VOLUME_FX_CNTRL].parameter_page_max = 1;
	parameter_control[REVERB_FX_CNTRL].parameter_page_max = 2;
	parameter_control[EQ_FX_CNTRL].parameter_page_max = 2;
	
	gSW_timer_max = T_SW_SCAN*context->digitalSampleRate/( context->digitalFrames * 1000);
	gSW_timer = 0;
}


void setup_analog_inputs(BelaContext *context)
{
	float dpfc = 2.0*M_PI*8*(gifs*ANALOG_CHANNELS);
	for(unsigned int i = 0; i<ANALOG_CHANNELS; i++)
	{
		knobs[i].setup_analog_CF(ANALOG_CHANNELS, context->audioFrames, context->audioSampleRate, dpfc);
	}
}


void setup_lcd()
{
	lcd.init();
	lcd.noCursor();
	lcd.backlight();
	lcd.setCursor(3,0);
	lcd.print("Active Effect:");
	lcd.setCursor(6,1);
	lcd.print("Flanger");
}



bool setup(BelaContext *context, void *userData)
{
	gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	gifs = 1.0/context->audioSampleRate;
	gNframes = context->audioFrames;
	

	ch0 = (float*) malloc(sizeof(float)*context->audioFrames);
	ch1 = (float*) malloc(sizeof(float)*context->audioFrames);
	

	startup_mask_timer = 4*((int) context->audioSampleRate);
	
	gKnobToggleTimeout = (KNOB_SWITCH_TIMEOUT*((unsigned int) context->audioSampleRate))/(gNframes * ANALOG_CHANNELS); 
	
	setup_digital_inputs(context);
	setup_analog_inputs(context);
	
	//Delay based effects setup
	gDLY_lfo_type = SINE;
	gCHOR_lfo_type = RELAX;
	gFLANGE_lfo_type = EXP;
	
	delay = tflanger_init(delay, T_ECHO, context->audioSampleRate);
	chorus = tflanger_init(delay, T_CHORUS, context->audioSampleRate);
	flanger = tflanger_init(delay, T_FLANGE, context->audioSampleRate);

	tflanger_setPreset(chorus, 0);
	tflanger_setPreset(delay, 1);
	tflanger_setPreset(flanger, 2);
	
	tflanger_setFinalGain(delay, 0.0);
	tflanger_setFinalGain(chorus, 0.0);
	tflanger_setFinalGain(flanger, 0.0);
	
	//Wah wah setup
	wah = make_iwah(wah, context->audioSampleRate);
	iwah_circuit_preset(VOX, wah, context->audioSampleRate );
	iwah_bypass(wah, true);
	
	//Reverb setup
	reverb.init(context->audioSampleRate, false, context->audioFrames); //false = no ambisonic processing 
	
	//Phaser setup
	phaser = make_phaser(phaser, context->audioSampleRate);
	phaser_circuit_preset(PHASE_90, phaser );
	
	//Tremolo setup
	tremolo = make_trem(tremolo, context->audioSampleRate);
	
	//Sustainer setup
	sustainer.init(context->audioSampleRate, gNframes);
	sustainer.setpreset (0);
	
	//Compressor setup
	fbcompressor = make_feedback_compressor(fbcompressor, context->audioSampleRate, gNframes);
	
	// Graphic EQ setup
	geq1 = make_equalizer(geq1, 6, 164.0, 5980.0, context->audioSampleRate);
	
	//Envelope filter setup
	ef = envf_make_filter(ef, context->audioSampleRate, gNframes);
	gMaster_Envelope = (float*) malloc(sizeof(float)*gNframes);
	for(int i = 0; i<gNframes; i++)
		gMaster_Envelope[i] = 0.0;
		
	
	scope.setup(2, context->audioSampleRate);
	
	lcdPrintTask = Bela_createAuxiliaryTask(lcdPrint, 98, "lcdPrint");		//Create new thread for LCD communication
	
	setup_lcd();

	return true;
}


void render(BelaContext *context, void *userData)
{
	format_audio_buffer(context, ch1, 1);
	format_audio_buffer(context, ch0, 0);
	scan_inputs(context);
	debounce_digital_inputs(context);
	process_digital_inputs();
	apply_settings();

	//CH0 effects
	volume.volume_pedal_tick_n(ch0, knobs[EXP_PEDAL].filtered_buf, gNframes);
	feedback_compressor_tick_n(fbcompressor, ch0, gMaster_Envelope);
	geq_tick_n(geq1, ch0, gNframes);
	sustainer.tick_n(ch0);
	envf_tick_n(ef, ch0, gMaster_Envelope);
	iwah_tick_n(wah, ch0, knobs[EXP_PEDAL].filtered_buf, gNframes);
	trem_tick_n(tremolo, ch0, gNframes);

	//CH1 effects
	distortion.distortion_tick_n(ch0,gNframes);
	phaser_tick_n(phaser, gNframes, ch0);
	tflanger_tick(chorus, gNframes, ch0, gMaster_Envelope);
	tflanger_tick(flanger, gNframes, ch0, gMaster_Envelope);
	tflanger_tick(delay, gNframes, ch0, gMaster_Envelope);
	reverb.tick_mono(gNframes, ch0);

	looper.looper_tick_n(ch0, gNframes);
	
	
	for(unsigned int n = 0; n < context->audioFrames; n++)
	{
		if(startup_mask_timer > 0) 
			startup_mask_timer--;
		scope.log(audioRead(context,n,0)*5, ch0[n]*5);
		audioWrite(context, n, 0, ch0[n]);
		audioWrite(context, n, 1, ch0[n]);
	}	
}

void cleanup(BelaContext *context, void *userData)
{
	lcd.clear();
	lcd.setCursor(1,0);
	lcd.print("Program Terminated");
}