// Copyright (c) 2014, A.W. Stanley
// Released under the ISC Licence; See LICENCE.

#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

// A class for Blink(1) designed for manual use of fading.
// See FEATURES for more information.
class Blink1
{
private:

	// Used for internal operations
	enum eBlinkBuild
	{
		eMk1 = 0,
		eMk2
	};

	// Get the build (mk1, mk2, and for the future)
	eBlinkBuild m_build;

	// A pointer to the device.
	// (Actually hid_device*, but hidapi isn't required elsewhere
	//  so this is masked behind a void pointer.)
	void *m_device;

	// Internal write
	bool Write(void* b, int l);

	// Internal read
	bool Read(void* b, int l);

	// Busy devices
	static std::vector<wchar_t*> m_busy;

	// Busy lock
	std::mutex m_busy_lock;

	// Lock for the thread's work (to prevent device locks)
	std::mutex m_worker_lock;

	// Internal thread
	std::thread* m_repeat_worker;

	// Worker thread running
	std::atomic<bool> m_worker_running;

	// Worker function
	void WorkerThread();

	// Action
	struct ActionRGB
	{
		// How many milliseconds it should be visible
		// (Total display time is fade + display)
		unsigned int display_duration;

		// How long it takes (0 = SetRGB)
		// (Total display time is fade + display)
		unsigned int fade_time;

		// Red (0-255) - LED1
		unsigned char red;

		// Green (0-255) - LED1
		unsigned char green;

		// Blue (0-255) - LED1
		unsigned char blue;

		// Red (0-255) - LED2
		unsigned char red2;

		// Green (0-255) - LED2
		unsigned char green2;

		// Blue (0-255) - LED2
		unsigned char blue2;
	};

	// Array of actions
	std::vector<ActionRGB*> m_actions;

	// After setting the serial and locking it into the busy array
	// this is used.
	void Init();

	// Checks if a serial is listed as busy
	bool SerialBusy(wchar_t *serial);

	// Gets the next 'free' serial using SerialBusy
	wchar_t *GetNextSerial();

	// Internal fade function (for loop)
	void _FadeToRGBN(int f, unsigned char r,
		unsigned char g, unsigned char b, unsigned char n,
		bool from_loop = false);

	// Internal fade function (for loop)
	void _FadeToRGB(int f, unsigned char r,
		unsigned char g, unsigned char b,
		bool from_loop = false);

public:

	// Start the action loop
	void StartActionLoop();

	// Stop the action loop
	void HaltActionLoop();

	// Remove an action
	void RemoveAction(unsigned int position);

	// Add action to the end of the queue
	// (Red2, Green2, and Blue2 are only used on MK2)
	void AddAction(
		unsigned int red,
		unsigned int green,
		unsigned int blue,
		unsigned int display_duration = 1000,
		unsigned int fade_time = 1000,
		unsigned int red2 = 0,
		unsigned int green2 = 0,
		unsigned int blue2 = 0);

	// Set action 
	// (Red2, Green2, and Blue2 are only used on MK2)
	void SetAction(unsigned short position,
		unsigned int red,
		unsigned int green,
		unsigned int blue,
		unsigned int display_duration=1000, 
		unsigned int fade_time = 1000,
		unsigned int red2 = 0,
		unsigned int green2 = 0,
		unsigned int blue2 = 0);

	// Clear remaining Blink1 actions or stop the loop.
	void ClearActions();

	// Fades the Blink1 to R/G/B (0-255 range)
	// over fade_duration milliseconds.
	void FadeToRGB(int fade_duration,
		unsigned char red, unsigned char green,
		unsigned char blue);

	// See FadeToRGB for all variables except 'n', which
	// refers to the RGB LED used.
	// Note: Auto-mapped to FadeToRGB
	void FadeToRGBN(int f, unsigned char r,
		unsigned char g, unsigned char b, unsigned char n);

	// Set the Blink1 to R/G/B (0-255 range), instantly.
	void SetRGB(unsigned char red,
		unsigned char green, unsigned char blue);

	// Get the R/G/B (0-255 range)
	int GetRGB(unsigned char &r,
		unsigned char &g, unsigned char &b);

	// Server tickle command
	// clears the action loop
	void ServerTickle(unsigned char on_off,
		unsigned char timeout, unsigned char startpos,
		unsigned char endpos, unsigned char maintain = 0);

	// Write steps
	// does not impact the action loop
	void WritePatternStep(unsigned short f,
		unsigned char r, unsigned char g,
		unsigned char b, unsigned char p);

	// Play
	// clears the action loop
	void Play(unsigned char play,
		unsigned char start, unsigned char end = 0,
		unsigned char count = 0);

	// Turn blink off (easy alias for servertickle 'off')
	void TurnOff();

	// Create an instance from the first *free* Blink1
	Blink1();

	// Create an instance from a given serial
	Blink1(wchar_t *serial);

	// Cleans up and 'frees' the Blink1
	~Blink1();

	// Get the current device serial (or "" if no device loaded)
	wchar_t* GetDeviceSerial();

	// Get a list of serials
	static wchar_t **GetDeviceSerials(int &count);

	// Print serials
	static void PrintDeviceSerials();
};