#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <iostream>
#include <string>

#pragma comment(lib, "setupapi.lib")

static std::string extractSerialNumber(const std::string& deviceID) 
{
    size_t lastSlash = deviceID.find_last_of('\\');
    if (lastSlash != std::string::npos) {
        return deviceID.substr(lastSlash + 1);  // Extract part after the last '\'
    }
    return "Serial Number Not Found";
}

static std::string getSerialNumberFromCOMPort(const std::string& portName) 
{
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) 
        return "";

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    char friendlyName[MAX_PATH];
    bool found = false;
    std::string serialNumber = "";

    // Iterate over all available devices
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
        // Get the Friendly Name (e.g., "USB Serial Device (COM16)")
        if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL,
            (PBYTE)friendlyName, sizeof(friendlyName), NULL)) {
            std::string nameStr = friendlyName;

            // Check if this is the requested COM port
            if (nameStr.find("(" + portName + ")") != std::string::npos) {
                char deviceInstanceID[MAX_PATH];

                // Get the Device Instance ID (e.g., "USB\VID_04D8&PID_EF4A\95733333332351410021")
                if (SetupDiGetDeviceInstanceIdA(hDevInfo, &devInfoData, deviceInstanceID, sizeof(deviceInstanceID), NULL)) {
                    serialNumber = extractSerialNumber(deviceInstanceID);
                    found = true;
                    break;  // Stop once we find the correct port
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return serialNumber;
}