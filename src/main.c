
#include <stdint.h>
#include "sys_init.h"
#include "usbd_framework.h"

UsbDeviceType usb_device;
uint32_t buffer_received[8]; //buffer para armazenar os dados pegos no RxFifo


int main() {
    configureMCO1();
    configureClock();
    UsbdInit(&usb_device); // Inicializa USB
    //Ponteiro aponta para endereço de memória da primeira posição da variavel global do buffer de recepção
    usb_device.ptr_out_buffer = &buffer_received; 
    while(1) {
        //Fazer poll no barramento para localizar mudanças
        UsbdPoll(&usb_device);
    }
}



