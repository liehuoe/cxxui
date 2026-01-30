#include <type_traits>
#include <windows.h>

namespace cxxui::detail {

class SizeEventBase {
    template <typename T>
    friend class WindowBase;

public:
    int GetWidth() const { return LOWORD(lp_); }
    int GetHeight() const { return HIWORD(lp_); }

protected:
    LPARAM lp_;
};

}  // namespace cxxui::detail
