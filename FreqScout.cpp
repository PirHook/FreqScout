#include <iostream>
#include "hdr/utils/terminalUtil.h"

int main(int argc, char* argv[])
{
    while(1){
        std::cout << "ITERATING_1\n";
        std::cout << exec("rtl_test").first << std::endl;
        std::cout << "ITERATING_2\n";
    }
}
