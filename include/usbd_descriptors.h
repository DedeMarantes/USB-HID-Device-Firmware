#ifndef USBD_DESCRIPTORS_H
#define USBD_DESCRIPTORS_H

#ifdef __cplusplus
extern "C" {
#endif
#include "usb_config_types.h"

typedef struct {
    uint8_t bLength; //Tamanho do descriptor em bytes
    uint8_t bDescriptorType; //Device descriptor type
    uint16_t bcdUsb; //release da especificação usb
    uint8_t bDeviceClass; //Código da classe do dispositivo
    uint8_t bDeviceSubClass; //Código da subclasse do dispositivo
    uint8_t bDeviceProtocol; //Código de protocolo
    uint8_t bMaxPacketSize0; //tamanho máximo do pacote para EN0
    uint16_t idVendor; //ID do vendedor
    uint16_t idProduct; //ID do produto
    uint16_t bcdDevice; //Número do release do dispositivo em binary coded decimal
    uint8_t iManufacturer; //Indice do string descriptor que descreve o fabricante
    uint8_t iProduct; //Indice do string descriptor que descreve o produto
    uint8_t iSerialNumber; //Indice do string descriptor que descreve o serial number do device
    uint8_t bNumConfigurations; //Número de configurações possíveis
} UsbDeviceDescriptor;

extern UsbDeviceDescriptor device_descriptor;


#ifdef __cplusplus
}
#endif

#endif