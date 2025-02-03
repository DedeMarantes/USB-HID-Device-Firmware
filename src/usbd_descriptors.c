#include "usbd_descriptors.h"

//Criando device descriptor desse dispositivo
const UsbDeviceDescriptor device_descriptor = {
    .bLength = sizeof(UsbDeviceDescriptor),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_DEVICE,
    .bcdUsb = 0x0200,
    .bDeviceClass = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass = USB_SUBCLASS_NONE,
    .bDeviceProtocol = USB_PROTOCOL_NONE,
    .bMaxPacketSize0 = 8,
    .idVendor = 0x6666, //Vendor id para prototipo
    .idProduct = 0x13AA,
    .bcdDevice = 0x0100,
    .iManufacturer = 0,
    .iProduct = 0,
    .iSerialNumber = 0,
    .bNumConfigurations = 1
};

//Report descriptor para um mouse
const uint8_t hid_report_descriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (Button 1)
    0x29, 0x03, //     Usage Maximum (Button 3)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x95, 0x03, //     Report Count (3)
    0x75, 0x01, //     Report Size (1)
    0x81, 0x02, //     Input (Data, Variable, Absolute)
    0x95, 0x01, //     Report Count (1)
    0x75, 0x05, //     Report Size (5)
    0x81, 0x01, //     Input (Constant) for padding
    0x05, 0x01, //     Usage Page (Generic Desktop)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x02, //     Report Count (2)
    0x81, 0x06, //     Input (Data, Variable, Relative)
    0xC0,       //   End Collection
    0xC0        // End Collection
};

const UsbConfigurationFullPacket configuration_descriptor = {
//Criando configuration descriptor do dispositivo nesse dispositivo só tem uma configuração
    .usb_configuration_descriptor = {
        .bLength = sizeof(UsbConfigurationDescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength = sizeof(UsbConfigurationFullPacket),
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = USB_CONFIG_ATTR_BUS_PWRD,
        .bMaxPower = 25 //cada valor é 2mA então 25 é 50mA
    },
    //Criando Interface Descriptor para ser enviado para o host
    .usb_interface_descriptor = {
        .bLength = sizeof(UsbInterfaceDescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber = 1,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CLASS_HID,
        .bInterfaceSubClass = USB_SUBCLASS_NONE,
        .bInterfaceProtocol = USB_PROTOCOL_NONE,
        .iInterface = 0
    },
    //Criando endpoint descriptor para ser enviado
    .usb_endpoint_descriptor = {
        .bLength = sizeof(UsbEndpointDescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = 0x83, // Endpoint 3 IN
        .bmAttributes = (uint8_t) USB_TRANSFER_TYPE_INTERRUPT,
        .wMaxPacketSize = 64,
        .bInterval = 50 // Intervalo de 50 frames de polling no endpoint
    },
    //Criando o HID descriptor para o host
    .usb_hid_descriptor = {
        .bLength = sizeof(UsbHidDescriptor),
        .bDescriptorType = USB_DESCRIPTOR_TYPE_HID,
        .bcdHID = 0x0112, //versão hid 1.12
        .bCountryCode = 0,
        .bNumDescriptors = 1, // 1 report descriptor
        .bDescriptorType2 = 0x22, //tipo 0x22 = Report descriptor
        .wDescriptorLength = sizeof(hid_report_descriptor)
    }
};