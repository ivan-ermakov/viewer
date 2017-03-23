#ifndef MEMORY_LEAKS_DETECTION_H
#define MEMORY_LEAKS_DETECTION_H

#if defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)
    #include <cstdlib>
    #include <crtdbg.h>
    #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
    #define new DEBUG_NEW
#endif

void installMemoryLeaksFilter();

#endif
