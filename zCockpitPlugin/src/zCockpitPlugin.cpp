

#include <XPLMUtilities.h>

#include "ZCockpitPlugin.h"
#include <stdio.h>
#include "../../../shared_src/logger.h"
extern logger LOG;

static XPLMDataRef zCockpitPluginVersion = nullptr;
float getzCockpitPluginVersion(void* inRefcon);

ZCockpitPlugin::ZCockpitPlugin() : flightLoopInterval(1.0f / 30.f) // Default to 30hz
{
    server = std::make_unique<Server>();
}

ZCockpitPlugin::~ZCockpitPlugin() {
    if (server != nullptr) {
        server = nullptr;
        LOG() << "server closed";
    }
}

float ZCockpitPlugin::flightLoop(float inElapsedSinceLastCall,
    float inElapsedTimeSinceLastFlightLoop,
    int inCounter,
    void* inRefcon) {
;
    // Tell each dataref to update its value through the XPLM api
 //// for (DataRef* ref : refs) updateDataRef(ref);

    // Do processing
    server->update();


    return flightLoopInterval;
}



int ZCockpitPlugin::pluginStart(char* outName, char* outSig, char* outDesc) {
    LOG() << "zCockpit Plugin started";

    strcpy_s(outName, sizeof("zCockpit"), "zCockpit");
    strcpy_s(outSig, sizeof("open.source.zCockpitplugin"), "open.source.zCockpitplugin");
    strcpy_s(outDesc, sizeof("This plugin supports hardware (switches. button, lights ...) for Zibo 738"), "This plugin supports hardware (switches. button, lights ...) for Zibo 738");


    g_menu_container_idx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "zCockpit", 0, 0);
    g_menu_id = XPLMCreateMenu("ZCockpit", XPLMFindPluginsMenu(), g_menu_container_idx, &ZCockpitPlugin::menu_handler, NULL);

    sprintf_s(msg, sizeof(msg), "ZCockpit Plugin Version: %0.1f", VERSION);
    XPLMAppendMenuItem(g_menu_id, msg, nullptr, 1);
    XPLMEnableMenuItem(g_menu_id, 0, 0);
    XPLMAppendMenuItem(g_menu_id, "Toggle Settings", (void*)"Menu Item 1", 1);
    XPLMAppendMenuSeparator(g_menu_id);
    XPLMAppendMenuItem(g_menu_id, "Toggle Shortcuts", (void*)"Menu Item 2", 1);
    XPLMAppendMenuItemWithCommand(g_menu_id, "Toggle Flight Configuration (Command-Based)", XPLMFindCommand("sim/operation/toggle_flight_config"));
    ;

    XPLMMenuID aircraft_menu = XPLMFindAircraftMenu();
    if (aircraft_menu) // This will be NULL unless this plugin was loaded with an aircraft (i.e., it was located in the current aircraft's "plugins" subdirectory)
    {
        XPLMAppendMenuItemWithCommand(aircraft_menu, "Toggle Settings (Command-Based)", XPLMFindCommand("sim/operation/toggle_settings_window"));
    }


 

    // Init application and server
 

    //register datarefs

    zCockpitPluginVersion = XPLMRegisterDataAccessor("zcockpit/plugin/version",
        xplmType_Float,
        0,
        nullptr, nullptr,
        getzCockpitPluginVersion, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr);

    //init dataref

    zCockpitPluginVersion = XPLMFindDataRef("zcockpit/plugin/version");
    XPLMSetDataf(zCockpitPluginVersion, VERSION);

    // Log that we have started
    sprintf_s(msg, sizeof(msg), "zCockpit: Plugin Version: %0.1f started\n", VERSION);

    XPLMDebugString(msg);

    return 1;
}


void ZCockpitPlugin::pluginStop() {
    server = nullptr;
    LOG() << "server closed";

    // Since we created this menu, we'll be good citizens and clean it up as well
    XPLMDestroyMenu(g_menu_id);
    // If we were able to add a command to the aircraft menu, it will be automatically removed for us when we're unloaded

    XPLMUnregisterDataAccessor(zCockpitPluginVersion);
}

void ZCockpitPlugin::receiveMessage(XPLMPluginID inFromWho, long inMessage, void* inParam) {
    LOG() << inFromWho << inMessage;
}

void  ZCockpitPlugin::menu_handler(void* in_menu_ref, void* in_item_ref)
{
    if (!strcmp((const char*)in_item_ref, "Menu Item 1"))
    {
        XPLMCommandOnce(XPLMFindCommand("sim/operation/toggle_settings_window"));
    }
    else if (!strcmp((const char*)in_item_ref, "Menu Item 2"))
    {
        XPLMCommandOnce(XPLMFindCommand("sim/operation/toggle_key_shortcuts_window"));
    }
}




float getzCockpitPluginVersion(void* inRefcon) {
    return VERSION;
}