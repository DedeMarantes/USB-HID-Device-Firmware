#ifndef USB_TYPES_H
#define USB_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "usbd_descriptors.h"
//Tipos de transferência USB
typedef enum {
    USB_TRANSFER_TYPE_CONTROL,
    USB_TRANSFER_TYPE_ISOCHRONOUS,
    USB_TRANSFER_TYPE_BULK,
    USB_TRANSFER_TYPE_INTERRUPT
} UsbTransferType;

//Estrutura para lidar com eventos, chamadas na camada de driver porém implementado na camada framework
typedef struct {
    void (*usbResetReceived) ();
    void (*setupDataReceived) (uint8_t endpoint_number, uint16_t byte_count);
    void (*outDataReceived) (uint8_t endpoint_number, uint16_t byte_count);
    void (*inDataReceived) (uint8_t endpoint_number);
    void (*inTransferComplete) (uint8_t endpoint_number);
    void (*outTransferComplete) (uint8_t endpoint_number);
    void (*usbPolled) ();
} UsbEvents;

//Tipo de estado do dispositivo usb
typedef enum {
    USB_DEVICE_STATE_DEFAULT,
    USB_DEVICE_STATE_ADRESSED,
    USB_DEVICE_STATE_CONFIGURED,
    USB_DEVICE_STATE_SUSPENDED
} UsbDeviceState;

//Definindo estágios da transferência
typedef enum {
    USB_CONTROL_STAGE_SETUP, //Pode ser chamado também de USB_CONTROL_STAGE_IDLE
    USB_CONTROL_STAGE_DATA_OUT,
    USB_CONTROL_STAGE_DATA_IN,
    USB_CONTROL_STAGE_DATA_IN_IDLE,
    USB_CONTROL_STAGE_DATA_IN_ZERO,
    USB_CONTROL_STAGE_STATUS_OUT,
    USB_CONTROL_STAGE_STATUS_IN
} UsbControlTransferStage;

//Struct para dispositivo USB
typedef struct {
    //Estado atual do dispositivo
    UsbDeviceState usb_device_state;
    //Estágio atual de transferência no endpoint 0 de controle
    UsbControlTransferStage usb_control_stage;
    //Configuração usb selecionada
    uint8_t config_value;
    //Ponteiros para buffers IN e OUT
    void *ptr_out_buffer;
    uint32_t out_data_size; 
    void *ptr_in_buffer;
    uint32_t in_data_size;
} UsbDeviceType;

//Estrutura para representar os campos de um request do tipo device seguindo os padrões da especificação USB
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} UsbDeviceRequest;

//Constantes para definir tipos de request do bmRequestType que é um bitmap campos D[4-2] são reservados
//Campo direction só tem um bit (D[7]) e indica direção da comunicação do device para o host ou vice e versa
#define USB_BM_REQUEST_DIRECTION_MSK (1 << 7)
#define USB_BM_REQUEST_DIRECTION_TODEVICE (0 << 7)
#define USB_BM_REQUEST_DIRECTION_TOHOST (1 << 7)

//Campo type (2 bits D[6-5]) define se vai ser do tipo standard, class ou vendor specific
#define USB_BM_REQUEST_TYPE_MSK (3 << 5)
#define USB_BM_REQUEST_TYPE_STANDARD (0 << 5)
#define USB_BM_REQUEST_TYPE_CLASS (1 << 5)
#define USB_BM_REQUEST_TYPE_VENDOR (2 << 5)

//campo recipient (2 bits D[1-0]) indica se o request se refere ao device, a um endpoint ou a uma interface
#define USB_BM_REQUEST_RECIPIENT_MSK (3 << 0)
#define USB_BM_REQUEST_RECIPIENT_DEVICE (0 << 0)
#define USB_BM_REQUEST_RECIPIENT_INTERFACE (1 << 0)
#define USB_BM_REQUEST_RECIPIENT_ENDPOINT (2 << 0)
#define USB_BM_REQUEST_RECIPIENT_OTHER (3 << 0)

//Constantes para definir os tipos de standard device requests mais comuns
#define GET_STATUS 0x00
#define CLEAR_FEATURE 0x01
#define SET_FEATURE 0x03
#define SET_ADDRESS 0x05
#define GET_DESCRIPTOR 0x06
#define SET_DESCRIPTOR 0x07
#define GET_CONFIGURATION 0x08
#define SET_CONFIGURATION 0x09
#define GET_INTERFACE 0x0A
#define SET_INTERFACE 0x0B


// Constantes para definir tipos de descriptors
#define USB_DESCRIPTOR_TYPE_DEVICE 0x01
#define USB_DESCRIPTOR_TYPE_CONFIGURATION 0x02
#define USB_DESCRIPTOR_TYPE_STRING 0x03
#define USB_DESCRIPTOR_TYPE_INTERFACE 0x04
#define USB_DESCRIPTOR_TYPE_ENDPOINT 0x05
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER 0x06
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION 0x07
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER 0x08

// Constantes para definir códigos das classes USB
#define USB_CLASS_PER_INTERFACE 0x00       // Class code for a device that uses a class specification interface
#define USB_CLASS_AUDIO 0x01       // Audio devices
#define USB_CLASS_COMM 0x02        // Communications and CDC Control devices
#define USB_CLASS_HID 0x03         // Human Interface Devices (keyboards, mice, etc.)
#define USB_CLASS_PHYSICAL 0x05    // Physical devices
#define USB_CLASS_IMAGE 0x06       // Imaging devices
#define USB_CLASS_PRINTER 0x07     // Printers
#define USB_CLASS_STORAGE 0x08     // Mass Storage devices
#define USB_CLASS_HUB 0x09         // USB Hubs
#define USB_CLASS_CDC_DATA 0x0A    // CDC Data
#define USB_CLASS_SMART_CARD 0x0B  // Smart Card devices
#define USB_CLASS_CONTENT_SEC 0x0D // Content Security devices
#define USB_CLASS_VIDEO 0x0E       // Video devices
#define USB_CLASS_PERSONAL_HEALTHCARE 0x0F // Personal Healthcare devices
#define USB_CLASS_AUDIO_VIDEO 0x10 // Audio/Video devices
#define USB_CLASS_BILLBOARD 0x11   // Billboard devices
#define USB_CLASS_USB_TYPE_C_BRIDGE 0x12 // USB Type-C Bridge devices
#define USB_CLASS_DIAGNOSTIC 0xDC  // Diagnostic devices
#define USB_CLASS_WIRELESS_CTRL 0xE0 // Wireless Controller devices
#define USB_CLASS_MISC 0xEF        // Miscellaneous devices
#define USB_CLASS_APP_SPECIFIC 0xFE // Application-Specific devices
#define USB_CLASS_VENDOR_SPECIFIC 0xFF // Vendor-Specific devices


//Constantes 
#define USB_SUBCLASS_NONE 0x00
#define USB_SUBCLASS_VENDOR 0xFF
#define USB_PROTOCOL_NONE 0x00
#define USB_PROTOCOL_IAD 0x01
#define USB_PROTOCOL_VENDOR 0xFF

#ifdef __cplusplus
}
#endif
#endif