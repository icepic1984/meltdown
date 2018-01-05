#include <iostream>
#include <vector>

const unsigned long threshold = 100;

bool probe(uint8_t* addr, unsigned long threshold) {
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
    
    return time < threshold;
}


int main(int argc, char *argv[])
{
    
    std::vector<uint8_t> data(10);
    for(int i = 0; i < 1000; ++i){
        
        data[0] = 0;
        std::cout <<probe(&data[0], threshold) << std::endl;
        std::cout <<probe(&data[0], threshold) << std::endl;
    }
    return 0;
}
