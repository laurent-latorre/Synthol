
// PoMAD Synthol Synthetizer
//
// Laurent Latorre - Polytech Montpellier - 2013
//
// Microelectronics & Robotics Dpt. (MEA)
// http://www.polytech.univ-montp2.fr/MEA
//
// Embedded Systems Dpt. (SE)
// http://www.polytech.univ-montp2.fr/SE


#include <stdio.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

#include "llist.h"

#include "midi.h"
#include "synth.h"
#include "cc_lut.h"

extern llist 			note_list;						// Note list
extern synth_params		params;							// Synthesizer parameters

extern bool				new_note_event;



// Start MIDI Input with USART3 and DMA1
// -------------------------------------

uint8_t* MIDI_Start()
{
	static uint8_t MIDI_buffer[MIDI_BUFFER_LENGTH];

	GPIO_InitTypeDef  	GPIO_InitStructure;
	USART_InitTypeDef 	USART_InitStructure;
	NVIC_InitTypeDef 	NVIC_InitStructure;
	DMA_InitTypeDef  	DMA_InitStructure;

	// Start USART3 clock

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	// Start DMA1 Clock

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

	// Initialize TX & RX Pins (USART3 TX & RX on PD8 & PD9)

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// Connect the RX and TX pins to USART3 alternative function

	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);

	// Setup the properties of USART3

	USART_InitStructure.USART_BaudRate = 31250;												// MIDI baudrate (31250)
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;								// 8 bits data
	USART_InitStructure.USART_StopBits = USART_StopBits_1;									// 1 stop bit
	USART_InitStructure.USART_Parity = USART_Parity_No;										// No parity bit
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; 		// No flow control
	USART_InitStructure.USART_Mode = USART_Mode_Rx; 										// RX only is enabled
	USART_Init(USART3, &USART_InitStructure);

	// Enable the USART3 RX DMA Interrupt

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Configure the Priority Group to 2 bits

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	// finally this enables the complete USART3 peripheral

	USART_Cmd(USART3, ENABLE);

	// Configure DMA to store MIDI bytes directly into MIDI_buffer

	DMA_DeInit(DMA1_Stream1);

	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)MIDI_buffer;
	DMA_InitStructure.DMA_BufferSize = (uint16_t)sizeof(MIDI_buffer);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	DMA_Init(DMA1_Stream1, &DMA_InitStructure);

	// Enable the USART Rx DMA request

	USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);

	// Enable the DMA RX Stream

	DMA_Cmd(DMA1_Stream1, ENABLE);

	return (MIDI_buffer);
}


uint8_t MIDI_GetNbNewBytes(uint8_t* MIDI_Buffer)
{
	static uint16_t 	dma_cpt_prev=MIDI_BUFFER_LENGTH;
	uint16_t			dma_cpt, n=0;

	// Get current DMA counter

	dma_cpt = DMA_GetCurrDataCounter(DMA1_Stream1);

	// If DMA counter has changed, compute the number of received MIDI bytes

	if (dma_cpt != dma_cpt_prev)
	{
		if (dma_cpt < dma_cpt_prev)
		{
			n = dma_cpt_prev - dma_cpt;
		}
		else
		{
			n = dma_cpt_prev - (dma_cpt - MIDI_BUFFER_LENGTH);
		}

		// Store the new DMA counter

		dma_cpt_prev = dma_cpt;
	}
	return(n);
}


void MIDI_parser(uint8_t* MIDI_buffer, uint8_t nb_MIDI_bytes)
{
	static uint8_t 	index = 0;
	static uint8_t	state = 0;

	static uint8_t	MIDI_note, MIDI_velocity;
	static uint8_t  MIDI_CC_number, MIDI_CC_value;
	static uint8_t  MIDI_PB_byte1, MIDI_PB_byte2;

	uint8_t			MIDI_byte;

	uint16_t		PB_wheel;

	// Process message

	while (nb_MIDI_bytes != 0)
	{
		// Read a new byte from the MIDI buffer

		MIDI_byte = MIDI_buffer[index];

		//printf ("0x%x @index = %d\n", MIDI_byte, index);

		// Move to next byte

		switch (state)
		{

			// State 0 = Starting point for a new MIDI message

			case 0 :
			{
				switch (MIDI_byte & 0xF0)
				{

					case 0x90 :														// Note ON message
					{
						state = 10;													// Next state is 10

						// printf ("note ON event\n");

						if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_MIDI_bytes--;

						break;
					}


					case 0x80 :														// Note OFF message
					{
						state = 20;													// Next state is 20

						// printf ("note OFF event\n");

						if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_MIDI_bytes--;

						break;
					}

					case 0xB0 :														// CC message
					{
						state = 30;													// Next state is 30

						// printf ("CC event\n");

						if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_MIDI_bytes--;

						break;
					}

					case 0xE0 :														// Pitch Bend message
					{
						state = 40;													// Next state is 40

						// printf ("PB event\n");

						if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_MIDI_bytes--;

						break;
					}


					default :														// Other type of message, move to next byte but stays in state 0
					{
						if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
						else index ++;
						nb_MIDI_bytes--;

						break;
					}
				}

				break;
			}


			// State 10 & 11 : Note ON command

			case 10 :
			{
				if (MIDI_byte>0x7F)												// If the following byte is not a note number
				{
					state = 0;													// Return to state 0 without moving to next byte
				}

				else
				{
					MIDI_note = MIDI_byte;										// Save MIDI note

					if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
					else index ++;
					nb_MIDI_bytes--;

					state = 11;													// Next state is 11
				}

				break;
			}

			case 11 :
			{
				MIDI_velocity = MIDI_byte;										// Save MIDI velocity

				if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;					// Move to next MIDI byte
				else index ++;
				nb_MIDI_bytes--;

				state = 10;														// Next state is 10

				if (MIDI_velocity > 0)
				{
					printf ("Note ON : %d %d\n", MIDI_note, MIDI_velocity);
					note_list = add_note_last(note_list, MIDI_note, MIDI_velocity);

					new_note_event = 1;
				}
				else
				{
					printf ("Note OFF : %d %d\n", MIDI_note, MIDI_velocity);
					note_list = delete_note(note_list, MIDI_note);
				}


				break;
			}

			// State 20 & 21 : Note OFF command

			case 20 :
			{
				if (MIDI_byte>0x7F)												// If the following byte is not a note number
				{
					state = 0;													// Return to state 0 without moving to next byte
				}

				else
				{
					MIDI_note = MIDI_byte;										// Save MIDI note

					if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
					else index ++;
					nb_MIDI_bytes--;

					state = 21;													// Next state is 21
				}

				break;
			}

			case 21 :
			{
				MIDI_velocity = MIDI_byte;										// Save MIDI velocity

				if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;					// Move to next MIDI byte
				else index ++;
				nb_MIDI_bytes--;

				state = 20;														// Next state is 20

				printf ("Note OFF : %d %d\n", MIDI_note, MIDI_velocity);
				note_list = delete_note(note_list, MIDI_note);

				break;
			}


			// State 30 & 31 : CC command

			case 30 :
			{
				if (MIDI_byte>0x7F)												// If the following byte is not a CC number
				{
					state = 0;													// Return to state 0 without moving to next byte
				}

				else
				{
					MIDI_CC_number = MIDI_byte;									// Save MIDI CC number

					if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
					else index ++;
					nb_MIDI_bytes--;

					state = 31;													// Next state is 31
				}

				break;
			}

			case 31 :
			{
				MIDI_CC_value = MIDI_byte;										// Save MIDI velocity

				if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;					// Move to next MIDI byte
				else index ++;
				nb_MIDI_bytes--;

				state = 30;														// Next state is 30

				printf ("CC : %d %d\n", MIDI_CC_number, MIDI_CC_value);
				ChangeParam(MIDI_CC_number, MIDI_CC_value);

				break;
			}



			// State 40 & 41 : Pitch Bend message

			case 40 :
			{
				if (MIDI_byte > 0x7F)												// If following byte is note a PB value
				{
					state = 0;													// Return to state 0
				}

				else
				{
					MIDI_PB_byte1 = MIDI_byte;									// Save MIDI CC number

					if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;				// Move to next MIDI byte
					else index ++;
					nb_MIDI_bytes--;

					state = 41;													// Next state is 41
				}

				break;
			}

			case 41 :
			{
				MIDI_PB_byte2 = MIDI_byte;										// Save MIDI velocity

				if (index == (MIDI_BUFFER_LENGTH-1)) index = 0;					// Move to next MIDI byte
				else index ++;
				nb_MIDI_bytes--;

				PB_wheel = (uint8_t) MIDI_PB_byte2;
				PB_wheel <<=7;
				PB_wheel |= (uint8_t) MIDI_PB_byte1;

				// printf ("PB : %x\n", PB_wheel);

				params.bend = params.pitch * 2.0f * ( (float_t)PB_wheel - 8192.0f) / (4096.0f * 12.0f);

				state = 40;														// Next state is 00

				break;
			}

		}
	}





}


void ChangeParam(uint8_t param_number, uint8_t param_value)
{
	switch (param_number)
	{
		case 32 :								// Button A1 -> Browse OSC1 Waveforms
		{
			if (param_value == 127)
			{
				params.osc1_waveform++;
				if (params.osc1_waveform == 5) params.osc1_waveform = 0;

				//printf ("Osc1 waveform set to %d\n", params.osc1_waveform);
			}
			break;
		}

		case 33 :								// Button A2 -> Browse OSC2 Waveforms
		{
			if (param_value == 127)
			{
				params.osc2_waveform++;
				if (params.osc2_waveform == 5) params.osc2_waveform = 0;

				//printf ("Osc2 waveform set to %d\n", params.osc2_waveform);
			}
			break;
		}

		case 88 :								// Button A2 -> Browse OSC2 Waveforms
		{
			if (param_value == 127)
			{
				params.osc1_octave = params.osc1_octave * 2.0f;
				if (params.osc1_octave >= 4.0f) params.osc1_octave = 0.5f;

				//printf ("Osc2 waveform set to %d\n", params.osc2_waveform);
			}
			break;
		}

		case 89 :								// Button A2 -> Browse OSC2 Waveforms
		{
			if (param_value == 127)
			{
				params.osc2_octave = params.osc2_octave * 2.0f ;
				if (params.osc2_octave >= 4.0f) params.osc2_octave = 0.5f;

				//printf ("Osc2 waveform set to %d\n", params.osc2_waveform);
			}
			break;
		}

		case 56 :								// OSC1 mix level
		{
			params.osc1_mix = (float_t) param_value / 127;
			break;
		}

		case 57 :								// OSC2 mix level
		{
			params.osc2_mix = (float_t) param_value / 127;
			break;
		}

		case 58 :								// Filter cutoff
		{
			params.cutoff = cutoff_CC[param_value];
			//params.cutoff = (float_t) 1.0f + (200.0f * param_value / 127);
			break;
		}

		case 59 :								// Filter resonance
		{
			params.reso = 4.0f * (float_t) param_value / 127;
			break;
		}

		case 48 :								// ADRS Attack Time
		{
			params.adsr1_attack = 0.001f + (float_t) param_value / 127;
			break;
		}

		case 49 :								// ADRS Decay Time
		{
			params.adsr1_decay = 0.001f + (float_t) param_value / 127;
			break;
		}

		case 50 :								// ADRS Sustain Level
		{
			params.adsr1_sustain = (float_t) param_value / 127;
			break;
		}

		case 51 :								// ADRS Release Time
		{
			params.adsr1_release = 0.001f + (float_t) param_value / 127;
			break;
		}


		case 52 :								// ADRS Attack Time
		{
			params.adsr2_attack = 0.001f + (float_t) param_value / 127;
			break;
		}

		case 53 :								// ADRS Decay Time
		{
			params.adsr2_decay = 0.001f + (float_t) param_value / 127;
			break;
		}

		case 54 :								// ADRS Sustain Level
		{
			params.adsr2_sustain = (float_t) param_value / 127;
			break;
		}

		case 55 :								// ADRS Release Time
		{
			params.adsr2_release = 0.001f + (float_t) param_value / 127;
			break;
		}

		case 60 :								// ADRS Release Time
		{
			params.lfo1_frequency = 10.0f * (float_t) param_value / 127;
			break;
		}

		case 61 :								// ADRS Release Time
		{
			params.lfo1_depth = 1.0f * (float_t) param_value / 127;
			break;
		}

		case 62 :								// ADRS Release Time
		{
			params.lfo2_frequency = 10.0f * (float_t) param_value / 127;
			break;
		}

		case 63 :								// ADRS Release Time
		{
			params.lfo2_depth = 0.1f * (float_t) param_value / 127;
			break;
		}

		case 66 :								// ADRS Release Time
				{
					params.detune = 1.0f + 0.0004f*(param_value-64);
					break;
				}


	}

}



