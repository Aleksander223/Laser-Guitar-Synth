# Laser Guitar Synth
Laser Guitar Synth

## Video
https://www.youtube.com/watch?v=Mv7VVykAYJc

## Credits
Thanks to Luana Pantiru for designing the project and helping me assemble it, Sensorium for the Mozzi Audio Library, Stack Overflow for fixing common bugs, and the teaching assistants for guiding me through.

## Intro
I always have liked musical instruments, particulary the guitar. Recently I have got myself into electronic music and how it works, and I have discovered sound synthesizers. Since that very moment, I knew that I wanted to make one, but making one with Arduino wouldn't prove an easy task...

## How it works
Mozzi is a sound synthesis library for Arduino. Normally, an Arduino can only output square waves using PWM, thus resulting in boring sounds. Mozzi completely overhauls the sounds that an Arduino can produce. You can output basically anything from sine waves, sawtooth waves, a combination of those and even samples. The library is powerful indeed, although it is too powerful for the capabilities of an Arduino.

Thus, my synth (Guitarduino) is monophonic (meaning only one sound can be played at the same time). Handling polyphonic sounds with Arduino is doable but proves quite difficult since it does not possess a lot of processing power. 

The audio is processed through different filters (currently just ADSR enveloping and gain change, want to add a LFO later) and is output on PWM Pin 11 on Arduino Mega. The sound is then filtered through both a High Pass Filter and a Low Pass Filter, to remove all of the unwanted noise from the circuit. Then the signal is sent off to an audio jack, which you can use to output sound to any device with an audio cable (such as speakers or headphones).

The user can control the various parameters of the audio using the handy LCD Display and Joystick.

The strings are laser diodes, which are pointed towards photoresistors. If the user touches the laser (thus blocking it from going to the photoresistor), its internal resistance changes, thus producing a perceptible change which the Arduino treats as a string pluck. If the laser makes its way back to the photoresistor, we know that the string has been released.

## How does one play the Guitarduino
Like a real guitar, you have to pluck the strings! There are 4 "strings" (which are lasers), and touching one with your finger, activates the associated note. The pitch is controlled with the position of your hand on the neck of the guitar. The farther away it is from the body of the guitar, the lower pitch it will be. The notes are mapped in major chords (meaning that the 4 strings are actually in a Major Arpeggio, allowing for easier playing), the root of which is controlled with the position of your hand on the guitar neck.

## Materials
- 3.5 mm Audio Jack
- Arduino Mega
- LCD Display
- Joystick
- 4 5V Laser Diodes
- 4 Photoresistors
- HC-SR04 Distance Sensor or similar
- Cables (lots of them)
- Breadboard/prototyping board
- PAM8403 Amp or similar
- LPF Circuit
- HPF Circuit
- Speaker/headphones (to test your audio output)
- Screws (mostly M3 or M2)
- 4 10k Resistors (for photoresistors)
- 2 100uF Capacitors (for noise filtering on the power rail)

HPF Circuit (high pass filter of RC type, will filter frequencies below 225Hz):
- 1 6.8k Resistor
- 1 104nF Capacitor

LPF Circuit (low pass filter of RC type, will filter frequencies above 7650Hz, also called a notch filter):
- 1 200 Resistor
- 1 104nF Capacitor

If you don't have these components on hand, you can calculate something similar for the noise filters with http://sim.okawa-denshi.jp/en/CRtool.php

## Schematics

High pass filter and low pass filter
![HPF + LPF](https://i.imgur.com/6TGlQsl.png)

## Final assembly
![Final Assembly](https://i.imgur.com/HHGaI7z.jpg)
![Final Assembly](https://i.imgur.com/yYZOuHs.jpg)
![Final Assembly](https://i.imgur.com/Xo2G1gx.jpg)
![Final Assembly](https://i.imgur.com/KCVDQjW.jpg)

