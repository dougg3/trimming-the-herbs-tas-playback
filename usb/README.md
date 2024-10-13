# Trimming the Herbs playback through USB

This uses a Raspberry Pi Pico to demonstrate the ability to play back [Trimming the Herbs](https://www.youtube.com/watch?v=siz4oB4CGKo) in Super Mario Maker on the Wii U by pretending to be a USB gamepad. The Wii U does not natively support USB gamepads, so you have to use [HID to VPAD](https://github.com/Maschell/hid_to_vpad) in order to enable USB gamepad support first.

This requires using some kind of exploit to be able to run homebrew apps, so you have to use an exploit of your choice to get [Tiramisu](https://tiramisu.foryour.cafe/) running first. Note: You cannot use Aroma because it's too new for HID to VPAD.

**Don't attempt this unless you are comfortable with homebrew stuff on the Wii U. You could probably brick your Wii U if you do something incorrectly.**

## Hardware requirements

- Raspberry Pi Pico
- (Optional, but highly recommended): an HDMI decoder capable of providing a 3.3V VSYNC signal to the Raspberry Pi Pico on GP2, e.g.: [TFP401 breakout board](https://www.adafruit.com/product/2218).
  - Wire up the VSYNC output and ground to the Pi Pico's GP2 pin.
- USB to serial adapter connected to the Raspberry Pi Pico's GP0 and GP1 pins to provide console access. I use the [Raspberry Pi Debug Probe](https://www.raspberrypi.com/products/debug-probe/).
- If you are using the VSYNC signal, you will need an HDMI splitter if you also want to see the Wii U's HDMI output on a TV or monitor.
- A FAT32-formatted SD card to put in the Wii U

## Build instructions

Install any prerequisites as specified by the "Manually Configure your Environment" section of the [Getting Started with Pico guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf). Then build this firmware:

```
git clone --recursive https://github.com/raspberrypi/pico-sdk.git
git clone https://github.com/dougg3/trimming-the-herbs-tas-playback.git
cd trimming-the-herbs-tas-playback/usb
mkdir build
cd build
export PICO_SDK_PATH=/path/to/your/checkout/of/pico-sdk
cmake ..
make -j $(nproc)
```

(Obviously, replace the path in the export command with the actual path where you cloned it to)

When this finishes, you should have a file usb_hid_sim.uf2 which you can program to the Pi Pico using the normal method of holding the BOOTSEL button while plugging it into your computer and dragging the file over.

## SD card prep

- Start with a FAT32-formatted SD card.
- Download [Tiramisu](https://tiramisu.foryour.cafe/) and copy the contents of the zip file to the root level of the SD card.
- Download [HIDtoVPAD v0.9k](https://github.com/Maschell/hid_to_vpad/releases/tag/v0.9k) and copy the contents of the zip file to the root level of the SD card.
- Copy DougPlayback.ini from this repository into the wiiu/controller directory on the SD card. There should already be a bunch of other .ini files in the directory.
- If using UDPIH for the exploit:
  - Copy recovery_menu from [LoadRPX UDPIH Payload](https://hacksguidewiki.sfo3.digitaloceanspaces.com/hacksguidewiki/Recovery_menu.zip) to the root of the SD card
  - **NOTE: Do not use the recovery_menu file from the UDPIH repository. We need the LoadRPX payload instead.**
  - Make a copy of wiiu/environments/tiramisu/root.rpx on the SD card, move the copy to the root of the SD card, and rename it to launch.rpx
- If using bluubomb:
  - Copy loadrpx.bin from [sd_kernels.zip](https://github.com/GaryOderNichts/bluubomb/releases/tag/v5) to the root of the SD card and rename it to bluu_kern.bin.
  - Make a copy of wiiu/environments/tiramisu/root.rpx on the SD card, move the copy to the root of the SD card, and rename it to launch.rpx
- Safely eject the SD card. You're all done.

## Usage instructions

- Use an exploit such as [wiiuexploit.xyz](http://wiiuexploit.xyz), [UDPIH](https://github.com/GaryOderNichts/udpih), or [bluubomb](https://github.com/GaryOderNichts/bluubomb) to run Tiramisu. You do not have to install Tiramisu permanently; you just need it so that you can run HIDtoVPAD.
  - If you use UDPIH or bluubomb, after running the exploit just launch any Wii U app such as Health and Safety Information. Environment Loader should pop up and allow you to pick tiramisu. This is a temporary one-time thing that UDPIH or bluubomb does.
- If using UDPIH with your Pi Pico for the exploit:
  - After you have successfully gotten Tiramisu running you can reflash the Pico with this firmware for the gamepad emulation instead. In the future it should be possible to consolidate UDPIH into this same firmware so you don't need to reflash it, but for now this is how you can use both.
- When loading Tiramisu, if you get a yellow warning screen, just press the A button to continue without blocking. We won't be installing anything permanently.
- With usb_hid_sim.uf2 programmed to the Pi Pico, plug it into the Wii U.
- In the Boot Selector menu, go to Homebrew Launcher and launch the HID to VPAD app. Press the Load button to launch it.
- Select Wii U Pro Controller #1, and then press the A button on the Wii U Gamepad to say you want to connect a controller.
- In the Pi Pico's UART console, type a lowercase letter 'a'. This should virtually press the A button on the USB controller.
- Then type an uppercase letter 'A' to release the USB controller's A button.
- The screen should show that Pro Controller 1 is now paired to VID: 0xCAFE, PID: 0xBABE.
- Press the + button on the Wii U Gamepad to apply patches. This will also exit you back to the main Wii U menu.
- Now load Super Mario Maker and load up Trimming the Herbs for playing.
- **Note: If you don't have a VSYNC signal hooked up to GP2 on the Pi Pico, type a lowercase 'v' in the Pi Pico's UART console to disable vsync and switch to the timer instead. You can press it again to re-enable it you want to test both methods.** Keep in mind that if you use the timer, it won't always beat the level successfully; it will work most of the time though.
- When Trimming the Herbs begins, type a lowercase 'q' in the Pi Pico's UART console. This should cause the game to pause, which will demonstrate that the TAS playback engine is working. If it doesn't pause, check and make sure you have VSYNC hooked up properly if you're using VSYNC.
- Press the spacebar on the UART console to begin playing the TAS.
- If anything gets out of sync, type a lowercase 'q' to cancel the playback and pause to prepare for trying again.

## Special thanks

- [Maschell](https://github.com/Maschell) for creating Tiramisu and HIDtoVPAD which enabled this to work
- [GaryOderNichts](https://github.com/GaryOderNichts) for creating some incredible exploits which also enabled this to work
