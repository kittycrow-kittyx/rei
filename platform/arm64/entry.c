#include "croissant/all.h"
#include "console.h"
#ifdef REI_ROOT_SD
#include "storage.h"
#endif
extern const uint8_t rei_root_start[], rei_root_end[];
#define K(name) croissant_native_kernel_ts__##name
#define M(name) croissant_native_memory_ts__##name
#define V(name) croissant_native_vfs_ts__##name
#define A(name) croissant_native_abi_ts__##name
static void require(bool ok,const char *message){if(!ok)rei_panic(message);}
static void status(int32_t code,const char *message){
    if(code){rei_console_puts(message);rei_console_puts(" status=");
        if(code<0){rei_console_char('-');rei_console_number((uint64_t)-(int64_t)code);}
        else rei_console_number((uint32_t)code);
        rei_panic("\n");}
}
static void copy_bytes(croissant_native_memory_ts__TetoMemory mem,uint32_t at,const uint8_t *bytes,uint32_t n){
    for(uint32_t i=0;i<n;i++)__bake_c_u8_set(mem.bytes,at+i,bytes[i]);
}
static uint32_t component(croissant_native_memory_ts__TetoMemory mem,uint32_t parent,uint32_t scratch,const char *name){
    uint32_t size=0;
    while(name[size]){M(storeU8)(mem,scratch+size,name[size]);size++;}
    uint32_t inode=V(tetoVfsLookup)(mem,parent,scratch,size);
    require(inode!=0 && inode!=UINT32_MAX,"REI: VFS LOOKUP FAILED\n");
    return inode;
}
static void run_hello(croissant_native_memory_ts__TetoMemory mem,uint32_t root){
    const uint32_t scratch=K(tetoImageReserve)(mem,256);
    require(scratch!=UINT32_MAX,"REI: SCRATCH RESERVE FAILED\n");
    const uint32_t etc=component(mem,root,scratch,"etc");
    const uint32_t motd=component(mem,etc,scratch,"motd");
    uint64_t size=V(tetoVfsFileSize)(mem,motd);
    require(size>0&&size<=256,"REI: MOTD SIZE INVALID\n");
    require(V(tetoVfsReadData)(mem,motd,0,scratch,(uint32_t)size)==(int32_t)size,"REI: MOTD READ FAILED\n");
    for(uint32_t i=0;i<size;i++)rei_console_char((char)M(loadU8)(mem,scratch+i));
    rei_console_puts("\nREI: MIKUOS MOTD READ PASS\n");
    uint32_t usr=component(mem,root,scratch,"usr");
    uint32_t bin=component(mem,usr,scratch,"bin");
    uint32_t hello=component(mem,bin,scratch,"hello-rv64.39");
    size=V(tetoVfsFileSize)(mem,hello);
    status(K(tetoImageRelease)(mem,scratch,256),"REI: SCRATCH RELEASE FAILED");
    require(size>0&&size<1024*1024,"REI: HELLO SIZE INVALID\n");
    status(K(tetoHartInit)(mem,0,UINT64_C(0x4000000000),0),"REI: HART INIT FAILED");
    status(K(tetoProcessInit)(mem,0,1,0,0,0,0,0,0,0),"REI: PROCESS INIT FAILED");
    uint32_t input=K(tetoImageReserve)(mem,(uint32_t)size);
    require(input!=UINT32_MAX,"REI: PROGRAM RESERVE FAILED\n");
    require(V(tetoVfsReadData)(mem,hello,0,input,(uint32_t)size)==(int32_t)size,"REI: PROGRAM READ FAILED\n");
    status(croissant_native_thx_ts__tetoLoadThx(mem,0,input,(uint32_t)size),"REI: THX LOAD FAILED");
    status(K(tetoImageRelease)(mem,input,(uint32_t)size),"REI: PROGRAM RELEASE FAILED");
    rei_console_puts("REI: MIKUOS HELLO LOADED\n");
    /* The bundled fixture does not use AT_RANDOM. General userspace needs entropy. */
    static const uint8_t argv0[]="/usr/bin/hello-rv64.39";
    const uint32_t arglen=sizeof(argv0)-1, startupLen=36+arglen;
    uint32_t start=K(tetoImageReserve)(mem,startupLen);
    require(start!=UINT32_MAX,"REI: STARTUP RESERVE FAILED\n");
    M(fill)(mem,start,startupLen,0);
    M(storeU32)(mem,start,A(TETO_STARTUP_MAGIC));
    M(storeU32)(mem,start+4,1);
    M(storeU32)(mem,start+36-4,arglen);
    copy_bytes(mem,start+36,argv0,arglen);
    status(croissant_native_start_ts__tetoBuildInitialStack(mem,0,start,startupLen,1024*1024),"REI: STARTUP FAILED");
    status(K(tetoImageRelease)(mem,start,startupLen),"REI: STARTUP RELEASE FAILED");
    bool wrote = false;
    for(unsigned batch=0;batch<1024;batch++){
        rei_console_poll();
        uint64_t tick, freq;
        __asm__ volatile("mrs %0, cntpct_el0":"=r"(tick));
        __asm__ volatile("mrs %0, cntfrq_el0":"=r"(freq));
        int32_t result=K(tetoRunRv64Batch)(mem,0,1024,(int64_t)((tick/(freq?freq:1))*1000000+(tick%(freq?freq:1))*1000000/(freq?freq:1)),0);
        if(result==A(TETO_BATCH_HOST)){
            int32_t op=K(tetoHostOperation)(mem,0), fd=K(tetoHostDescriptor)(mem,0);
            uint32_t n=K(tetoHostLength)(mem,0);
            uint64_t addr=K(tetoHostAddress)(mem,0);
            require(op==A(TETO_HOST_WRITE)&&(fd==1||fd==2)&&n<=65536,"REI: UNSUPPORTED HOST REQUEST\n");
            for(uint32_t i=0;i<n;i++){
                int32_t ch=K(guestLoadU8)(mem,0,addr+i);
                require(ch>=0&&ch<=255,"REI: GUEST READ FAILED\n");
                rei_console_char((char)ch);
            }
            if(n) wrote = true;
            status(K(tetoResumeHost)(mem,0,n),"REI: HOST RESUME FAILED");
        }else if(result==A(TETO_BATCH_EXITED)){
            status(K(tetoHartExitCode)(mem,0),"REI: GUEST EXITED NONZERO");
            require(wrote,"REI: HELLO DID NOT WRITE\n");
            rei_console_puts("REI: MIKUOS HELLO EXIT PASS\n");
            return;
        }else if(result!=A(TETO_BATCH_BUDGET)){
            rei_console_puts("REI: GUEST STOPPED status=");
            rei_console_number((uint32_t)result);
            rei_console_puts(" pc=");rei_console_number(K(tetoHartPc)(mem,0));
            rei_panic("\n");
        }
    }
    rei_panic("REI: GUEST INSTRUCTION LIMIT\n");
}
void rei_native_start(void){
    rei_console_init();
#ifdef REI_PI5
    rei_console_puts("REI: RASPI5 AARCH64 BOOT\n");
#else
    rei_console_puts("REI: RASPI3B AARCH64 BOOT\n");
#endif
    rei_console_puts("REI: INITIALISING GENERATED GLOBALS\n");
    croissant_init_globals();
    rei_console_puts("REI: GLOBALS READY\n");
    croissant_native_memory_ts__TetoMemory memory=M(directMemory)(64u*1024*1024,false);
    rei_console_puts("REI: ARENA READY\n");
    status(K(tetoKernelInit)(memory,1,false),"REI: KERNEL INIT FAILED");
    require(K(tetoKernelValid)(memory),"REI: KERNEL VALIDATION FAILED\n");
    rei_console_puts("REI: CANONICAL KERNEL INIT PASS\n");
#ifdef REI_ROOT_SD
    uint32_t size = 0;
    rei_console_puts("REI: LOADING SD ROOT.VFS\n");
    require(rei_sd_root_open(&size) == 0, "REI: SD ROOT OPEN FAILED\n");
    uint32_t at = K(tetoImageReserve)(memory, size);
    if (at == UINT32_MAX) {
        rei_sd_root_close();
        rei_panic("REI: ROOT RESERVE FAILED\n");
    }
    uint8_t chunk[4096];
    for (uint32_t offset = 0; offset < size;) {
        uint32_t remaining = size - offset;
        uint32_t length = remaining < sizeof chunk ? remaining : sizeof chunk;
        if (rei_sd_root_read(chunk, length) != 0) {
            rei_sd_root_close();
            rei_panic("REI: SD ROOT READ FAILED\n");
        }
        copy_bytes(memory, at + offset, chunk, length);
        offset += length;
        rei_console_poll();
    }
    require(rei_sd_root_close() == 0, "REI: SD ROOT CLOSE FAILED\n");
    rei_console_puts("REI: SD ROOT READ PASS\n");
#else
    size_t size=(uintptr_t)rei_root_end-(uintptr_t)rei_root_start;
    require(size>0&&size<=16u*1024*1024,"REI: EMBEDDED ROOT SIZE INVALID\n");
    uint32_t at=K(tetoImageReserve)(memory,(uint32_t)size);
    require(at!=UINT32_MAX,"REI: ROOT RESERVE FAILED\n");
    copy_bytes(memory,at,rei_root_start,(uint32_t)size);
#endif
    status(V(tetoLoadVfs)(memory,at,(uint32_t)size),"REI: ROOT MOUNT FAILED");
    status(K(tetoImageRelease)(memory,at,(uint32_t)size),"REI: ROOT RELEASE FAILED");
    require(V(tetoVfsLoaded)(memory),"REI: ROOT NOT MOUNTED\n");
    rei_console_puts("REI: MIKUOS ROOT MOUNT PASS\n");
    rei_console_puts("REI: VFS INODES ");rei_console_number(V(tetoVfsInodeCount)(memory));rei_console_puts("\n");
    run_hello(memory,V(tetoVfsRoot)(memory));
    rei_console_idle();
    for(;;)__asm__ volatile("wfe");
}
