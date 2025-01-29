#ifndef USBD_DRIVER_H
#define USBD_DRIVER_H

//Checar se for usar c++ e poder usar esse cabeçalho
#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f746xx.h"
#include "usb_config_types.h"
#include "strings.h"
//Pelo datasheet o endereço base do USB OTG na velocidade high speed começa no endereço 0x40040000 
//Assim a parte global começa na base + offset base que é 0
#define USB_OTG_HS_GLOBAL ((USB_OTG_GlobalTypeDef*) (USB_OTG_HS_PERIPH_BASE + USB_OTG_GLOBAL_BASE))
//endereço do usb device é base + offset do registrador do device
//Ponteiro para estrutura de registradores que começa no endereço 0x48040000
#define USB_OTG_HS_DEVICE ((USB_OTG_DeviceTypeDef*) (USB_OTG_HS_PERIPH_BASE + USB_OTG_DEVICE_BASE))
//Da parde de power e clock gating segue a mesma lógica começa no endereço 0x4E040000
//Ponteiro para variavel de 32 bits inteiro, só tem registrador relacionado a essa configuração e ele tem 32 bits de tamanho
#define USB_OTG_HS_PCGCCTL ((uint32_t*) (USB_OTG_HS_PERIPH_BASE + USB_OTG_PCGCCTL_BASE))

//esse microcontrolador tem 9 Endpoints no total IN ou OUT
#define ENDPOINT_COUNT 9
//Tamanho máximo do pacote para full speed devices é 64 bytes
#define MAX_PACKET_SIZE 64
//constante para posições endpoint 0 IN e OUT no registrador de interrupção mask
#define USB_OTG_DAINTMSK_INEN0 (1 << 0)
#define USB_OTG_DAINTMSK_OUTEN0 (1 << 16)
//Definir constantes para status pacotes
#define STATUS_SETUP_PKT_RCVD 0x06 
#define STATUS_OUT_PKT_RCVD 0x02
#define STATUS_SETUP_PKT_COMPLETED 0x04
#define STATUS_OUT_PKT_COMPLETED 0x03

//Funções para calcular estrutura de registradores de configuração de endpoints IN e OUT
//Função apenas retorna o ponteiro para a posição na memória onde está o registrador
/*Registradores em cada endpoint x ficam 32 bits (0x20) de espaço entre eles 
então para acessar registrador x é só fazer endpoint_x * 0x20
*/
inline static USB_OTG_INEndpointTypeDef* IN_ENDPOINT(uint8_t endpoint_number) {
    return (USB_OTG_INEndpointTypeDef*) (USB_OTG_HS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + (endpoint_number * 0x20));
}

inline static USB_OTG_OUTEndpointTypeDef* OUT_ENDPOINT(uint8_t endpoint_number) {
    return (USB_OTG_OUTEndpointTypeDef*) (USB_OTG_HS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + (endpoint_number * 0x20));
}

//Função para acessar localização do endereço de memória do Fifo
//tamanho do fifo definido por words(32 bits)
//cada região na memória de cada TxFifo é separado por 0x1000
inline static __IO uint32_t* FIFO(uint8_t endpoint_number) {
    return (__IO uint32_t*) (USB_OTG_HS_PERIPH_BASE + USB_OTG_FIFO_BASE + (endpoint_number * 0x1000));
}

//Macro para auxilio configuração para IN e OUT endpoint x nos registradores
#define IN_EP_REG_CONFIG(NUMBER, SIZE, TYPE) USB_OTG_DIEPCTL_USBAEP | _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, SIZE) \
        | USB_OTG_DIEPCTL_SNAK | _VAL2FLD(USB_OTG_DIEPCTL_EPTYP, TYPE) | USB_OTG_DIEPCTL_SD0PID_SEVNFRM \
        | _VAL2FLD(USB_OTG_DIEPCTL_TXFNUM, NUMBER)

#define OUT_EP_REG_CONFIG(SIZE, TYPE) USB_OTG_DOEPCTL_USBAEP | _VAL2FLD(USB_OTG_DOEPCTL_MPSIZ, SIZE) \
        | USB_OTG_DOEPCTL_SNAK | _VAL2FLD(USB_OTG_DOEPCTL_EPTYP, TYPE) | USB_OTG_DOEPCTL_SD0PID_SEVNFRM
        
//Funções do driver que vão ser expostas para a camada superior
typedef struct {
    void (*usbdPinsInit)();
    void (*usbdCoreInit)();
    void (*connectDevice)();
    void (*disconnectDevice)();
    void (*readPacket)(void *buffer, uint16_t size);
    void (*writePacket)(uint16_t endpoint_number, void const *buffer, uint16_t size);
    void (*flushRxFifo)();
    void (*flushTxFifo)(uint8_t endpoint_number);
    void (*inEndpointConfig)(uint8_t endpoint_number, UsbTransferType endpoint_type, uint16_t endpoint_size);
    void (*poll)(); //Função para escanear o barramento com mudanças nos registradores de interrupts
    void (*setDeviceAddress) (uint8_t address);
    //TODO adicionar ponteiros para as outras funções do driver
} UsbDriver;

//instância para chamar as funções do driver
extern const UsbDriver usb_driver;
//Variavel para tratar os eventos, que é ativado pela camada de driver e o handler é implementado na camada framework
extern UsbEvents usb_events;

#ifdef __cplusplus
}
#endif

#endif