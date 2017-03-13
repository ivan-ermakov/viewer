#if defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)

#pragma push_macro("calloc")
#pragma push_macro("free")
#pragma push_macro("malloc")
#pragma push_macro("realloc")
#pragma push_macro("new")

#undef calloc
#undef free
#undef malloc
#undef realloc
#undef new

#endif