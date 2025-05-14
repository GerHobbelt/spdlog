#pragma once

#ifndef NOMINMAX
    #define NOMINMAX  // prevent windows redefining min/max
#endif

#if 0		               // [GHo: WIN32_LEAN_AND_MEAN is cute, but no cigar: when header files such as these do this and are themselves loaded in sourcefiles of larger applications/userland code, things can break in very hard-to-diagnose ways! So... DON'T!
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <winsock2.h>   // [GHo] fixes issues elsewhere, e.g. spdlog, when this header file is loaded before another, which loads winsock2.h or (*shudder*) winsock.h, e.g. windows.h: you'll get clashes in preprocessor defines in ws2def.h vs. winsock2.h  :-((
#include <windows.h>
