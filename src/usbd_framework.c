#include "usbd_framework.h"

void USBD_Init() {
    usbdPinsInit(); //Inicializa pinos
    usbdCoreInit(); //Inicializa Core
    connectDevice(); //conecta dispositivo
}

