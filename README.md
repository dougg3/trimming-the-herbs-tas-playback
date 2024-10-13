# Trimming the Herbs TAS Playback

This repository contains a few Trimming the Herbs TAS playback projects I've worked on:

- [MicroPython program that presses buttons on a Wii Classic Controller by soldering wires between a Raspberry Pi Pico and the controller](/classic_controller)
  - This is unreliable for playback but does eventually succeed.
- [C firmware for the Raspberry Pi Pico that pretends to be a USB controller and is capable of playing back Trimming the Herbs perfectly.](/usb)
  - It's only perfect if you wire up VSYNC from an HDMI decoder chip. Otherwise it uses a timer and does fail from time to time due to desync.
