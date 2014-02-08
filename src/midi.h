
// PoMAD Synthol Synthetizer
//
// Laurent Latorre - Polytech Montpellier - 2013
//
// Microelectronics & Robotics Dpt. (MEA)
// http://www.polytech.univ-montp2.fr/MEA
//
// Embedded Systems Dpt. (SE)
// http://www.polytech.univ-montp2.fr/SE

#define MIDI_BUFFER_LENGTH		64


// Start MIDI interface and return a pointer to the MIDI buffer
uint8_t* MIDI_Start (void);

// Returns the number of new bytes in the MIDI buffer

uint8_t	MIDI_GetNbNewBytes (uint8_t*);

// Parse MIDI bytes

void MIDI_parser (uint8_t*, uint8_t);

void ChangeParam(uint8_t, uint8_t);
