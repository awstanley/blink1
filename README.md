# Blink(1) Class

This is a class for [blink(1)] devices which uses [hidapi] and C++11.  It extends my previous work on creating a lightweight, easily portable, blink(1) library for when you wish to integrate it easily into an application.

hidapi source code is also in this repository.  hidapi is used under the BSD Licence (see src/LICENCE.hidapi).

## Requirements:
 * A compiler which supports C++11;
 * A platform supported by hidapi;
 * One or more blink(1) devices if you actually want output.

## Features:
 * Auto-selection of the first 'unused' blink(1) device.
 * Set and Get RGB
 * SetRGBN (MK2 only, calls will be translated to SetRGB for MK1)
 * Turn off alias (using ServerTickle)
 * Set pattern step
 * Play
 * Server tickle
 * System driven (threaded) 'action' loop to handle multiple notifications, using the following 'simple' actions:
   * Fade to RGBN

### System driven action loop
 * Add/Set/Remove action;
 * Auto-sleeps for the duration of the fade step;
 * Auto-halts the thread (but does not clear the loop) when server tickle is used.
 * Auto-halts and terminates the thread when 'play' is used;
   * Play is used to emulate repeated loops in MK1, to keep consistently MK2 devices also lose their system loop.


## Getting (and learning) serial numbers

Sometimes you need to know which is which, or you want to ensure that you always get the same order.

#### Getting serial numbers

This isn't really much of an example, but it shows how to get serials if you've lost them, or want them.

```c++
#include <vector>
#include <random>
#include <blink1.hpp>

int main(int argc, char **argv)
{
	Blink1::PrintDeviceSerials();
	return 0;
}
```


#### Colour identification

Sometimes it's easier (or better) to use colours to do it all at once.  This is configured to show 7 (I only have 6).

```c+++
#include <vector>
#include <random>
#include <blink1.hpp>

int colour_sets[7][3] = {
	{ 255, 0, 0 },
	{ 0, 255, 0 },
	{ 0, 0, 255 },
	{ 255, 255, 0},
	{ 255, 0, 255},
	{ 255, 255, 255 },
	{125, 0, 0 }
};

int main(int argc, char **argv)
{
	std::mt19937 rnd;
	std::random_device rd;
	rnd.seed(rd());

	int r, g, b;

	int count;
	wchar_t ** serials = Blink1::GetDeviceSerials(count);
	Blink1** devices = new Blink1*[count];
	for (int i = 0; i < count; i++)
	{
		devices[i] = new Blink1(serials[i]);
		printf("Serial: %ls (0x%02X,0x%02X,0x%02X)\n",
			devices[i]->GetDeviceSerial(), 
			colour_sets[i][0], colour_sets[i][1], colour_sets[i][2]);
		devices[i]->SetRGB(
			colour_sets[i][0], colour_sets[i][1], colour_sets[i][2]);
	}

	// Sleep for five seconds.
	std::this_thread::sleep_for(std::chrono::seconds(5));

	// Turn off and clean up
	for (int i = count - 1; i > -1; i--)
	{
		devices[i]->TurnOff();
		delete devices[i];
	}

	return 0;
}
```


## Useful examples:

### Example 1 - auto selection of free devices

The following source keeps creating and randomly setting the RGB on blink1 devices until there are none left, it then sleeps for five seconds, turns off the devices, and then exits.

```c++
#include <vector>
#include <random>
#include <blink1.hpp>

int main(int argc, char **argv)
{
	std::mt19937 rnd;
	std::random_device rd;
	rnd.seed(rd());

	int r, g, b;

	int count;
	wchar_t ** serials = Blink1::GetDeviceSerials(count);
	Blink1** devices = new Blink1*[count];
	for (int i = 0; i < count; i++)
	{
		r = rnd() % 255; g = rnd() % 255; b = rnd() % 255;
		devices[i] = new Blink1();
		printf("Serial: %ls (0x%02X,0x%02X,0x%02X)\n",
			devices[i]->GetDeviceSerial(), r, g, b);
		devices[i]->SetRGB(r, g, b);
	}

	// Sleep for five seconds.
	std::this_thread::sleep_for(std::chrono::seconds(5));

	for (int i = count - 1; i > -1; i--)
	{
		devices[i]->TurnOff();
		delete devices[i];
	}

	return 0;
}
```

### Example 2 - GetDeviceSerials

The following source uses the class to find all available devices using `GetDeviceSerials` function, randomly setting the RGB on blink1 devices as they are selected.  It then sleeps for five seconds, turns off the devices, and then exits.

```c++
#include <random>
#include <blink1.hpp>

int main(int argc, char **argv)
{
	std::mt19937 rnd;
	std::random_device rd;
	rnd.seed(rd());

	int r, g, b;

	int count;
	wchar_t ** serials = Blink1::GetDeviceSerials(count);
	Blink1** devices = new Blink1*[count];
	for (int i = 0; i < count; i++)
	{
		r = rnd() % 255; g = rnd() % 255; b = rnd() % 255;
		devices[i] = new Blink1(serials[i]);
		printf("Serial: %ls (0x%02X,0x%02X,0x%02X)\n",
			devices[i]->GetDeviceSerial(), r, g, b);
		devices[i]->SetRGB(r, g, b);
	}

	// Sleep for five seconds.
	std::this_thread::sleep_for(std::chrono::seconds(5));

	for (int i = count - 1; i > -1; i--)
	{
		devices[i]->TurnOff();
		delete devices[i];
	}

	return 0;
}
```

### Example 3 (Action list)

```c++
#include <vector>
#include <random>
#include <blink1.hpp>

int main(int argc, char **argv)
{
	std::mt19937 rnd;
	std::random_device rd;
	rnd.seed(rd());

	int count;
	wchar_t ** serials = Blink1::GetDeviceSerials(count);
	Blink1** devices = new Blink1*[count];

	int steps = (rd() % 5) + 5;

	int r, g, b, r2, g2, b2, f, d, max_f, max_d, run_f, run_d;

	// Maximum fade and duration (so we can stop and shutdown
	// after a full show)
	max_f = 0; max_d = 0;

	// Generate some fade and display times, as well as colours
	for (int i = 0; i < count; i++)
	{
		devices[i] = new Blink1();
		printf("-----\nSerial: %ls\n",devices[i]->GetDeviceSerial());

		run_f = 0;
		run_d = 0;

		for (int j = 0; j < steps; j++)
		{
			r = rnd() % 255; g = rnd() % 255; b = rnd() % 255;
			r2 = rnd() % 255; g2 = rnd() % 255; b2 = rnd() % 255;

			// Half a second (this could be rnd() too)
			d = 500;
			
			// Fade time
			f = (rnd() % 1000) + 500;

			run_f += f;
			run_d += d;

			printf(
				"Step %i\n"
				"  LED1: (0x % 02X, 0x % 02X, 0x % 02X)\n"
				"  LED2: (0x % 02X, 0x % 02X, 0x % 02X)\n"
				"  Duration: %i\n"
				"  Fade: %i\n\n",
				j, r, g, b, r2, g2, b2, d, f
			);

			devices[i]->AddAction(r, g, b, d, f, r2, g2, b2);
		}

		if (run_d > max_d){ max_d = run_d; }
		if (run_f > max_f){ max_f = run_f; }

		devices[i]->StartActionLoop();
	}

	printf("Sleeping for %i+%i+2000 (%i) while it runs.\n",
		max_d, max_f, (max_d + max_f + 2000));

	// Sleep for the duration, fade, and 2000 milliseconds
	std::this_thread::sleep_for(
		std::chrono::milliseconds(
			(max_d + max_f + 2000)
		)
	);

	for (int i = count - 1; i > -1; i--)
	{
		devices[i]->TurnOff();
		delete devices[i];
	}

	return 0;
}
```

## Licence (ISC)
Copyright (c) 2014, A.W. Stanley

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted, provided
that the above copyright notice and this permission notice appear
in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.



## Changes

2014-06-14:
  Moved from hacky scripts, gisted code, and tested on my array of Blink(1) (mk1) devices.
  

## Notes
 * Built from the information provided in the [blink1 docs directory]
 * When my Blink(1) mk2 arrives I'll test on it too.
 * hidapi is used in this code, and distributed in this repository, under the BSD Licence.
 * This particular version has only been lightly tested on Windows 8.1, but it should work elsewhere with little to no changes.

[hidapi]:https://github.com/signal11/hidapi
[blink(1)]:http://thingm.com/products/blink-1/
[blink1 docs directory]:https://github.com/todbot/blink1/blob/master/docs/blink1-hid-commands.md