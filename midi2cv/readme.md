# Arduino files for DK-synth midi->cv module
## v1.1 Feb 16, 2024
* Adds pitch bend and modulation control functionality
  - Pitch bend is enabled for all monophonic and polyphonic configurations and depending on the controller should bend the pitch +/- a few semitones.
  - Modulation is enabled in the following modes:
    - Monophonic/Solo (left switch up, right switch up)
    - Polyphonic/Cycle (left switch down, right switch down)
  - When modulation is enabled, Pitch out 3 on the MIDI-CV module becomes a CV control out for monophonic/solo mode,  and Pitch outs 3 and 4 become CV control outs for polyphonic/cycle mode. Instead of routing them to V/OCT on a VCO like you normally would, they can be routed to "PWM" inputs on the VCO, and you can control pulse width of a square wave using the mod strip/lever on your MIDI device. 
