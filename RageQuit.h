#pragma once
#pragma comment(lib, "pluginsdk.lib")
#include "bakkesmod/plugin/bakkesmodplugin.h"

class RageQuit : public BakkesMod::Plugin::BakkesModPlugin
void RageQuit::rageQuit_onCommand()
{
    // Manual command execution
    cvarManager->log("Manual rage quit command executed");
    OnAltF4Pressed();
};
