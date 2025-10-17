#include <iostream>
#include <sys/random.h>
using namespace std;

#include "fastRand64ProMax.cpp"

/*main.cpp*/
int main() {
    seed();
    size_t d20[21] = {0};
    for(size_t i = 0; i < 1'000'000'000; i++) {
        d20[rand_m(20)]++;
    }

    for (int i=1; i<21; i++) {
        std::cout << i << ": " << d20[i] << "\n";
    }

}