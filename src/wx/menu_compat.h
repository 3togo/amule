//
// wxWidgets 3.2 menu.h compatibility wrapper
// This header ensures all required headers are included before menu.h
//

#ifndef _WX_MENU_COMPAT_H_
#define _WX_MENU_COMPAT_H_

// Include our window compatibility wrapper first
#include "window_compat.h"

// Prevent the system window.h from being included again
#define _WX_WINDOW_H_BASE_

// Now include the actual menu.h
#include <wx/menu.h>

#endif // _WX_MENU_COMPAT_H_
