// Defines a custom assertion macro
#pragma once

#ifdef _DEBUG
extern bool DbgAssertFunction(bool expr, const char* expr_string, const char* desc, int line_num, const char* file_name);
#ifdef _WIN32
#define DbgAssert(expr, description) {if (DbgAssertFunction((expr), "expr", description, __LINE__, __FILE__)) {__debugbreak();}}
#else
#define DbgAssert(expr, description) {if (DbgAssertFunction((expr), "expr", description, __LINE__, __FILE__)) {SDL_TriggerBreakpoint();}}
#endif
#else
#define DbgAssert(expr, description)
#endif // _DEBUG
