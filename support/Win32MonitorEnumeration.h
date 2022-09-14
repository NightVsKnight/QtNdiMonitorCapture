#pragma once
#include <dwmapi.h>

#include <QString>

struct Monitor
{
public:
    Monitor(nullptr_t) {}
    Monitor(HMONITOR hmonitor, std::wstring& className, bool isPrimary)
    {
        m_hmonitor = hmonitor;
        m_className = className;
        m_bIsPrimary = isPrimary;
    }

    HMONITOR Hmonitor() const noexcept { return m_hmonitor; }
    std::wstring ClassName() const noexcept { return m_className; }
    bool IsPrimary() const noexcept { return m_bIsPrimary; }

private:
    HMONITOR m_hmonitor;
    std::wstring m_className;
    bool m_bIsPrimary;
};

BOOL WINAPI EnumMonitorProc(HMONITOR hmonitor,
    HDC hdc,
    LPRECT lprc,
    LPARAM data);

std::vector<Monitor> EnumerateMonitors();
