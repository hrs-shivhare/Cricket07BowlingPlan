#ifndef C07_BOWLING_PLAN_HOOK_H
#define C07_BOWLING_PLAN_HOOK_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#define C07_PLAN_HOOKS 9
typedef struct C07Hook {
    uint32_t rva;
    unsigned length;
    BYTE expected[8];
    const void *replacement;
    void *trampoline;
} C07Hook;
/* All signatures and memory protections are checked before any entry changes. */
BOOL C07InstallPlanHooks(BYTE *image, C07Hook hooks[C07_PLAN_HOOKS]);
#endif
