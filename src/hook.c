#include "hook.h"
#include <string.h>
_Static_assert(sizeof(void *) == 4, "A 32-bit process is required");

static void jump(BYTE *buffer, const BYTE *instruction, const void *target) {
    uint32_t displacement = (uint32_t)((uintptr_t)target - (uintptr_t)(instruction + 5));
    buffer[0] = 0xe9;
    memcpy(buffer + 1, &displacement, sizeof(displacement));
}

BOOL C07InstallPlanHooks(BYTE *image, C07Hook hooks[C07_PLAN_HOOKS]) {
    DWORD old[C07_PLAN_HOOKS], ignored;
    unsigned prepared = 0, writable = 0;
    if (!image || !hooks) return FALSE;
    for (unsigned i = 0; i < C07_PLAN_HOOKS; ++i) {
        if (hooks[i].trampoline || !hooks[i].replacement || hooks[i].length < 5 ||
            hooks[i].length > sizeof(hooks[i].expected) ||
            memcmp(image + hooks[i].rva, hooks[i].expected, hooks[i].length)) return FALSE;
    }
    for (unsigned i = 0; i < C07_PLAN_HOOKS; ++i) {
        BYTE *trampoline = VirtualAlloc(NULL, hooks[i].length + 5,
                                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!trampoline) goto fail;
        hooks[i].trampoline = trampoline;
        ++prepared;
        /* All verified entry prologues contain no relative instructions. */
        memcpy(trampoline, image + hooks[i].rva, hooks[i].length);
        jump(trampoline + hooks[i].length, trampoline + hooks[i].length,
             image + hooks[i].rva + hooks[i].length);
        if (!VirtualProtect(trampoline, hooks[i].length + 5, PAGE_EXECUTE_READ, &ignored) ||
            !FlushInstructionCache(GetCurrentProcess(), trampoline, hooks[i].length + 5)) goto fail;
    }
    for (unsigned i = 0; i < C07_PLAN_HOOKS; ++i) {
        if (!VirtualProtect(image + hooks[i].rva, hooks[i].length,
                            PAGE_EXECUTE_READWRITE, &old[i])) goto fail;
        ++writable;
    }
    /* Publish trampolines before any detour can run. InitializeASI runs before play. */
    for (unsigned i = 0; i < C07_PLAN_HOOKS; ++i) {
        BYTE patch[8];
        memset(patch, 0x90, sizeof(patch));
        jump(patch, image + hooks[i].rva, hooks[i].replacement);
        memcpy(image + hooks[i].rva, patch, hooks[i].length);
        FlushInstructionCache(GetCurrentProcess(), image + hooks[i].rva, hooks[i].length);
    }
    /* Entries may share a page. Unwind in reverse so the earliest protection
       snapshot, taken before any entry made that page writable, wins last. */
    for (unsigned i = C07_PLAN_HOOKS; i-- > 0;)
        VirtualProtect(image + hooks[i].rva, hooks[i].length, old[i], &ignored);
    return TRUE;
fail:
    for (unsigned i = writable; i-- > 0;)
        VirtualProtect(image + hooks[i].rva, hooks[i].length, old[i], &ignored);
    for (unsigned i = 0; i < prepared; ++i) {
        VirtualFree(hooks[i].trampoline, 0, MEM_RELEASE);
        hooks[i].trampoline = NULL;
    }
    return FALSE;
}
