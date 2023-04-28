#include "fastfetch.h"

#include "modules/modules.h"

int main(int argc, char** argv)
{
    //Disable compiler warnings
    FF_UNUSED(argc, argv);

    FFinstance instance;
    ffInitInstance(&instance); //This also applys default configuration to instance.config

    //Modify instance.config here

    // ffPrepareCPUUsage();
    // ffPreparePublicIp(&instance);
    // ffPrepareWeather(&instance);

    //Does things like starting detection threads, disabling line wrap, etc
    ffStart(&instance);

    //Printing
    ffPrintTitle(&instance, &instance.config.title);
    ffPrintSeparator(&instance, &instance.config.separator);
    ffPrintOS(&instance, &instance.config.os);
    ffPrintHost(&instance, &instance.config.host);
    //ffPrintBios(&instance, &instance.config.bios);
    //ffPrintBoard(&instance, &instance.config.board);
    //ffPrintChassis(&instance);
    ffPrintKernel(&instance, &instance.config.kernel);
    //ffPrintProcesses(&instance);
    ffPrintUptime(&instance, &instance.config.uptime);
    ffPrintPackages(&instance);
    ffPrintShell(&instance);
    ffPrintDisplay(&instance, &instance.config.display);
    // ffPrintBrightness(&instance);
    ffPrintDesktopEnvironment(&instance);
    ffPrintWM(&instance);
    ffPrintWMTheme(&instance, &instance.config.wmTheme);
    ffPrintTheme(&instance);
    ffPrintIcons(&instance);
    ffPrintFont(&instance, &instance.config.font);
    ffPrintCursor(&instance, &instance.config.cursor);
    ffPrintTerminal(&instance);
    ffPrintTerminalFont(&instance);
    ffPrintCPU(&instance, &instance.config.cpu);
    ffPrintGPU(&instance, &instance.config.gpu);
    ffPrintMemory(&instance);
    //ffPrintSwap(&instance);
    ffPrintDisk(&instance, &instance.config.disk);
    ffPrintBattery(&instance, &instance.config.battery);
    ffPrintPowerAdapter(&instance);
    //ffPrintPlayer(&instance);
    //ffPrintMedia(&instance);
    //ffPrintLocalIp(&instance);
    //ffPrintPublicIp(&instance);
    //ffPrintWifi(&instance);
    //ffPrintCPUUsage(&instance, &instance.config.cpuUsage);
    ffPrintLocale(&instance, &instance.config.locale);
    //ffPrintDateTime(&instance);
    //ffPrintDate(&instance);
    //ffPrintTime(&instance);
    //ffPrintVulkan(&instance);
    //ffPrintOpenGL(&instance);
    //ffPrintOpenCL(&instance);
    //ffPrintUsers(&instance);
    //ffPrintWeather(&instance);
    //ffPrintBluetooth(&instance);
    //ffPrintSound(&instance, &instance.config.sound);
    //ffPrintGamepad(&instance);
    ffPrintBreak(&instance);
    ffPrintColors(&instance);

    ffFinish(&instance);
    ffDestroyInstance(&instance);
    return 0;
}
