
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

#define _2PI                    6.283185307f
#define _PI						3.14159265f
#define _INVPI					0.3183098861f

#define SAMPLERATE              48000
#define BUFF_LEN_DIV4           160 	// number of samples <==> 3.3 ms latency at 48kHz
#define BUFF_LEN_DIV2           320		// Audio buffer length
#define BUFF_LEN                640  	// Audio buffer length for both Left & Right Channels

#define WTB_LEN					1024	// Length of the oscillator wavetable

#define VOL                     85		// Max volume before saturation


//	Data type declaration

typedef struct synth_params synth_params;

struct synth_params
{
    float_t 	pitch;
    float_t		bend;

    float_t		detune;

    uint8_t		osc1_waveform, osc2_waveform;
    float_t		osc1_octave, osc2_octave;
    float_t		osc1_mix, osc2_mix;

    float_t		cutoff;
    float_t		reso;

    float_t		adsr1_attack;
    float_t		adsr1_decay;
    float_t		adsr1_sustain;
    float_t		adsr1_release;

    float_t		adsr2_attack;
    float_t		adsr2_decay;
    float_t		adsr2_sustain;
    float_t		adsr2_release;

    float_t		lfo1_frequency;
    float_t		lfo1_depth;

    float_t		lfo2_frequency;
    float_t		lfo2_depth;
};

void Make_Sound(uint16_t);
