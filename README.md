Synthol
=======

STM32F4 based Monophonic Virtual Analog Synth

Summary of implemented features :

- 2 oscillators based on wavetables with 24 overtone (square, triangle, sawtooth, sine)
- OSC1 & OSC2 can be switched over 3 octaves
- OSC2 detune
- Noise generator based on STM32 RNG
- MOOG-like low-pass filter with resonance
- 2 LFO's (here mapped to pitch and filter cutoff)
- 2 ADSR enveloppe generators (here mapped to VCA and filter cutoff)
- Switchable Ring Modulation (OSC2 -> OSC1)
- Pitch-bend
- MOD wheel mapped to filter resonance

Code is provided as is it with no support (sorry, I'm too busy).

I apologize for my very poor coding skills, I'm not a computer engineer, more on the hardware side of these stuff...

I'm sure you can improve things a lot...

Built OK with CoIDE 1.7.5 (enable hardware FPU) and GNU Tools ARM Embedded\4.6 2012q2 toolchain

You'll need a MIDI master kayboard to send note & CC events to hear something and play with parameters. You'll also need 
a litte bit of harware to wire MIDI signals into USART3.

See midi.c file to find out MIDI implementation (sorry, no documentation, but it's quite easy to understand)

For demo, see : https://www.youtube.com/watch?v=SS7UrrUPdn0

Good Luck !

Laurent.




