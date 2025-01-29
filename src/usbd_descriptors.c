#include "usbd_descriptors.h"

//Criando descritor
UsbDeviceDescriptor device_descriptor = {
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