
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
#include <stdio.h>

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery_audio_codec.h"

#include "main.h"
#include "pitch.h"
#include "llist.h"
#include "midi.h"
#include "synth.h"


// GOBAL DEBUG VARIABLE

float_t		global_pitch;
float_t		global_fc;


// G L O B A L   V A R I A B L E S

// Audio & MIDI buffers

uint16_t	audiobuff[BUFF_LEN];		// Circular audio buffer

// Note List

llist 		note_list = NULL;

// Synthesizer parameters structure

synth_params	params;
bool			trig;
bool			new_note_event;


// F U N C T I O N S    P R O T O T Y P E S

void LED_PUSH_Start(void);
void FDTI_Start(void);


// M A I N    P R O G R A M

int main(void)
{
	int 	i;					// General purpose variable

	uint8_t*	pMIDI_buffer;		// MIDI variables
	uint8_t		nb_MIDI_bytes;
	note* 		play_note;


	// Initializations
	// ---------------

	// Start peripherals

	LED_PUSH_Start();					// Start Discovery KIT LEDS and PUSH BUTTON peripherals
	FDTI_Start();						// Start the debug terminal (printf)
	pMIDI_buffer = MIDI_Start();		// Start the MIDI input interface (DMA)


	// Empty the audio buffer and initialize sample pointer

	for (i=0; i<BUFF_LEN; i=i+2)
		{
			audiobuff[i] = (uint16_t)((int16_t) 0.0f);			// Left Channel value
			audiobuff[i+1] = (uint16_t)((int16_t) 0.0f);		// Right Channel Value
		}


	// Initialize Synth

	params.pitch = 220.0f;
	params.bend = 0.0f;

	params.detune = 1.0f;

	params.osc1_waveform = 3;
	params.osc2_waveform = 3;

	params.osc1_octave = 1.0f;
	params.osc2_octave = 1.0f;

	params.osc1_mix = 0.2f;
	params.osc2_mix = 0.2f;

	params.cutoff = 24.0f;
	params.reso = 1.0f;

	params.adsr1_attack = 0.1f;
	params.adsr1_decay = 0.1f;
	params.adsr1_sustain = 0.5f;
	params.adsr1_release = 0.1f;

	params.adsr2_attack = 0.1f;
	params.adsr2_decay = 0.1f;
	params.adsr2_sustain = 0.5f;
	params.adsr2_release = 0.1f;

	params.lfo1_frequency = 10.0f;
	params.lfo1_depth = 0.8f;

	params.lfo2_frequency = 10.0f;
	params.lfo2_depth = 0.8f;


	// Start Audio

	EVAL_AUDIO_Init(OUTPUT_DEVICE_AUTO, VOL, SAMPLERATE);
    EVAL_AUDIO_Play((uint16_t*)audiobuff, BUFF_LEN);


    // Test Terminal

    printf("\n\n");
    printf("Init terminated \n");


	// Main Loop
    // ---------


    while(1)
    {
    	// Test if MIDI bytes have arrived

    	nb_MIDI_bytes = MIDI_GetNbNewBytes(pMIDI_buffer);

    	// If yes, parse message

    	if (nb_MIDI_bytes)
    	{
    		// printf("bytes arrived : %d\n", nb_MIDI_bytes);
    		MIDI_parser(pMIDI_buffer, nb_MIDI_bytes);
    	}

		// Find if there is a note to play

		play_note = get_last_note(note_list);

		if (play_note == NULL)
		{
			trig = 0;
		}
		else
		{
			params.pitch = pitch_table[(play_note->midi_note)-24];
			trig = 1;
		}
    }
}



// F U N C T I O N S


// Start STM32F4 Discovery LED and PUSH BUTTON

void LED_PUSH_Start()
{
	GPIO_InitTypeDef  	GPIO_InitStructure;


	// Start GPIOA & GPIOD Clocks

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// Initialize LEDS IOs (GPIOD[12,13,14,15])

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// Initialize Push Button (GPIOA[0])

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}


// Start degug terminal on USART2 (TX only)

void FDTI_Start()
{
	GPIO_InitTypeDef  	GPIO_InitStructure;
	USART_InitTypeDef 	USART_InitStructure;

	// Start USART2 clock

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	// GPIOA Configuration for USART2 (TX on PA2)

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Connect the TX pin (PA2) to USART2 alternative function

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);

	// Setup the properties of USART2

	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	// Enable USART2

	USART_Cmd(USART2, ENABLE);
}



// I N T E R R U P T S    H A N D L E S


// DMA half & complete transfer Callbacks

void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size)
{
	GPIO_SetBits(GPIOD, GPIO_Pin_12);			// LED12
	Make_Sound(BUFF_LEN_DIV2);					// Fill buffer second half
}

void EVAL_AUDIO_HalfTransfer_CallBack(uint32_t pBuffer, uint32_t Size)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_12);			// LED12
	Make_Sound(0);								// Fill buffer first half
}


// These ones are implemented "just in case" but should not occur

uint16_t EVAL_AUDIO_GetSampleCallBack(void)
{
	return 0;
}

uint32_t Codec_TIMEOUT_UserCallback(void)
{
	return (0);
}

void USART3_IRQHandler(void)
{
	// printf("Here is a USART3 interrupt\n");
}

void DMA1_Stream1_IRQHandler(void)
{
	// printf("Here is a DMA interrupt\n");

	// By clearing this flag, the DMA counter restarts at the beginning

	if (DMA_GetITStatus(DMA1_Stream1, DMA_IT_TCIF1))
	  {
	    DMA_ClearITPendingBit(DMA1_Stream1, DMA_IT_TCIF1);
	  }
}
