#ifndef USBD_FRAMEWORK_H
#define USBD_FRAMEWORK_H

//Checar se for usar c++ e poder usar esse cabeçalho
#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_driver.h"
#include "stdio.h"
#include "usbd_descriptors.h"

void UsbdInit(UsbDeviceType *usb_device);
void UsbdPoll();


#ifdef __cplusplus
}
#endif

#endif