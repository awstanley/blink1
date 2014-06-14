// Copyright (c) 2014, A.W. Stanley
// Released under the ISC Licence; See LICENCE.

#if defined _WIN32 || defined _WIN64
#pragma comment (lib, "setupapi.lib")
#endif

#include "hidapi.h"
#include "../include/blink1.hpp"

#include <vector>
#include <thread>
#include <chrono>

#define THINGM_VENDOR_ID 0x27B8
#define BLINK1M_PRODUCT_ID 0x1ED

// Configure based on the WS2812 RGB LED:
// http://rgb-123.com/ws2812-color-output/
// Output (as provided) from: GammaE=255*(res/255).^(1/.45)
int GammaE[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3,
3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8,
8, 9, 9, 9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14,
15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 21, 21, 22, 22,
23, 23, 24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32,
33, 34, 34, 35, 36, 37, 37, 38, 39, 40, 40, 41, 42, 43, 44,
45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
74, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89, 90, 91,
93, 94, 95, 96, 98, 99, 100, 102, 103, 104, 106, 107, 109,
110, 111, 113, 114, 116, 117, 119, 120, 121, 123, 124, 126,
128, 129, 131, 132, 134, 135, 137, 138, 140, 142, 143, 145,
146, 148, 150, 151, 153, 155, 157, 158, 160, 162, 163, 165,
167, 169, 170, 172, 174, 176, 178, 179, 181, 183, 185, 187,
189, 191, 193, 194, 196, 198, 200, 202, 204, 206, 208, 210,
212, 214, 216, 218, 220, 222, 224, 227, 229, 231, 233, 235,
237, 239, 241, 244, 246, 248, 250, 252, 255 };

std::vector<wchar_t*> Blink1::m_busy;

void Blink1::WorkerThread()
{
	ActionRGB *act;
	unsigned int pos = 0;
	while (true)
	{
		while (m_worker_running && m_actions.size() > 0)
		{
			if (pos >= m_actions.size()) {pos = 0;}

			act = m_actions[pos];
			
			if (act != NULL)
			{
				std::unique_lock<std::mutex> lock(m_worker_lock);
				
				if (m_build == eMk2)
				{
					_FadeToRGBN(act->fade_time, act->red,
						act->green, act->blue, 1, true);
					_FadeToRGBN(act->fade_time, act->red2,
						act->green2, act->blue2, 2, true);
				}
				else
				{
					_FadeToRGB(act->fade_time, act->red,
						act->green, act->blue, true);
				}
				lock.unlock();

				unsigned int st = act->fade_time + act->display_duration;
				std::this_thread::sleep_for(std::chrono::milliseconds(st));
			}
			pos++;
		}
		pos = 0;

		// Sleep for a second when it's not running
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

wchar_t* Blink1::GetDeviceSerial()
{
	wchar_t *s = new wchar_t[9];
	wmemset(s, 0, 9);
	if (m_device != NULL)
	{
		hid_get_serial_number_string((hid_device*)m_device, s, 9);
	}
	return s;
}

void Blink1::RemoveAction(unsigned int position)
{
	std::unique_lock<std::mutex> lock(m_worker_lock);
	if (position < m_actions.size())
	{
		m_actions.erase(m_actions.begin()+position);
	}
	lock.unlock();
}

// Add action to the end of the queue
void Blink1::AddAction(
	unsigned int red,
	unsigned int green,
	unsigned int blue,
	unsigned int display_time,
	unsigned int fade_time,
	unsigned int red2,
	unsigned int green2,
	unsigned int blue2)
{
	std::unique_lock<std::mutex> lock(m_worker_lock);
	ActionRGB *a = new ActionRGB();
	a->red = red;
	a->green = green;
	a->blue = blue;
	a->red2 = red2;
	a->green2 = green2;
	a->blue2 = blue2;
	a->fade_time = fade_time;
	a->display_duration = display_time;
	m_actions.push_back(a);
	lock.unlock();
}

// Set action 
void Blink1::SetAction(unsigned short position,
	unsigned int red,
	unsigned int green,
	unsigned int blue,
	unsigned int display_time,
	unsigned int fade_time,
	unsigned int red2,
	unsigned int green2,
	unsigned int blue2)
{
	std::unique_lock<std::mutex> lock(m_worker_lock);
	ActionRGB *a = new ActionRGB();
	a->red = red;
	a->green = green;
	a->blue = blue;
	a->red2 = red2;
	a->green2 = green2;
	a->blue2 = blue2;
	a->fade_time = fade_time;
	a->display_duration = display_time;

	if (position < m_actions.size())
	{
		m_actions[position] = a;
		lock.unlock();
		return;
	}
	else
	{
		if (m_actions.size() == position)
		{
			m_actions.push_back(a);
			lock.unlock();
			return;
		}
	}
	// If this point is reached the action needs to be deleted
	delete a;
	lock.unlock();
}

void Blink1::ClearActions()
{
	std::unique_lock<std::mutex> lock(m_worker_lock);
	m_worker_running = false;
	while (m_actions.begin() != m_actions.end())
	{
		delete m_actions[0];
		m_actions.erase(m_actions.begin());
	}
	lock.unlock();
}

// Get the device serial
// MK1: <= 0x1FFFFFFF
// MK2: >= 0x20000000
void Blink1::Init()
{
	wchar_t *s = GetDeviceSerial();

	static const char hex[] = "0123456789ABCDEF";
	unsigned char g = hex[*s % 16];
	m_build = eMk1;
	if (g >= 2){m_build = eMk2;}

	m_worker_running = false;
	m_repeat_worker = new std::thread(&Blink1::WorkerThread, this);
}

bool Blink1::Write(void* b, int l)
{
	if (m_device == NULL) { return false; }
	int rc = 0;
	hid_device *D = (hid_device*)m_device;
	if (D != NULL)
	{
		rc = hid_send_feature_report(D,(unsigned char*)b,l);
	}
	return (rc != -1);
}

bool Blink1::Read(void* b, int l)
{
	if (m_device == NULL) { return false; }
	int rc = 0;
	hid_device *D = (hid_device*)m_device;
	if (D != NULL)
	{
		//rc = hid_send_feature_report(D, (unsigned char*)b, l);
		rc = hid_get_feature_report(D, (unsigned char*)b, l);
		hid_close(D);
	}
	return (rc != -1);
}

// x Set RGB color now format: {'n', r,g,b, 0,0, 0,0 }
void Blink1::SetRGB(unsigned char r, unsigned char g,
	unsigned char b)
{
	ClearActions();
	unsigned char B[9]={1,'n',r,g,b,0,0,0,0};
	Write(B, 9);
}

// x Fade to RGB color format: {'c', r,g,b, th,tl, 0,0 }
void Blink1::_FadeToRGB(int f, unsigned char r,
	unsigned char g, unsigned char b, bool from_loop)
{
	if (!from_loop){m_worker_running = false;ClearActions();}
	unsigned char B[9] = { 1, 'c', r, g, b, ((f / 10) >> 8), (f / 10) % 0xFF, 0, 0 };
	Write(B, 9);
}

// x Fade to RGB color format: {'c', r,g,b, th,tl, 0,0 }
void Blink1::FadeToRGB(int f, unsigned char r,
	unsigned char g, unsigned char b)
{
	_FadeToRGB(f, r, g, b, false);
}

// todo: emulate count and end
void Blink1::Play(unsigned char o, unsigned char b,
	unsigned char e, unsigned char c)
{
	ClearActions();
	if (m_build == eMk1) { c = 0; e = 0; }
	unsigned char B[9]={1,'p',o,b,e,c,0,0,0};
	Write(B, 9);
}

void Blink1::TurnOff()
{
	ServerTickle(0, 0, 0, 0, 0);
}

void Blink1::ServerTickle(unsigned char o,
	unsigned char f, unsigned char b,
	unsigned char e, unsigned char m)
{
	m_worker_running = false;
	if(m_build==eMk1){m=0;}
	unsigned char B[9]={1,'D',o,((f/10)>>8),(f/10)%0xFFc,m,b,e};
	Write(B, 9);
}

void Blink1::WritePatternStep(unsigned short f, unsigned char r,
	unsigned char g, unsigned char b, unsigned char p)
{
	unsigned char B[9]={1,'P',GammaE[r],GammaE[g],GammaE[b],((f/10)>>8),((f/10)%0xff),p,0};
	Write(B,9);
}

// x Fade to RGB color format: {'c', r,g,b, th,tl, 0,0 }
void Blink1::_FadeToRGBN(int f, unsigned char r,
	unsigned char g, unsigned char b, unsigned char n,
	bool from_loop)
{
	if (!from_loop){ m_worker_running = false; ClearActions(); }
	unsigned char B[9] = { 1, 'c', r, g, b, ((f / 10) >> 8), (f / 10) % 0xFF, n, 0 };
	Write(B, 9);
}


// x Fade to RGB color format: {'c', r,g,b, th,tl, 0,0 }
void Blink1::FadeToRGBN(int f, unsigned char r,
	unsigned char g, unsigned char b, unsigned char n)
{
	_FadeToRGBN(f, r, g, b, n, false);
}

int Blink1::GetRGB(unsigned char &r,
	unsigned char &g, unsigned char &b)
{
	unsigned char B[9] = { 1 };
	if (Read(B,9))
	{
		r = B[2]; g = B[3]; b = B[4];
		return true;
	}
	return false;
}

// Copy out
wchar_t** Blink1::GetDeviceSerials(int &count)
{
	count = 0;
	struct hid_device_info *devs, *cur_dev;
	int length, i;
	wchar_t **serials;

	// Get the number
	devs = hid_enumerate(THINGM_VENDOR_ID, BLINK1M_PRODUCT_ID);
	cur_dev = devs;
	while (cur_dev){ count++; cur_dev = cur_dev->next; }
	hid_free_enumeration(devs);

	// Store the number
	serials = new wchar_t*[count];
	devs = hid_enumerate(THINGM_VENDOR_ID, BLINK1M_PRODUCT_ID);
	cur_dev = devs;
	for (i = 0; i < count; i++)
	{
		length = wcslen(cur_dev->serial_number);
		serials[i] = new wchar_t[length + 1];
		wmemcpy(serials[i], cur_dev->serial_number, length);
		serials[i][length] = 0;
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);
	return serials;
}

void Blink1::PrintDeviceSerials()
{
	int count;
	wchar_t** serials = GetDeviceSerials(count);
	for (int i = 0; i < count; i++)
	{
		printf("Serial (%i): %ls\r\n", i, serials[i]);
	}
}

void Blink1::StartActionLoop()
{
	m_worker_running = true;
}

void Blink1::HaltActionLoop()
{
	m_worker_running = false;
}

bool Blink1::SerialBusy(wchar_t *s)
{
	for (unsigned int j = 0; j < m_busy.size(); j++)
	{
		if (wcscmp(m_busy[j],s) == 0)
		{
			return true;
		}
	}
	return false;
}

wchar_t* Blink1::GetNextSerial()
{
	int c;
	wchar_t** s = GetDeviceSerials(c);
	for (int i = 0; i < c; i++)
	{
		if (!SerialBusy(s[i]))
		{
			return s[i];
		}
	}
	return NULL;
}

Blink1::Blink1(wchar_t *serial)
{
	std::unique_lock<std::mutex> lock(m_busy_lock);
	bool valid = true;
	if (SerialBusy(serial))
	{
		lock.unlock();
		return;
	}
	m_device = hid_open(THINGM_VENDOR_ID, BLINK1M_PRODUCT_ID, serial);
	if (m_device != NULL)
	{
		m_busy.push_back(serial);
		Init();
	}
	lock.unlock();
}

Blink1::Blink1()
{
	std::unique_lock<std::mutex> lock(m_busy_lock);
	wchar_t* serial = GetNextSerial();
	if (serial == NULL)
	{
		m_device = NULL;
		lock.unlock();
		return;
	}
	m_device = hid_open(THINGM_VENDOR_ID, BLINK1M_PRODUCT_ID, serial);
	if (m_device != NULL)
	{
		m_busy.push_back(serial);
		Init();
	}
	lock.unlock();
}

Blink1::~Blink1()
{
	std::unique_lock<std::mutex> lock(m_busy_lock);
	
	// Get the serial (to free it)
	wchar_t *serial = GetDeviceSerial();

	// Remove the serial from the list of busy devices
	for (unsigned int j = 0; j < m_busy.size(); j++)
	{
		if (m_busy[j] == serial)
		{
			m_busy.erase(m_busy.begin() + j);
			break;
		}
	}

	// Close it
	hid_close((hid_device*)m_device);

	lock.unlock();
}