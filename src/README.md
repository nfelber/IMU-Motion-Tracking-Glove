# Software conception

This document explains our software design.

## Dataflow

```
 _______  _______       _______                              _______________
|       ||       |     |       |                            |               |
|       ||       |     |       |                            |    Computer   |
|  MPU  ||  MPU  | ... |  NXP  |                            |               |
|       ||       |     |       |                            |_______________|
|_______||_______|     |_______|                            |...............|
    |        |      :      |                                |:::::::::::::::|
    |--------'------'------'                                |_____|___|_____|
    | I2C raw data                                                  |
 ___|_________                                                      |
|             |                                                     |
|             |                         _____________________       | BLE
|             |                        |                     |      |
|             |                        |                     |      |
|             |     ESP-NOW (wifi)     |                     |      |
|   ESP8266   | ---------------------> |      ESP32-CAM      | -----'
|             |      orientations      |                     |  hand skeleton
|             |                        |   pose-estimation   |
|     AHRS    |                        |_____________________|
|  algorithm  |
|_____________|

```

# ESP8266

The Arduino sketch for the esp8266 can be found inside the `esp8266` folder.
The program uses the [I2cdev](https://github.com/jrowberg/i2cdevlib) library to program the MPU-6050 and communicate with them using the I2C protocol.
It controls the NXP 9-DOF sensor similarily using the [Adafruit AHRS](https://github.com/adafruit/Adafruit_AHRS) library.

The program starts by initialising each sensor.
Then it programs the onboard digital motion processor found on each MPU-6050 so that all the sensor fusion is done on the sensor itself.
Finally, it reads and computes the orientation of each sensor as fast as possible in a 40 ms loop (25 Hz).
The orientation of the NXP sensor is given relative to the absolute (if I'm allowed this misuse of language) reference frame given by the gravity vector and the vector heading north.
The orientation of the MPUs are calculated relative to the orientation of the NXP, and their yaw angle is regularly updated using the NXP as a reference as they tend to drift with time otherwise.

At the end of each loop, these six orientations are then sent to the ESP32-CAM in the form of euler angles using the ESP-NOW wifi protocol.

# ESP32-CAM

The Arduino sketch for the esp8266 can be found inside the `esp32-ble` folder.
This program runs the pose estimation model whose goal is to reconstitute a skeleton of the hand using the orientation data.
The skeleton is composed of 20 joints: 4 for each finger except the thumb which only has 3 and one joint for the wrist.

All the joints have a position in cartesian space relative to the wrist which has position `(0, 0, 0)`.
This way, all the points can easily be translated if some information about the positioning of the hand would be found.

After each update, the program sends the data via bluetooth to the computer on which a running driver can retreive them.

# Rust driver

A simple driver written in [Rust](https://www.rust-lang.org/) can be found in the `MTG-driver` folder.
The crate is very straightforward to use and can provide all the glove's data as well as some very basic gestures detection.

# Demo software

## Debugger

The debugger, found in the `MTG-driver` folder is a simple plotter which displays the hand skeleton as well as all the received data on the screen.
To run it:

```console
$ cargo run --example debugger
```

## Music sequencer

A virtual music sequencer demo is provided in the `midi-sequencer` folder.
For it to work, the virtual loopback MIDI software [LoopMidi](https://www.tobias-erichsen.de/software/loopmidi.html) must be installed and running.
Then, run the program with:

```console
$ cargo run
```

and select the LoopMIDI port.
Then, the virtual MIDI port can be used with any [DAW](https://en.wikipedia.org/wiki/Digital_audio_workstation).

Four virtual knobs control the four notes of the sequencer loop.
Each of them can be grabbed by pinching in front of you with the thumb and a different finger.
There are four additionnal knobs that can be grabbed the same way, but pinching towards the floor.
These additional knobs control the frequency, the cutoff, the base height of every note and the scale that is used.

## Flappy bird

...