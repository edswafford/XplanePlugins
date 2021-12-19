// Downloaded from https://developer.x-plane.com/code-sample/x-plane-menu-sdk-sample/


#include "XPLMMenus.h"
#include <XPLMProcessing.h>

#include <string>
#include "ZCockpitPlugin.h"
#include "../../../shared_src/logger.h"




#if IBM
	#include <windows.h>
#endif



#ifndef XPLM300
	#error This is made to be compiled against the XPLM300 SDK
#endif

std::unique_ptr<ZCockpitPlugin> zcockpit_plugin{ nullptr };


logger LOG("ZCockpitPlugin.log");

void menu_handler(void *, void *);

PLUGIN_API float FlightLoopCallback(
	float inElapsedSinceLastCall,
	float inElapsedTimeSinceLastFlightLoop,
	int inCounter,
	void* inRefcon) {
		return zcockpit_plugin->flightLoop(inElapsedSinceLastCall, inElapsedTimeSinceLastFlightLoop, inCounter, inRefcon);
	return 1;
}


PLUGIN_API int XPluginStart(
						char *		outName,
						char *		outSig,
						char *		outDesc)
{
	zcockpit_plugin = std::make_unique<ZCockpitPlugin>();
	XPLMRegisterFlightLoopCallback(FlightLoopCallback, 0.01f, nullptr);
	return zcockpit_plugin->pluginStart(outName, outSig, outDesc);

}

PLUGIN_API void	XPluginStop(void)
{
	XPLMDebugString("zCockpit: Stopping plugin\n");
	XPLMUnregisterFlightLoopCallback(FlightLoopCallback, nullptr);
	XPLMDebugString("zCockpit: FlightLoop Callback Unregistered\n");
	zcockpit_plugin->pluginStop();
	XPLMDebugString("zCockpit: Plugin stopped!\n");
}

PLUGIN_API void XPluginDisable(void) {
	XPLMDebugString("zCockpit: Plugin disabled!\n");
}

PLUGIN_API int XPluginEnable(void) { 
	XPLMDebugString("zCockpit: Plugin enabled!\n");
	return 1;
}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam) {
	XPLMDebugString("zCockpit: Plugin received message!\n");
}


