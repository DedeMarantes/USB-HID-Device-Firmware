#ifndef USB_TYPES_H
#define USB_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

//Tipos de transferência USB
typedef enum {
    USB_TRANSFER_TYPE_CONTROL,
    USB_TRANSFER_TYPE_ISOCHRONOUS,
    USB_TRANSFER_TYPE_BULK,
    USB_TRANSFER_TYPE_INTERRUPT
} UsbTransferType;


#ifdef __cplusplus
}
#endif
#endif