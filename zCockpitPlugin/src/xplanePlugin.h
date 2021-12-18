#pragma once


#include <XPLMDataAccess.h>
#include <XPLMMenus.h>
#include "Server.h"

#define VERSION 0.2f

class XPlanePlugin {

private:
	XPlanePlugin();
	~XPlanePlugin();
	XPlanePlugin(const XPlanePlugin&) = delete;
	XPlanePlugin& operator=(const XPlanePlugin&) = delete;


public:
	static XPlanePlugin& getInstance()
	{
		static XPlanePlugin instance;
		return instance;
	}

	// X-Plane API
	float flightLoop(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon);
	int pluginStart(char* outName, char* outSig, char* outDesc);
	void pluginStop();
	void receiveMessage(XPLMPluginID inFromWho, long inMessage, void* inParam);

	static void menu_handler(void* in_menu_ref, void* in_item_ref);


public: // DataRefProvider implementation



private:
	float flightLoopInterval; // Delay between loop calls (in seconds)

	int g_menu_container_idx; // The index of our menu item in the Plugins menu
	XPLMMenuID g_menu_id; // The menu container we'll append all our menu items to
	char msg[200];


	// Server initializes Network
	// All network communication goes through the server
	Server server;
};