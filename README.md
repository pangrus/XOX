# XOX
4 midi channel drum module for Teensy 3.2 with SGTL5000 audio shield

#### midi channels
CH 6  - Kick   (tuned on note frequency)
CH 7  - Tom    (tuned on note frequency)
CH 8  - Noise  (note frequency affects filter cutoff)
CH 9  - Cymbal (note frequency affects filter cutoff)
CH 10 - Snare  (note frequency affects filter cutoff)

#### midi CC
CC  0  - Main output level
CC 16  - Kick level
CC 25  - Kick pitch modulation
CC 18  - Tom level
CC 48  - Tom pitch modulation
CC 74  - Cymbal level
CC 61  - Noise level
CC 102 - Snare level
Velocity is mapped on release time

