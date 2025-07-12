#include "pch.h"
#include "RageQuit.h"
#include <Windows.h>

BAKKESMOD_PLUGIN(RageQuit, "Rage Quit", "1.0", PLUGINTYPE_FREEPLAY)

void RageQuit::onLoad()
{
    // Initialize plugin settings - always enabled by default
    ragequit_enabled = std::make_shared<bool>(true);

    // Register console command
    cvarManager->registerNotifier("rage_quit", [this](std::vector<std::string> args) {
        this->rageQuit_onCommand();
        }, "Alt+F4 rage quit functionality", PERMISSION_ALL);

    // Register cvar for enabling/disabling - default value is 1 (enabled)
    // Force the cvar to be enabled and save it to config
    cvarManager->registerCvar("rage_quit_enabled", "1", "Enable rage quit Alt+F4 functionality", true, true, 0, true, 1);

    // Force enable the plugin and save the setting
    auto enabledCvar = cvarManager->getCvar("rage_quit_enabled");
    if (enabledCvar) {
        enabledCvar.setValue(true);
    }

    // Hook keyboard input events
    gameWrapper->HookEvent("Function Engine.PlayerInput.InputKey",
        [this](std::string eventName) {
            this->CheckKeyboardInput();
        });

    // Immediately start monitoring Alt+F4 without waiting
    HandleAltF4Combination();

    // Auto-enable plugin notification
    gameWrapper->Toast("RageQuit", "Plugin auto-enabled! Alt+F4 ready", "default", 3.0f);
    cvarManager->log("RageQuit plugin loaded and AUTO-ENABLED - Alt+F4 functionality active by default");
}

void RageQuit::onUnload()
{
    // Cleanup
    gameWrapper->UnhookEvent("Function Engine.PlayerInput.InputKey");
    cvarManager->log("RageQuit plugin unloaded");
}

void RageQuit::CheckKeyboardInput()
{
    // Plugin is always enabled by default - no need to check cvar
    // Force enable if somehow disabled
    auto enabledCvar = cvarManager->getCvar("rage_quit_enabled");
    if (enabledCvar && !enabledCvar.getBoolValue()) {
        enabledCvar.setValue(true);
        cvarManager->log("RageQuit auto-enabled (was disabled)");
    }

    // Monitor Alt+F4 combination using Windows API
    HandleAltF4Combination();
}

void RageQuit::HandleAltF4Combination()
{
    // Ensure plugin is always enabled
    EnsurePluginEnabled();

    // Check if Alt+F4 is pressed using Windows API
    bool currentAltPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;  // Alt key
    bool currentF4Pressed = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;     // F4 key

    // Detect Alt+F4 combination
    if (currentAltPressed && currentF4Pressed && (!altPressed || !f4Pressed)) {
        // Alt+F4 was just pressed
        OnAltF4Pressed();
    }

    // Update key states
    altPressed = currentAltPressed;
    f4Pressed = currentF4Pressed;

    // Continue monitoring
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        this->HandleAltF4Combination();
        }, 0.1f);
}

void RageQuit::EnsurePluginEnabled()
{
    // Force the plugin to stay enabled
    auto enabledCvar = cvarManager->getCvar("rage_quit_enabled");
    if (enabledCvar && !enabledCvar.getBoolValue()) {
        enabledCvar.setValue(true);
        *ragequit_enabled = true;
        cvarManager->log("RageQuit plugin auto-re-enabled");
    }
}

void RageQuit::ForceEnable()
{
    // Force enable the plugin
    auto enabledCvar = cvarManager->getCvar("rage_quit_enabled");
    if (enabledCvar) {
        enabledCvar.setValue(true);
    }
    *ragequit_enabled = true;
    autoEnabled = true;
    cvarManager->log("RageQuit plugin force-enabled");
}

void RageQuit::OnAltF4Pressed()
{
    // Record the current time
    auto currentTime = std::chrono::steady_clock::now();
    lastAltF4Time = currentTime;

    // Check current game state
    bool isInGame = gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame();
    bool isInMainMenu = !isInGame && gameWrapper->IsInFreeplay() == false;

    if (isInGame) {
        // First Alt+F4: Exit to main menu
        cvarManager->log("Alt+F4 pressed - Exiting to main menu");
        gameWrapper->Toast("Rage Quit", "Exiting to main menu...", "default", 2.0f);
        ExitToMainMenu();
        inMainMenu = true;
    }
    else if (inMainMenu || isInMainMenu) {
        // Second Alt+F4: Exit game completely
        cvarManager->log("Alt+F4 pressed - Exiting game");
        gameWrapper->Toast("Rage Quit", "Exiting game...", "default", 1.0f);
        ExitGame();
    }
    else {
        // Default behavior - exit to main menu first
        cvarManager->log("Alt+F4 pressed - Exiting to main menu (default)");
        gameWrapper->Toast("Rage Quit", "Exiting to main menu...", "default", 2.0f);
        ExitToMainMenu();
        inMainMenu = true;
    }
}

void RageQuit::ExitToMainMenu()
{
    try {
        // Disconnect from current match
        if (gameWrapper->IsInOnlineGame()) {
            cvarManager->executeCommand("disconnect");
        }
        else if (gameWrapper->IsInGame()) {
            cvarManager->executeCommand("disconnect");
        }

        // Force return to main menu
        gameWrapper->SetTimeout([this](GameWrapper* gw) {
            cvarManager->executeCommand("unreal_command open MainMenu");
            }, 0.5f);

        // Update state
        inMainMenu = true;

    }
    catch (const std::exception& e) {
        cvarManager->log("Error exiting to main menu: " + std::string(e.what()));
        cvarManager->executeCommand("disconnect");
    }
}

void RageQuit::ExitGame()
{
    try {
        // Show exit message
        cvarManager->log("Exiting Rocket League...");

        // Method 1: Use unreal quit command
        cvarManager->executeCommand("quit");

        // Method 2: Backup - force close after delay
        gameWrapper->SetTimeout([this](GameWrapper* gw) {
            cvarManager->executeCommand("unreal_command quit");
            }, 1.0f);

        // Method 3: Ultimate backup - Windows API
        gameWrapper->SetTimeout([this](GameWrapper* gw) {
            // Find Rocket League window and close it
            HWND rlWindow = FindWindow(NULL, L"Rocket League");
            if (rlWindow) {
                PostMessage(rlWindow, WM_CLOSE, 0, 0);
            }
            }, 2.0f);

    }
    catch (const std::exception& e) {
        cvarManager->log("Error exiting game: " + std::string(e.what()));
        // Force quit as last resort
        exit(0);
    }
}