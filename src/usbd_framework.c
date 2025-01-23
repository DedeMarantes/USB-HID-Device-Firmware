#include "usbd_framework.h"


void UsbdInit() {
    usb_driver.usbdPinsInit(); //Inicializa pinos
    usb_driver.usbdCoreInit(); //Inicializa Core
    usb_driver.connectDevice(); //conecta dispositivo
}

void UsbdPoll() {
    usb_driver.poll();
}
