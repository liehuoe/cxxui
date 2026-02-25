#pragma once

#include <shellscalingapi.h>
#ifdef _MSC_VER
    #pragma comment(lib, "shcore.lib")  // GetDpiForMonitor
#endif

#include "library.hpp"

namespace cxxui::detail {
class Shcore {
public:
    UINT GetDpiForMonitor(HMONITOR hmonitor) {
        if (auto func = lib_.Get<GetDpiForMonitorPtr>("GetDpiForMonitor"); func) {
            UINT dpi_x, dpi_y;
            if (SUCCEEDED(func(hmonitor, MDT_DEFAULT, &dpi_x, &dpi_y))) {
                return dpi_x;
            }
        }
        return GetDefaultDpi();
    }

private:
    UINT GetDefaultDpi() const noexcept {
        HDC hdc = GetDC(nullptr);
        UINT dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        return dpi ? dpi : 96;
    }
    using GetDpiForMonitorPtr = HRESULT(WINAPI*)(HMONITOR hmonitor,
                                                 MONITOR_DPI_TYPE dpiType,
                                                 UINT* dpiX,
                                                 UINT* dpiY);
    Library lib_{"shcore.dll"};
};

}  // namespace cxxui::detail
