#ifndef USBD_DESCRIPTORS_H
#define USBD_DESCRIPTORS_H

#ifdef __cplusplus
extern "C" {
#endif
#include "usb_config_types.h"

typedef struct __attribute__((packed)) {
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

//Precisa evitar o padding do compilador para que seja calculado o size certo
typedef struct __attribute__((packed)) {
    uint8_t bLength; //tamanho do descritor em bytes
    uint8_t bDescriptorType; // tipo do descritor de configuração
    uint16_t wTotalLength; //Tamanho total dos dados dos descritores que vão ser enviados (endpoint, interface e configuration)
    uint8_t bNumInterfaces; //Número de interfaces
    uint8_t bConfigurationValue; //Número da configuração
    uint8_t iConfiguration; //Indice do string descriptor que descreve essa configuração
    uint8_t bmAttributes; // Características da configuração
    uint8_t bMaxPower; //Consumo máximo de corrente suportado pelo dispositivo usb
} UsbConfigurationDescriptor;

//Macros para bmAtttributes 0x80 porque o bit D[7] sempre setado para 1
#define USB_CONFIG_ATTR_SELF_PWRD (1 << 6) | 0x80 //Self powered device
#define USB_CONFIG_ATTR_REMOTE_WKUP (1 << 5) | 0x80 //Suporta wakeup remoto
#define USB_CONFIG_ATTR_BUS_PWRD 0x80 //recebe energia do barramento

typedef struct __attribute__((packed)) {
    uint8_t bLength;               // Tamanho do descritor (em bytes)
    uint8_t bDescriptorType;       // Tipo do descritor (0x04 para Interface Descriptor)
    uint8_t bInterfaceNumber;      // Número da interface
    uint8_t bAlternateSetting;     // Configuração alternativa
    uint8_t bNumEndpoints;         // Número de endpoints usados pela interface
    uint8_t bInterfaceClass;       // Classe da interface (ex: HID, CDC, etc.)
    uint8_t bInterfaceSubClass;    // Subclasse da interface
    uint8_t bInterfaceProtocol;    // Protocolo da interface
    uint8_t iInterface;            // Índice da string de descrição da interface
} UsbInterfaceDescriptor;

typedef struct __attribute__((packed)) {
    uint8_t bLength;               // Tamanho do descritor (em bytes)
    uint8_t bDescriptorType;       // Tipo do descritor (0x05 para Endpoint Descriptor)
    uint8_t bEndpointAddress;      // Endereço do endpoint (bit 7: direção, bits 3..0: número do endpoint)
    uint8_t bmAttributes;          // Atributos do endpoint (tipo de transferência)
    uint16_t wMaxPacketSize;       // Tamanho máximo do pacote
    uint8_t bInterval;             // Intervalo de polling (em ms)
} UsbEndpointDescriptor;

//Descritor específico para a classe HID e não entra como especificação do USB como os outros
typedef struct __attribute__((packed)) {
    uint8_t bLength; // Tamanho descritor em bytes
    uint8_t bDescriptorType; // tipo de descritor
    uint16_t bcdHID; // Versão da especificação HID (em formato BCD)
    uint8_t bCountryCode; // expressão numérica que representa o código do país
    uint8_t bNumDescriptors; // Número de descritores de relatório
    uint8_t bDescriptorType2; // Tipo do descritor de relatório nesse caso report descriptor
    uint16_t wDescriptorLength; // Tamanho do descritor de relatório
} UsbHidDescriptor;


typedef struct __attribute__((packed)) {
    UsbConfigurationDescriptor usb_configuration_descriptor;
    UsbInterfaceDescriptor usb_interface_descriptor;
    UsbHidDescriptor usb_hid_descriptor;
    UsbEndpointDescriptor usb_endpoint_descriptor;
} UsbConfigurationFullPacket;

extern const UsbDeviceDescriptor device_descriptor;
extern const UsbConfigurationFullPacket configuration_descriptor;
extern const uint8_t hid_report_descriptor[50];
extern const uint16_t hid_report_descriptor_size;


#ifdef __cplusplus
}
#endif

#endif