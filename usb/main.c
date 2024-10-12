/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2024 Doug Brown (downtowndougbrown.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "tusb.h"
#include "pico/unique_id.h"
#include "bsp/board.h"
#include "hardware/gpio.h"

#define REPORT_ID_GAMEPAD	1
#define USB_VID			0xCafe
#define USB_PID			0xBabe
#define USB_BCD			0x0200
#define VSYNC_GPIO		2

#define BTN_DOWN		(1UL << 28)
#define BTN_UP			(1UL << 29)
#define BTN_LEFT		(1UL << 30)
#define BTN_RIGHT		(1UL << 31)

#define CONFIG_TOTAL_LEN	(TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_HID		0x81

enum
{
	ITF_NUM_HID,
	ITF_NUM_TOTAL
};

struct action_struct;

static void hid_task(void);
static void send_hid_report(void);
static void press_button(uint32_t button);
static void release_button(uint32_t button);
static void update_hat(void);
static void start_actions(struct action_struct const *act, uint32_t count);
static void stop_actions(void);
static bool do_frame(void);
static bool timer_callback(repeating_timer_t *rt);
static void vsync_callback(uint gpio, uint32_t event_mask);

static bool use_vsync = true;
static hid_gamepad_report_t report =
{
	.x = 0,
	.y = 0,
	.z = 0,
	.rz = 0,
	.rx = 0,
	.ry = 0,
	.hat = 0,
	.buttons = 0
};
static repeating_timer_t frame_timer;

static uint32_t dpad_state;

typedef enum playback_action
{
	PRESS = 0,
	RELEASE,
	DELAY
} playback_action;

typedef struct action_struct
{
	playback_action action;
	uint32_t param;
} action_struct;

/// The actual TAS, starting with a press of the Start Over button.
/// Inputs courtesy of CSV by Ahoyo
static action_struct tth_tas_actions[] = {
    // Menuing to start over
    {RELEASE, GAMEPAD_BUTTON_A},
    {RELEASE, BTN_DOWN},
    {DELAY, 4},
    {PRESS, BTN_DOWN},
    {DELAY, 4},
    {RELEASE, BTN_DOWN},
    {DELAY, 3},
    {PRESS, GAMEPAD_BUTTON_A},
    {DELAY, 3},
    {RELEASE, GAMEPAD_BUTTON_A},
    // Start of actual TAS
    // Delay for actual run
    {DELAY, 213}, // 218 also works
    {PRESS, GAMEPAD_BUTTON_Y},
    {PRESS, GAMEPAD_BUTTON_TL},
    {DELAY, 16},
    {PRESS, BTN_RIGHT},
    {DELAY, 7},
    {PRESS, BTN_DOWN},
    {DELAY, 19},
    {RELEASE, BTN_RIGHT},
    {DELAY, 19},
    {PRESS, BTN_LEFT},
    {DELAY, 16},
    {RELEASE, BTN_LEFT},
    {RELEASE, GAMEPAD_BUTTON_TL},
    {DELAY, 3},
    {PRESS, BTN_RIGHT},
    {DELAY, 2},
    {RELEASE, BTN_RIGHT},
    {DELAY, 4},
    {RELEASE, GAMEPAD_BUTTON_Y},
    {DELAY, 4},
    {PRESS, GAMEPAD_BUTTON_Y},
    {DELAY, 4},
    {PRESS, BTN_RIGHT},
    {DELAY, 21},
    {PRESS, GAMEPAD_BUTTON_A},
    {DELAY, 13},
    {RELEASE, BTN_RIGHT},
    {DELAY, 2},
    {PRESS, BTN_LEFT},
    {DELAY, 11},
    {RELEASE, GAMEPAD_BUTTON_A},
    {DELAY, 9},
    {RELEASE, BTN_LEFT},
    {DELAY, 1},
    {PRESS, BTN_RIGHT},
    {DELAY, 1},
    {RELEASE, BTN_RIGHT},
    {DELAY, 2},
    {RELEASE, GAMEPAD_BUTTON_Y},
    {DELAY, 2},
    {PRESS, GAMEPAD_BUTTON_Y},
    {DELAY, 11},
    {PRESS, BTN_RIGHT},
    {DELAY, 16},
    {PRESS, GAMEPAD_BUTTON_A},
    {DELAY, 14},
    {RELEASE, BTN_RIGHT},
    {DELAY, 1},
    {PRESS, BTN_LEFT},
    {DELAY, 7},
    {RELEASE, GAMEPAD_BUTTON_A},
    {DELAY, 4},
    {RELEASE, BTN_LEFT},
    {DELAY, 1},
    {PRESS, BTN_RIGHT},
    {DELAY, 1},
    {RELEASE, BTN_RIGHT},
    {DELAY, 2},
    {RELEASE, GAMEPAD_BUTTON_Y},
    {DELAY, 4},
    {PRESS, BTN_LEFT},
    {DELAY, 5},
    {RELEASE, BTN_LEFT},
    {DELAY, 5},
    {PRESS, BTN_RIGHT},
    {DELAY, 8},
    {PRESS, GAMEPAD_BUTTON_Y},
    {DELAY, 8},
    {PRESS, GAMEPAD_BUTTON_A},
    {DELAY, 7},
    {RELEASE, BTN_RIGHT},
    {DELAY, 29},
    {PRESS, BTN_LEFT},
    {DELAY, 17},
    {RELEASE, BTN_LEFT},
    {DELAY, 7},
    {PRESS, BTN_LEFT},
    {DELAY, 8},
    {RELEASE, BTN_LEFT},
    {DELAY, 37},
    {PRESS, BTN_RIGHT},
    {DELAY, 15},
    {RELEASE, BTN_RIGHT},
    {DELAY, 4},
    {RELEASE, GAMEPAD_BUTTON_A},
    {DELAY, 4},
    {PRESS, GAMEPAD_BUTTON_A},
    {DELAY, 23},
    {RELEASE, GAMEPAD_BUTTON_Y},
    {DELAY, 5},
    {PRESS, GAMEPAD_BUTTON_Y},
    {DELAY, 19},
    {PRESS, BTN_LEFT},
    {DELAY, 6},
    {RELEASE, BTN_LEFT},
    {DELAY, 5},
    {PRESS, BTN_LEFT},
    {DELAY, 68},
    {RELEASE, BTN_LEFT},
    {DELAY, 2},
    {PRESS, BTN_RIGHT},
    {DELAY, 30},
    {RELEASE, GAMEPAD_BUTTON_A},
    {DELAY, 15},
    {PRESS, GAMEPAD_BUTTON_A},
    {RELEASE, BTN_RIGHT},
    {DELAY, 9},
    {PRESS, BTN_LEFT},
    {DELAY, 11},
    {RELEASE, BTN_LEFT},
    {DELAY, 3},
    {RELEASE, GAMEPAD_BUTTON_A},
    {DELAY, 4},
    {RELEASE, GAMEPAD_BUTTON_Y},
    {DELAY, 2},
    {PRESS, GAMEPAD_BUTTON_A},
    {DELAY, 2},
    {RELEASE, GAMEPAD_BUTTON_A},
    {DELAY, 7},
    {PRESS, BTN_LEFT},
    {DELAY, 31},
    {PRESS, GAMEPAD_BUTTON_A},
    {DELAY, 3},
    {RELEASE, GAMEPAD_BUTTON_A},
    {PRESS, GAMEPAD_BUTTON_Y},
    {DELAY, 3},
    {PRESS, GAMEPAD_BUTTON_A},
    {DELAY, 4},
    {RELEASE, BTN_DOWN},
    {DELAY, 14},
    {RELEASE, BTN_LEFT},
    {DELAY, 2},
    {RELEASE, GAMEPAD_BUTTON_Y},
    {PRESS, BTN_RIGHT},
    {DELAY, 19},
    {RELEASE, BTN_RIGHT},
    {DELAY, 5},
    {RELEASE, GAMEPAD_BUTTON_A}
};

/// Actions used to pause the game when we complete or if we need to stop it early because it failed
static const action_struct pause_actions[] = {
    {DELAY, 5},
    {PRESS, GAMEPAD_BUTTON_START},
    {DELAY, 5},
    {RELEASE, GAMEPAD_BUTTON_START},
};

static volatile action_struct const *actions;
static volatile uint32_t action_index;
static volatile uint32_t action_count;
static volatile uint32_t cur_param;

static tusb_desc_device_t const desc_device =
{
	.bLength = sizeof(tusb_desc_device_t),
	.bDescriptorType = TUSB_DESC_DEVICE,
	.bcdUSB = USB_BCD,
	.bDeviceClass = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

	.idVendor = USB_VID,
	.idProduct = USB_PID,
	.bcdDevice = 0x0100,

	.iManufacturer = 0x01,
	.iProduct = 0x02,
	.iSerialNumber = 0x03,

	.bNumConfigurations = 0x01,
};

/// Device descriptor callback
uint8_t const *tud_descriptor_device_cb(void)
{
	return (uint8_t const *)&desc_device;
}

uint8_t const desc_hid_report[] =
{
	TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(REPORT_ID_GAMEPAD))
};

/// HID report descriptor callback
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
	(void)instance;
	return desc_hid_report;
}


uint8_t const desc_configuration[] =
{
	TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),

	TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1)
};

/// Config descriptor callback
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
	(void)index;
	return desc_configuration;
}

static char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

static char const *string_desc_arr[] =
{
	(const char[]){0x09, 0x04}, // English
	"Doug Brown",
	"Wii U Controller",
	serial,
};

static uint16_t _desc_str[32];

/// USB string descriptor callback
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	(void)langid;

	uint8_t chr_count;

	if (index == 0)
	{
		memcpy(&_desc_str[1], string_desc_arr[0], 2);
		chr_count = 1;
	}
	else
	{
		if (index == 3)
		{
			pico_get_unique_board_id_string(serial, sizeof(serial));
		}

		if (index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))
		{
			return NULL;
		}

		const char *str = string_desc_arr[index];
		chr_count = strlen(str);
		if (chr_count > 31)
		{
			chr_count = 31;
		}

		for (uint8_t i = 0; i < chr_count; i++)
		{
			_desc_str[i + 1] = str[i];
		}
	}

	_desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

	return _desc_str;
}

/// Main function
int main(void)
{
	board_init();
	tusb_init();
	printf("Wii U USB Trimming the Herbs TAS playback\r\n");
	printf("By Doug Brown\r\n");
	printf("    https://www.downtowndougbrown.com/\r\n");
	printf("Made possible by hid_to_vpad by Maschell\r\n");
	printf("    https://github.com/Maschell/hid_to_vpad\r\n");
	printf("VSYNC %s\r\n", use_vsync ? "ON" : "OFF");

	while (1)
	{
		// Handle button presses from the UART
		int result = board_getchar();
		if (result >= 0)
		{
			switch (result)
			{
				case 'a':
					printf("Press A\r\n");
					report.buttons |= GAMEPAD_BUTTON_A;
					break;
				case 'b':
					printf("Press B\r\n");
					report.buttons |= GAMEPAD_BUTTON_B;
					break;
				case 'x':
					printf("Press X\r\n");
					report.buttons |= GAMEPAD_BUTTON_X;
					break;
				case 'y':
					printf("Press Y\r\n");
					report.buttons |= GAMEPAD_BUTTON_Y;
					break;
				case 'l':
					printf("Press L\r\n");
					report.buttons |= GAMEPAD_BUTTON_TL;
					break;
				case 'r':
					printf("Press R\r\n");
					report.buttons |= GAMEPAD_BUTTON_TR;
					break;
				case '[':
					printf("Press ZL\r\n");
					report.buttons |= GAMEPAD_BUTTON_TL2;
					break;
				case ']':
					printf("Press ZR\r\n");
					report.buttons |= GAMEPAD_BUTTON_TR2;
					break;
				case ',':
					printf("Press -\r\n");
					report.buttons |= GAMEPAD_BUTTON_SELECT;
					break;
				case '.':
					printf("Press +\r\n");
					report.buttons |= GAMEPAD_BUTTON_START;
					break;
				case 'A':
					printf("Release A\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_A;
					break;
				case 'B':
					printf("Release B\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_B;
					break;
				case 'X':
					printf("Release X\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_X;
					break;
				case 'Y':
					printf("Release Y\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_Y;
					break;
				case 'L':
					printf("Release L\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_TL;
					break;
				case 'R':
					printf("Release R\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_TR;
					break;
				case '{':
					printf("Release ZL\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_TL2;
					break;
				case '}':
					printf("Release ZR\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_TR2;
					break;
				case '<':
					printf("Release -\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_SELECT;
					break;
				case '>':
					printf("Release +\r\n");
					report.buttons &= ~GAMEPAD_BUTTON_START;
					break;
				case '4':
					printf("DPAD Left\r\n");
					report.hat = GAMEPAD_HAT_LEFT;
					break;
				case '8':
					printf("DPAD Up\r\n");
					report.hat = GAMEPAD_HAT_UP;
					break;
				case '6':
					printf("DPAD Right\r\n");
					report.hat = GAMEPAD_HAT_RIGHT;
					break;
				case '2':
					printf("DPAD Down\r\n");
					report.hat = GAMEPAD_HAT_DOWN;
					break;
				case '7':
					printf("DPAD Up+Left\r\n");
					report.hat = GAMEPAD_HAT_UP_LEFT;
					break;
				case '9':
					printf("DPAD Up+Right\r\n");
					report.hat = GAMEPAD_HAT_UP_RIGHT;
					break;
				case '1':
					printf("DPAD Down+Left\r\n");
					report.hat = GAMEPAD_HAT_DOWN_LEFT;
					break;
				case '3':
					printf("DPAD Down+Right\r\n");
					report.hat = GAMEPAD_HAT_DOWN_RIGHT;
					break;
				case '5':
					printf("Release DPAD\r\n");
					report.hat = GAMEPAD_HAT_CENTERED;
					break;
				case ' ':
					printf("Play TTH TAS\r\n");
					start_actions(tth_tas_actions, sizeof(tth_tas_actions)/sizeof(tth_tas_actions[0]));
					break;
				case 'q':
					printf("Stop and pause\r\n");
					stop_actions();
					start_actions(pause_actions, sizeof(pause_actions)/sizeof(pause_actions[0]));
					break;
				// These are for adjusting the start delay in case it differs for whatever reason
				case 'w':
					tth_tas_actions[10].param++;
					printf("TTH Start Delay %u\r\n", tth_tas_actions[10].param);
					break;
				case 'e':
					tth_tas_actions[10].param--;
					printf("TTH Start Delay %u\r\n", tth_tas_actions[10].param);
					break;
				// Use this to toggle the use of vsync on and off. With vsync off, a timer is used instead
				// (which is less reliable and will occasionally fail to clear Trimming the Herbs)
				case 'v':
					use_vsync = !use_vsync;
					printf("VSYNC %s\r\n", use_vsync ? "ON" : "OFF");
					break;
			}
		}

		tud_task();
		hid_task();
	}
}

/// Periodic task that decides when we should send out a report
static void hid_task(void)
{
	const uint32_t interval_ms = 1;
	static uint32_t start_ms = 0;

	if (board_millis() - start_ms < interval_ms) { return; }
	start_ms += interval_ms;

	send_hid_report();
}

/// Sends a new HID report out to the host
static void send_hid_report(void)
{
	if (!tud_hid_ready()) { return; }

	tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
}

/// Presses a button
static void press_button(uint32_t button)
{
	// The D-Pad is handled differently because it's turned into a hat value
	if (button >= BTN_DOWN)
	{
		dpad_state |= button;
		update_hat();
	}
	else
	{
		report.buttons |= button;
	}
}

/// Releases a button
static void release_button(uint32_t button)
{
	// The D-Pad is handled differently because it's turned into a hat value
	if (button >= BTN_DOWN)
	{
		dpad_state &= ~button;
		update_hat();
	}
	else
	{
		report.buttons &= ~button;
	}
}

/// Converts the four D-Pad bits into a hat value
static void update_hat(void)
{
	if (dpad_state & BTN_UP)
	{
		if (dpad_state & BTN_RIGHT)
		{
			report.hat = GAMEPAD_HAT_UP_RIGHT;
		}
		else if (dpad_state & BTN_LEFT)
		{
			report.hat = GAMEPAD_HAT_UP_LEFT;
		}
		else
		{
			report.hat = GAMEPAD_HAT_UP;
		}
	}
	else if (dpad_state & BTN_RIGHT)
	{
		if (dpad_state & BTN_DOWN)
		{
			report.hat = GAMEPAD_HAT_DOWN_RIGHT;
		}
		else
		{
			report.hat = GAMEPAD_HAT_RIGHT;
		}
	}
	else if (dpad_state & BTN_DOWN)
	{
		if (dpad_state & BTN_LEFT)
		{
			report.hat = GAMEPAD_HAT_DOWN_LEFT;
		}
		else
		{
			report.hat = GAMEPAD_HAT_DOWN;
		}
	}
	else if (dpad_state & BTN_LEFT)
	{
		report.hat = GAMEPAD_HAT_LEFT;
	}
	else
	{
		report.hat = GAMEPAD_HAT_CENTERED;
	}
}

/// Begins playing a list of actions
static void start_actions(action_struct const *act, uint32_t count)
{
	cancel_repeating_timer(&frame_timer);
	gpio_set_irq_enabled(VSYNC_GPIO, GPIO_IRQ_EDGE_RISE, false);
	report.buttons = 0;
	report.hat = 0;
	dpad_state = 0;
	action_index = 0;
	actions = act;
	cur_param = actions[0].param;
	action_count = count;
	if (!use_vsync)
	{
		add_repeating_timer_us(-16683, timer_callback, NULL, &frame_timer);
	}
	else
	{
		gpio_set_irq_enabled_with_callback(VSYNC_GPIO, GPIO_IRQ_EDGE_RISE, true, vsync_callback);
	}
}

/// Stops any active playback
static void stop_actions(void)
{
	cancel_repeating_timer(&frame_timer);
	gpio_set_irq_enabled(VSYNC_GPIO, GPIO_IRQ_EDGE_RISE, false);
	action_index = 0;
	action_count = 0;
	report.buttons = 0;
	report.hat = 0;
	dpad_state = 0;
}

/// Should be called once per frame, it's fine to do this from interrupt context.
/// Returns true if there are more actions to play, false if we're done
static bool do_frame(void)
{
	while (action_index < action_count)
	{
		const uint32_t action = actions[action_index].action;

		if (action == PRESS)
		{
			press_button(cur_param);
			if (++action_index < action_count)
			{
				cur_param = actions[action_index].param;
			}

		}
		else if (action == RELEASE)
		{
			release_button(cur_param);
			if (++action_index < action_count)
			{
				cur_param = actions[action_index].param;
			}
		}
		else if (action == DELAY)
		{
			if (cur_param == 0)
			{
				if (++action_index < action_count)
				{
					cur_param = actions[action_index].param;
				}
			}
			else
			{
				cur_param--;
				return true;
			}
		}
	}

	// We finished, now stop doing stuff
	if (action_count > 0)
	{
		action_count = 0;
		action_index = 0;
		report.buttons = 0;
		report.hat = 0;
		dpad_state = 0;
	}

	return false;
}

/// Timer callback, called once per frame (estimated)
static bool timer_callback(repeating_timer_t *rt)
{
	return do_frame();
}

/// VSYNC GPIO input callback, called once per frame based on the HDMI signal
static void vsync_callback(uint gpio, uint32_t event_mask)
{
	if (!do_frame())
	{
		gpio_set_irq_enabled(VSYNC_GPIO, GPIO_IRQ_EDGE_RISE, false);
	}
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
	(void)instance;
	(void)report;
	(void)len;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	(void)instance;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;

	return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
	(void)instance;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)bufsize;
}
