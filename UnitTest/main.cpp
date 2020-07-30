#include <iostream>

#include "unittest.hpp"
using namespace shochu;

int add(int a,int b){
    return a + b;
}

int main(){
    for(int i=0;i<10;++i){
        Expect_EQ(add(i,i),2*i);
    }
    Run_All_TestCase();

    return 0;
}