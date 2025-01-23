
#include <stdint.h>
#include "sys_init.h"
#include "usbd_framework.h"



int main() {
    configureMCO1();
    configureClock();
    UsbdInit(); // Inicializa USB
    while(1) {
        //Fazer poll no barramento para localizar mudanças
        UsbdPoll();
    }
}



