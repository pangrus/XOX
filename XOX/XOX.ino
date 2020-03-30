///////////////////////////////////////////////////////////////
//                                                           //
// _______________________X [O] X____________________________//
//                                                           //
//                5 midi channel drum module                 //
//         for Teensy 3.2 with SGTL5000 audio shield         //
//                                                           //
//   CH 6  - Kick   (tuned on note frequency)                //
//   CH 7  - Tom    (tuned on note frequency)                //
//   CH 8  - Noise  (note frequency affects filter cutoff)   //
//   CH 9  - Cymbal (note frequency affects filter cutoff)   //
//   CH 10 - Snare  (note frequency affects filter cutoff)   //
//                                                           //
//   CC  0  - Main output level                              //
//   CC 16  - Kick level                                     //
//   CC 25  - Kick pitch modulation                          //
//   CC 18  - Tom level                                      //
//   CC 48  - Tom pitch modulation                           //
//   CC 74  - Cymbal level                                   //
//   CC 61  - Noise level                                    //
//   CC 102 - Snare level                                    //
//   Velocity mapped on release time                         //
//                                                           //
//                                             Pangrus 2020  //
///////////////////////////////////////////////////////////////


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>

// Kick
AudioSynthSimpleDrum     kick;

// Tom
AudioSynthSimpleDrum     tom;

// Noise
AudioSynthNoiseWhite     noise;
AudioFilterStateVariable noise_highpass;
AudioEffectEnvelope      noise_envelope;

// Cymbal
AudioSynthWaveform       square_oscillator_1;
AudioSynthWaveform       square_oscillator_2;
AudioSynthWaveform       square_oscillator_3;
AudioSynthWaveform       square_oscillator_4;
AudioSynthWaveform       square_oscillator_5;
AudioSynthWaveform       square_oscillator_6;
AudioMixer4              cymbal_mixer_1;
AudioMixer4              cymbal_mixer_2;
AudioMixer4              cymbal_mixer_3;
AudioFilterStateVariable cymbal_bandpass_1;
AudioFilterStateVariable cymbal_bandpass_2;
AudioFilterStateVariable cymbal_highpass;
AudioEffectEnvelope      cymbal_envelope;

// Snare
AudioSynthWaveform       triangle_oscillator_1;
AudioSynthWaveform       triangle_oscillator_2;
AudioSynthWaveform       sine_oscillator_1;
AudioSynthWaveform       sine_oscillator_2;
AudioSynthNoiseWhite     snare_noise;
AudioFilterStateVariable snare_lowpass;
AudioFilterStateVariable snare_bandpass;
AudioFilterStateVariable snare_highpass;
AudioEffectEnvelope      triangle_envelope;
AudioEffectEnvelope      sine_envelope;
AudioEffectEnvelope      snare_noise_envelope;
AudioMixer4              triangle_mixer;
AudioMixer4              sine_mixer;
AudioMixer4              snare_mixer;

// Main
AudioControlSGTL5000     sgtl5000_1;
AudioMixer4              sub_mixer;
AudioMixer4              output_mixer;
AudioOutputI2S           i2s1;

// Kick
AudioConnection          patch01 (kick, 0, sub_mixer, 0);

// Tom
AudioConnection          patch02(tom, 0, sub_mixer, 1);

// Snare
AudioConnection          patch03 (triangle_oscillator_1, 0, triangle_mixer, 0);
AudioConnection          patch04 (triangle_oscillator_2, 0, triangle_mixer, 1);
AudioConnection          patch05 (triangle_mixer, 0, triangle_envelope, 0);
AudioConnection          patch06 (sine_oscillator_1, 0, sine_mixer, 0);
AudioConnection          patch07 (sine_oscillator_2, 0, sine_mixer, 1);
AudioConnection          patch08 (sine_mixer, 0, sine_envelope, 0);
AudioConnection          patch09 (snare_noise, 0, snare_lowpass, 0);
AudioConnection          patch10 (snare_lowpass, 0, snare_bandpass, 0);
AudioConnection          patch11 (snare_bandpass, 1, snare_noise_envelope, 0);
AudioConnection          patch12 (triangle_envelope, 0, snare_mixer, 0);
AudioConnection          patch13 (sine_envelope, 0, snare_mixer, 1);
AudioConnection          patch14 (snare_noise_envelope, 0, snare_mixer, 2);
AudioConnection          patch15 (snare_mixer, 0, snare_highpass, 0);

// Cymbal
AudioConnection          patch16 (square_oscillator_1, 0, cymbal_mixer_1, 0);
AudioConnection          patch17 (square_oscillator_2, 0, cymbal_mixer_1, 1);
AudioConnection          patch18 (square_oscillator_3, 0, cymbal_mixer_1, 2);
AudioConnection          patch19 (square_oscillator_4, 0, cymbal_mixer_1, 3);
AudioConnection          patch20 (cymbal_mixer_1, 0, cymbal_mixer_2, 0);
AudioConnection          patch21 (square_oscillator_5, 0, cymbal_mixer_2, 1);
AudioConnection          patch22 (square_oscillator_6, 0, cymbal_mixer_2, 2);
AudioConnection          patch23 (cymbal_mixer_2, 0, cymbal_bandpass_1, 0);
AudioConnection          patch24 (cymbal_mixer_2, 0, cymbal_bandpass_2, 0);
AudioConnection          patch25 (cymbal_bandpass_1, 1, cymbal_mixer_3, 0);
AudioConnection          patch26 (cymbal_bandpass_2, 1, cymbal_mixer_3, 1);
AudioConnection          patch27 (cymbal_mixer_3, 0, cymbal_highpass, 0);
AudioConnection          patch28 (cymbal_highpass, 2, cymbal_envelope, 0);
AudioConnection          patch29 (cymbal_envelope, 0, sub_mixer, 3);
AudioConnection          patch30 (noise, 0, noise_highpass, 0);
AudioConnection          patch31 (noise_highpass, 2, noise_envelope, 0);
AudioConnection          patch32 (noise_envelope, 0, sub_mixer, 2);

// Main
AudioConnection          patch33 (sub_mixer, 0, output_mixer, 0);
AudioConnection          patch34 (snare_highpass, 2, output_mixer, 1);
AudioConnection          patch35 (output_mixer, 0, i2s1, 0);
AudioConnection          patch36 (output_mixer, 0, i2s1, 1);


// Midi note number to frequency conversion table
const float frequencyTable [120] = {
  //  DO       DO#       RE        RE#        MI       FA        FA#       SOL       SOL#      LA        LA#       SI
  //  C        C#        D         D#         E        F         F#        G         G#        A         A#        B
  0008.176, 0008.662, 0009.177, 0009.723, 0010.301, 0010.913, 0011.562, 0012.250, 0012.978, 0013.750, 0014.568, 0015.434, //0
  0016.352, 0017.324, 0018.354, 0019.445, 0020.602, 0021.827, 0023.125, 0024.500, 0025.957, 0027.500, 0029.135, 0030.868, //12
  0032.703, 0034.648, 0036.708, 0038.891, 0041.203, 0043.654, 0046.249, 0048.999, 0051.913, 0055.000, 0058.270, 0061.735, //24
  0065.406, 0069.296, 0073.416, 0077.782, 0082.407, 0087.307, 0092.499, 0097.999, 0103.826, 0110.000, 0116.541, 0123.471, //48
  0130.813, 0138.591, 0146.832, 0155.563, 0164.814, 0174.614, 0184.997, 0195.998, 0207.652, 0220.000, 0233.082, 0246.942, //60
  0261.626, 0277.183, 0293.665, 0311.127, 0329.628, 0349.228, 0369.994, 0391.995, 0415.305, 0440.000, 0466.164, 0493.883, //72
  0523.251, 0554.365, 0587.330, 0622.254, 0659.255, 0698.456, 0739.989, 0783.991, 0830.609, 0880.000, 0932.328, 0987.767, //84
  1046.502, 1108.731, 1174.659, 1244.508, 1318.510, 1396.913, 1479.978, 1567.982, 1661.219, 1760.000, 1864.655, 1975.533, //96
  2093.005, 2217.461, 2349.318, 2489.016, 2637.020, 2793.826, 2959.955, 3135.963, 3322.438, 3520.000, 3729.310, 3951.066, //108
  4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040.000, 7458.620, 7902.133, //120
};


const float divisionFactor = (1.0 / 127.0);
float noteFrequency;
float releaseLength;
float cutFrequency;
float valueCC;


void setup() {

  usbMIDI.setHandleNoteOn(manageNoteOn);
  usbMIDI.setHandleNoteOff(manageNoteOff);
  usbMIDI.setHandleControlChange(manageControlChange);

  AudioMemory(80);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.75);


  // Kick
  kick.pitchMod (0.60);

  // Tom
  tom.pitchMod  (0.55);

  // Noise
  noise.amplitude (0.4);
  noise_envelope.attack(0);
  noise_envelope.hold(0);
  noise_envelope.decay(0);
  noise_envelope.release(2);
  noise_highpass.frequency (15000);
  noise_highpass.resonance (0.9);

  // Cymbal
  // The frequencies of the six oscillators are
  // 205,3 - 304,4 - 369,6 - 522,7 - 540 - 800 Hz
  // accordingly to TR808 schematic
  square_oscillator_1.begin(0.7, 205.3, WAVEFORM_SQUARE);
  square_oscillator_2.begin(0.7, 304.4, WAVEFORM_SQUARE);
  square_oscillator_3.begin(0.7, 396.6, WAVEFORM_SQUARE);
  square_oscillator_4.begin(0.7, 522.7, WAVEFORM_SQUARE);
  square_oscillator_5.begin(0.7, 540.0, WAVEFORM_SQUARE);
  square_oscillator_6.begin(0.7, 800.0, WAVEFORM_SQUARE);

  cymbal_mixer_1.gain(0, 0.33);
  cymbal_mixer_1.gain(1, 0.33);
  cymbal_mixer_1.gain(2, 0.33);
  cymbal_mixer_1.gain(3, 0.33);
  cymbal_mixer_2.gain(0, 0.33);
  cymbal_mixer_2.gain(1, 0.33);
  cymbal_mixer_2.gain(2, 0.33);
  cymbal_mixer_2.gain(3, 0);
  cymbal_bandpass_1.frequency (3440);
  cymbal_bandpass_1.resonance (1);
  cymbal_bandpass_2.frequency (7100);
  cymbal_bandpass_2.resonance (1);
  cymbal_mixer_3.gain(0, 0.8);
  cymbal_mixer_3.gain(1, 1);
  cymbal_mixer_3.gain(2, 0);
  cymbal_mixer_3.gain(3, 0);
  cymbal_envelope.attack(0);
  cymbal_envelope.hold(0);
  cymbal_envelope.decay(0);
  cymbal_envelope.release(2);
  cymbal_highpass.frequency (10000);
  cymbal_highpass.resonance (1);

  // Snare
  // oscillator frequencies from TR909 schematic
  triangle_oscillator_1.begin(1, 175, WAVEFORM_TRIANGLE);
  triangle_oscillator_2.begin(1, 224, WAVEFORM_TRIANGLE);
  triangle_mixer.gain(0, 1);
  triangle_mixer.gain(1, 1);
  triangle_mixer.gain(2, 0);
  triangle_mixer.gain(3, 0);
  triangle_envelope.attack(0);
  triangle_envelope.hold(0);
  triangle_envelope.decay(1);
  triangle_envelope.release(4);
  sine_oscillator_1.begin(0.8, 180, WAVEFORM_SINE);
  sine_oscillator_2.begin(0.8, 330, WAVEFORM_SINE);
  sine_mixer.gain(0, 1.0);
  sine_mixer.gain(1, 1.0);
  sine_mixer.gain(2, 0.0);
  sine_mixer.gain(3, 0.0);
  sine_envelope.attack(0);
  sine_envelope.hold(0);
  sine_envelope.decay(1);
  sine_envelope.release(6);
  snare_noise.amplitude (1);
  snare_lowpass.frequency (8000);
  snare_lowpass.resonance (0.8);
  snare_bandpass.frequency (6000);
  snare_bandpass.resonance (0.9);
  snare_noise_envelope.attack(0);
  snare_noise_envelope.hold(0);
  snare_noise_envelope.decay(1);
  snare_noise_envelope.release(8);
  snare_mixer.gain(0, 0.8);     // Triangle
  snare_mixer.gain(1, 0.7);     // Sine
  snare_mixer.gain(2, 1.0);     // Noise
  snare_mixer.gain(3, 0.0);
  snare_highpass.frequency (2000);
  snare_highpass.resonance (0.9);

  // Main
  sub_mixer.gain(0, 1.0);     // Kick
  sub_mixer.gain(1, 1.0);     // Tom
  sub_mixer.gain(2, 0.4);     // Noise
  sub_mixer.gain(3, 0.5);     // Cymbal
  output_mixer.gain(0, 1.0);  // Sub mix
  output_mixer.gain(1, 0.8);  // Snare
  output_mixer.gain(2, 0.0);
  output_mixer.gain(3, 0.0);

  // debug
  // Serial.begin (9600);
}


void loop() {
  usbMIDI.read();
}

void manageNoteOn(byte channel, byte note, byte velocity) {
  if ( note > 11 && note < 108 ) {
    noteFrequency = frequencyTable [note];
    cutFrequency = note * divisionFactor * 18000;
    releaseLength = (velocity - 6) * 6;
    playDrum(channel, noteFrequency, releaseLength);

    /* debug
      Serial.print ("note : ");
      Serial.println (note);
      Serial.print ("noteFrequency : ");
      Serial.println (noteFrequency);
      Serial.print ("cutFrequency : ");
      Serial.println (cutFrequency);
      Serial.print ("releaseLength : ");
      Serial.println (releaseLength);
    */

  }
}


void playDrum (byte channel, byte noteFrequency, int release) {
  switch (channel) {

    //Channel 6 - Kick
    case 6:
      kick.frequency(noteFrequency);
      kick.length(releaseLength);
      kick.noteOn();
      break;

    //Channel 7 - Tom
    case 7:
      tom.frequency(noteFrequency);
      tom.length(releaseLength);
      tom.noteOn();
      break;

    //Channel 8 - Noise
    case 8:
      noise_highpass.frequency (cutFrequency);
      noise_envelope.release(releaseLength);
      noise_envelope.noteOn();
      break;

    //Channel 9 - Cymbal
    case 9:
      cymbal_highpass.frequency (cutFrequency);
      cymbal_envelope.release(releaseLength);
      cymbal_envelope.noteOn();
      break;

    //Channel 10 - Snare
    case 10:
      snare_highpass.frequency (cutFrequency / 4);
      triangle_envelope.release(releaseLength / 3);
      sine_envelope.release(releaseLength / 3);
      snare_noise_envelope.release(releaseLength / 2);
      triangle_envelope.noteOn();
      sine_envelope.noteOn();
      snare_noise_envelope.noteOn();
      break;
  }
}


void manageNoteOff(byte channel, byte note, byte velocity) {
  switch (channel) {
    //Channel 8 - Noise
    case 8:
      noise_envelope.noteOff();
      break;
    //Channel 9 - Cymbal
    case 9:
      cymbal_envelope.noteOff();
      break;

    //Channel 10 - Snare
    case 10:
      triangle_envelope.noteOff();
      sine_envelope.noteOff();
      snare_noise_envelope.noteOff();
      break;
  }
}

void manageControlChange(byte channel, byte control, byte value) {
  valueCC = value * divisionFactor;
  switch (control) {

    // Main output level
    case 0:
      output_mixer.gain(0, 1.0 * valueCC);      // Sub mix
      output_mixer.gain(1, 0.8 * valueCC);      // Snare
      output_mixer.gain(2, 0);
      output_mixer.gain(3, 0);
      break;

    // Kick level  - CC16
    case 16:
      sub_mixer.gain(0, valueCC);
      break;

    // Kick pitch modulation  - CC25
    case 25:
      kick.pitchMod (valueCC);
      break;

    // Tom level  - CC18
    case 18:
      sub_mixer.gain(1, valueCC);
      break;

    // Tom pitch modulation  - CC48
    case 48:
      tom.pitchMod (valueCC);
      break;

    // Noise level  - CC61
    case 61:
      sub_mixer.gain(2, valueCC);
      break;

    // Cymbal level  - CC74
    case 74:
      sub_mixer.gain(3, valueCC);
      break;

    // Snare level
    case 102:
      snare_mixer.gain(0, 0.8 * valueCC);     // Triangle
      snare_mixer.gain(1, 0.7 * valueCC);     // Sine
      snare_mixer.gain(2, 1.0 * valueCC);     // Noise
      break;
  }
}
