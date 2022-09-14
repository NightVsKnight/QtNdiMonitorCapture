#include "Win32MonitorEnumeration.h"

BOOL WINAPI EnumMonitorProc(HMONITOR hmonitor,
  HDC,
  LPRECT,
  LPARAM data)
{
  MONITORINFOEX info_ex;
  info_ex.cbSize = sizeof(MONITORINFOEX);

  GetMonitorInfo(hmonitor, &info_ex);

  if (info_ex.dwFlags == DISPLAY_DEVICE_MIRRORING_DRIVER)
    return true;

  auto monitors = ((std::vector<Monitor>*)data);
  std::wstring name = info_ex.szDevice;
  auto monitor = Monitor(hmonitor, name, info_ex.dwFlags & MONITORINFOF_PRIMARY);

  monitors->emplace_back(monitor);

  return true;
}

std::vector<Monitor> EnumerateMonitors()
{
  std::vector<Monitor> monitors;
  ::EnumDisplayMonitors(NULL, NULL, EnumMonitorProc, (LPARAM)&monitors);
  return monitors;
}
