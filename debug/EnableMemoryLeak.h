#if defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)

#pragma pop_macro("new")
#pragma pop_macro("realloc")
#pragma pop_macro("malloc")
#pragma pop_macro("free")
#pragma pop_macro("calloc")

#endif
