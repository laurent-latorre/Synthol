
// PoMAD Synthol Synthetizer
//
// Laurent Latorre - Polytech Montpellier - 2013
//
// Microelectronics & Robotics Dpt. (MEA)
// http://www.polytech.univ-montp2.fr/MEA
//
// Embedded Systems Dpt. (SE)
// http://www.polytech.univ-montp2.fr/SE

#include <math.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

#include "synth.h"
#include "wavetable_24.h"


extern uint16_t			audiobuff[BUFF_LEN];			// Circular audio buffer
extern synth_params		params;							// Synthesizer parameters
extern bool				trig;
extern bool				new_note_event;

extern float_t	global_fc, global_pitch;


float fastsin(uint32_t);


void Make_Sound(uint16_t start_index)
{
	// Local variables

	uint16_t 			i;															// Audio buffer index

	float_t				pitch;

	static float_t		osc_1_wtb_pointer = 0;										// Pointer in OSC1 wavetable
	static float_t		osc_2_wtb_pointer = 0;										// Pointer in OSC2 wavetable

	float_t				osc_1_wtb_incr;												// OSC1 sample increment
	float_t				osc_2_wtb_incr;												// OSC2 sample increment


	uint16_t 			a,b;														// Index of bounding samples
	float_t				da, db;														// Distances to bounding samples

	static float_t		osc_1, osc_2;												// Individual oscillator audio output

	static float_t		f, fb, in1=0, in2=0, in3=0, in4=0, out1=0, out2=0, out3=0, out4=0;

//	static float_t		xa=0, xa1=0, xa2=0, ya=0, ya1=0, ya2=0;
//	static float_t		xb=0, xb1=0, xb2=0, yb=0, yb1=0, yb2=0;

	static float_t		signal=0;
	static float_t		signal_pf=0;

	static float_t		adsr1_output=0;												// ADSR output
	static uint8_t		adsr1_state=0;												// ADSR state (0=Off, 1=Attack, 2=Decay, 3=Sustain, 4=Release)
	static float_t		attack1_incr=0;												// ADSR increment
	static float_t		decay1_incr=0;
	static float_t		release1_incr=0;

	static float_t		adsr2_output=0;												// ADSR output
	static uint8_t		adsr2_state=0;												// ADSR state (0=Off, 1=Attack, 2=Decay, 3=Sustain, 4=Release)
	static float_t		attack2_incr=0;												// ADSR increment
	static float_t		decay2_incr=0;
	static float_t		release2_incr=0;

	static float_t		lfo1_wtb_pointer=0;
	float_t				lfo1_wtb_incr=0;
	float_t				lfo1_output=0;

	static float_t		lfo2_wtb_pointer=0;
	float_t				lfo2_wtb_incr=0;
	float_t				lfo2_output=0;


	// Step 1 : Audio pre-processing (compute every half buffer)
	// ---------------------------------------------------------

	GPIO_SetBits(GPIOD, GPIO_Pin_13);					// Set LED_13 (for debug)


	// Compute ADSR1 & ADSR2 state & increment

	if (new_note_event)																// If there is a new note to play
	{
		new_note_event = 0;															// Reset flag
		adsr1_state = 1;															// Set ADSR to 'Attack' state
		adsr2_state = 1;

		attack1_incr = (1 - adsr1_output)/(params.adsr1_attack * SAMPLERATE);		// Compute increment
		attack2_incr = (1 - adsr2_output)/(params.adsr2_attack * SAMPLERATE);
	}

	if (trig == 0)																	// If there is no note to play
	{
		if (adsr1_state !=4)
		{
			adsr1_state = 4;														// If not already done, set ADSR to 'release' state
			release1_incr = adsr1_output /(params.adsr1_release * SAMPLERATE);		// Compute increment
		}

		if (adsr2_state !=4)
		{
			adsr2_state = 4;														// If not already done, set ADSR to 'release' state
			release2_incr = adsr2_output /(params.adsr2_release * SAMPLERATE);		// Compute increment
		}

	}

	decay1_incr = (1-params.adsr1_sustain) / (params.adsr1_decay * SAMPLERATE);
	decay2_incr = (1-params.adsr2_sustain) / (params.adsr2_decay * SAMPLERATE);


	// Step 2 : Fills the buffer with individual samples
	// -------------------------------------------------


	i = 0;

	while(i<BUFF_LEN_DIV2)
	{

		// Compute LFO1 parameters

		lfo1_wtb_incr = WTB_LEN * params.lfo1_frequency / SAMPLERATE;		// Increment value of the LFO wavetable pointer

		lfo1_wtb_pointer = lfo1_wtb_pointer + lfo1_wtb_incr;

		if (lfo1_wtb_pointer > WTB_LEN)
		{
			lfo1_wtb_pointer = lfo1_wtb_pointer - WTB_LEN;
		}

		lfo1_output = params.lfo1_depth * sinewave[(uint16_t)lfo1_wtb_pointer];

		if (params.lfo1_depth==0) lfo1_output = 0;


		// Compute LFO2 parameters

		lfo2_wtb_incr = WTB_LEN * params.lfo2_frequency / SAMPLERATE;		// Increment value of the LFO wavetable pointer

		lfo2_wtb_pointer = lfo2_wtb_pointer + lfo2_wtb_incr;

		if (lfo2_wtb_pointer > WTB_LEN)
		{
			lfo2_wtb_pointer = lfo2_wtb_pointer - WTB_LEN;
		}

		lfo2_output = params.lfo2_depth * sinewave[(uint16_t)lfo2_wtb_pointer];

		if (params.lfo1_depth==0) lfo1_output = 0;


		// Compute pitch & oscillators increments

		pitch = params.pitch * (1 + lfo2_output) + params.bend;

		//osc_1_wtb_incr = WTB_LEN * (pitch * params.osc1_octave * (1.0f+0.5f*osc_2)) / SAMPLERATE;						// Increment value of the OSC1 wavetable pointer
		osc_1_wtb_incr = WTB_LEN * (pitch * params.osc1_octave) / SAMPLERATE;						// Increment value of the OSC1 wavetable pointer
		osc_2_wtb_incr = WTB_LEN * (pitch * params.osc2_octave * params.detune) / SAMPLERATE;		// Increment value of the OSC2 wavetable pointer


		// OSC1 Perform Linear Interpolation between 'a' and 'b' points

		a  = (int) osc_1_wtb_pointer;									// Compute 'a' and 'b' indexes
		da = osc_1_wtb_pointer - a;										// and both 'da' and 'db' distances
		b  = a + 1;
		db = b - osc_1_wtb_pointer;

		if (b==WTB_LEN) b=0;											// if 'b' passes the end of the table, use 0

		switch (params.osc1_waveform)
		{
			case 0 :
			{
				osc_1 = db*square[a] + da*square[b];					// Linear interpolation (same as weighted average)
				break;
			}

			case 1 :
			{
				osc_1 = db*triangle[a] + da*triangle[b];
				break;
			}

			case 2 :
			{
				osc_1 = db*sawtooth[a] + da*sawtooth[b];
				break;
			}

			case 3 :
			{
				osc_1 = db*distosine[a] + da*distosine[b];
				break;
			}

			default :
			{
				osc_1 = db*sinewave[a] + da*sinewave[b];
				break;
			}
		}


		// OSC2 Perform Linear Interpolation between 'a' and 'b' points

		a  = (int) osc_2_wtb_pointer;									// Compute 'a' and 'b' indexes
		da = osc_2_wtb_pointer - a;										// and both 'da' and 'db' distances
		b  = a + 1;
		db = b - osc_2_wtb_pointer;

		if (b==WTB_LEN) b=0;											// if 'b' passes the end of the table, use 0

		switch (params.osc2_waveform)
		{
			case 0 :
			{
				osc_2 = db*square[a] + da*square[b];					// Linear interpolation (same as weighted average)
				break;
			}

			case 1 :
			{
				osc_2 = db*triangle[a] + da*triangle[b];
				break;
			}

			case 2 :
			{
				osc_2 = db*sawtooth[a] + da*sawtooth[b];
				break;
			}

			case 3 :
			{
				osc_2 = db*distosine[a] + da*distosine[b];
				break;
			}

			default :
			{
				osc_2 = db*sinewave[a] + da*sinewave[b];
				break;
			}
		}

		// Oscillator Mixer section

		signal = (params.osc1_mix * osc_1) + (params.osc2_mix * osc_2);


		// Pseudo MOOG Filter Section

		f =  (float_t) params.cutoff * (1.0f+ lfo1_output) * (1.0f + adsr2_output) * pitch * 1.16f / (SAMPLERATE/2) ;

		if (f>1.0f) f=1.0f;

		fb = (float_t) params.reso * (1.0f - 0.15f * f*f);

		signal -= out4 * fb;
		signal *= 0.35013f * (f*f) * (f*f);
		out1 = signal + 0.3f * in1 + (1-f) * out1; 				// Pole 1
		in1 = signal;
		out2 = out1 + 0.3f * in2 + (1-f) * out2; 				// Pole 2
		in2 = out1;
		out3 = out2 + 0.3f * in3 + (1-f) * out3; 				// Pole 3
		in3 = out2;
		out4 = out3 + 0.3f * in4 + (1-f) * out4; 				// Pole 4
		in4 = out3;

		signal_pf = out4;


		// ADSR1 section

		switch (adsr1_state)
		{
			case 0 :											// Off
			{
				adsr1_output = 0;
				break;
			}

			case 1 :											// Attack
			{
				adsr1_output = adsr1_output + attack1_incr;

				if (adsr1_output > 1)
				{
					adsr1_output = 1;
					adsr1_state = 2;
					GPIO_SetBits(GPIOD, GPIO_Pin_15);
				}

				break;
			}

			case 2 :											// Decay
			{
				adsr1_output = adsr1_output - decay1_incr;

				if (adsr1_output < params.adsr1_sustain)
				{
					adsr1_output = params.adsr1_sustain;
					adsr1_state = 3;
				}
				break;
			}

			case 3 :											// Sustain
			{
				adsr1_output = params.adsr1_sustain;
				break;
			}

			case 4 :											// Release
			{
				adsr1_output = adsr1_output - release1_incr;

				if (adsr1_output < 0)
				{
					adsr1_output = 0;
					adsr1_state = 0;
					GPIO_ResetBits(GPIOD, GPIO_Pin_15);
				}
				break;
			}
		}


		// ADSR2 section

		switch (adsr2_state)
		{
			case 0 :											// Off
			{
				adsr2_output = 0;
				break;
			}

			case 1 :											// Attack
			{
				adsr2_output = adsr2_output + attack2_incr;

				if (adsr2_output > 1)
				{
					adsr2_output = 1;
					adsr2_state = 2;
					GPIO_SetBits(GPIOD, GPIO_Pin_15);
				}

				break;
			}

			case 2 :											// Decay
			{
				adsr2_output = adsr2_output - decay2_incr;

				if (adsr2_output < params.adsr2_sustain)
				{
					adsr2_output = params.adsr2_sustain;
					adsr2_state = 3;
				}
				break;
			}

			case 3 :											// Sustain
			{
				adsr2_output = params.adsr2_sustain;
				break;
			}

			case 4 :											// Release
			{
				adsr2_output = adsr2_output - release2_incr;

				if (adsr2_output < 0)
				{
					adsr2_output = 0;
					adsr2_state = 0;
					GPIO_ResetBits(GPIOD, GPIO_Pin_15);
				}
				break;
			}
		}


		// VCA section

		signal = adsr1_output * 32767.0f * signal_pf;

		if (signal >  32767.0f) signal =  32767.0f;
		if (signal < -32767.0f) signal = -32767.0f;


		// Fill the buffer

		audiobuff[start_index+i] =   (uint16_t)((int16_t) signal);		// Left Channel value
		audiobuff[start_index+i+1] = (uint16_t)((int16_t) signal);		// Right Channel Value


		// Update pointers for next loop

		i = i+2;															// Audio buffer (L+R)

		osc_1_wtb_pointer = osc_1_wtb_pointer + osc_1_wtb_incr;				// WTB buffer
		if (osc_1_wtb_pointer>WTB_LEN)										// return to zero if end is reached
		{
			osc_1_wtb_pointer = osc_1_wtb_pointer - WTB_LEN;
		}

		osc_2_wtb_pointer = osc_2_wtb_pointer + osc_2_wtb_incr;				// WTB buffer
		if (osc_2_wtb_pointer>WTB_LEN)										// return to zero if end is reached
		{
			osc_2_wtb_pointer = osc_2_wtb_pointer - WTB_LEN;
		}
	}

	GPIO_ResetBits(GPIOD, GPIO_Pin_13);

}







