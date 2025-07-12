void RageQuit::rageQuit_onCommand()
{
    // Manual command execution
    cvarManager->log("Manual rage quit command executed");
    OnAltF4Pressed();
};