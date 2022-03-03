#include <switch.h>
#include <ctime>
#include <cstring>
#include "audio.hpp"

FILE* g_log = NULL;

constexpr size_t MemoryPageSize = 0x1000;

extern "C" {
    extern u32 __start__;
    u32 __nx_applet_type = AppletType_None; // Sysmodules should not use applet*.
    u32 __nx_fs_num_sessions = 1; // Sysmodules will normally only want to use one FS session.

    #define INNER_HEAP_SIZE 0x100000 // Adjust size as needed.
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    u32 __nx_nv_transfermem_size = 0x15000;

    void __libnx_init_time(void);
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump* ctx);
}

void __libnx_exception_handler(ThreadExceptionDump *ctx) { //needs to be defined, else linkage errors
    MemoryInfo mem_info; u32 page_info;
    svcQueryMemory(&mem_info, &page_info, ctx->pc.x);
}

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	// Newlib
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    Result res = smInitialize();
    if (R_FAILED(res)) {
        fatalThrow(res);
    }

    // Retrieve the current version of Horizon OS.
    if (R_SUCCEEDED(setsysInitialize())) {
        SetSysFirmwareVersion fw;
        if (R_SUCCEEDED(setsysGetFirmwareVersion(&fw)))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    if (R_FAILED(res = fsInitialize())) {
        fatalThrow(res);
    }

    if (R_FAILED(res = fsdevMountSdmc())) {
        fatalThrow(res);
    }
    fsSetPriority(FsPriority_Realtime);

#if ENABLE_LOGGING == 1
    g_log = fopen("sdmc:/bt/BTSounds.txt", "w");
    if (g_log == NULL || ferror(g_log)) 
        fatalThrow(0xFF);
#endif

    if (R_FAILED(res = btdrvInitialize())) {
        fatalThrow(res);
    }

    if (R_FAILED(res = audrenInitialize(&audrenCfg))) {
        fatalThrow(res);
    }

    else {
        audio& audio = audio::getInstance();
    }
}

void __appExit(void) {
    btdrvExit();
    fsdevUnmountAll();
    fsExit();
    smExit();
}

// Main program entrypoint
int main(int argc, char* argv[]) {
    fprintflush(g_log, "Sysmodule booted!\n");

    BtdrvAddress addresses[0x8] = {0};
    s32 origOutNum;
    Result res = btdrvGetConnectedAudioDevice(addresses, 0x8, &origOutNum);
    fprintflush(g_log, "btdrvGetConnectedAudioDevice 1 res: %08X\n", res);
    if (R_FAILED(res)) {
        fatalThrow(res);
    }

    Event connectionStateChanged;
    res = btdrvAcquireAudioConnectionStateChangedEvent(&connectionStateChanged, true);
    fprintflush(g_log, "btdrvAcquireAudioConnectionStateChangedEvent res: %08X\n", res);
    if (R_FAILED(res)) {
        fatalThrow(res);
    }

    while (appletMainLoop()) {
        res = eventWait(&connectionStateChanged, UINT64_MAX);
        fprintflush(g_log, "Event Changed res: %08X\n", res);

        memset(addresses, 0, sizeof(BtdrvAddress)*8);
        s32 newOutNum;
        res = btdrvGetConnectedAudioDevice(addresses, 0x8, &newOutNum);
        fprintflush(g_log, "btdrvGetConnectedAudioDevice 2 res: %08X\n", res);
        if (R_FAILED(res)) {
            continue;
        }

        fprintflush(g_log, "New: %d - Old: %d\n", newOutNum, origOutNum);
        if (newOutNum > origOutNum) { //new device connected
            fprintflush(g_log, "Bluetooth Connected\n");
            audio::getInstance().playConnectSound();
        }

        else if (newOutNum < origOutNum) { //device disconnected
            fprintflush(g_log, "Bluetooth Ready To Pair\n");
            audio::getInstance().playDisconnectSound();
        }

        origOutNum = newOutNum;
    }

#if ENABLE_LOGGING == 1
    fclose(g_log);
#endif

    return 0;
}
