#include "usbd_framework.h"

/*Adicionando variavel global para ser usada em todas as funções nesse arquivo,
 assim não precisa colocar esse parametro em todas as funções é um ponteiro */
static UsbDeviceType *usb_handle;

void printSetupData(uint8_t *buffer, size_t byte_count) {
    printf("SETUP data: ");
    for (size_t i = 0; i < byte_count; i++) {
        printf("%02X ", buffer[i]); // Exibe cada byte em hexadecimal com 2 dígitos
    }
    printf("\n");
}


void UsbdInit(UsbDeviceType *usb_device) {
    usb_handle = usb_device;
    usb_driver.usbdPinsInit(); //Inicializa pinos
    usb_driver.usbdCoreInit(); //Inicializa Core
    usb_driver.connectDevice(); //conecta dispositivo
}

//Função escanear mudanças nas flags de interrupção 
void UsbdPoll() {
    usb_driver.poll();
}

static void processStandardDeviceReq() {
    UsbDeviceRequest const *request = usb_handle->ptr_out_buffer;
    //Analisar qual request específico o device recebeu 
    switch(request->bRequest) {
        //Request GetDescriptor o host pede por um descritor de tipo específico
        case GET_DESCRIPTOR:;
            //primeiros 8 bits do campo wValue são o valor do tipo de descriptor
            const uint8_t descriptor_type = request->wValue >> 8;
            //Pegar tamanho em bytes dos dados que serão transferidos no data stage se houver
            const uint16_t length = request->wLength;
            switch(descriptor_type) {
                case USB_DESCRIPTOR_TYPE_DEVICE:
                //ponteiro para buffer onde vai ser transmitido dados do TxFIFO para host
                    usb_handle->ptr_in_buffer = &device_descriptor;
                //Tamanho
                    usb_handle->in_data_size = length;
                //Altera estágio para DATA IN
                    usb_handle->usb_control_stage = USB_CONTROL_STAGE_DATA_IN;
                    break;
            }
            //TODO
            break;
    }
}

static void processControlTransferStage() {
    switch(usb_handle->usb_control_stage) {
        //Não fazer no estágio setup
        case USB_CONTROL_STAGE_SETUP:
            break;
        //Estágio para enviar resposta para host de requests
        case USB_CONTROL_STAGE_DATA_IN:;
        //escrevendo dados do txfifo para o endpoint 0
        //Buffer pode ter tamanho maior que o tamanho máximo por pacotes
            uint8_t data_size = device_descriptor.bMaxPacketSize0;
            usb_driver.writePacket(0, usb_handle->ptr_in_buffer, (uint8_t) data_size);
        //subtraindo size do buffer pelo número de dados escritos
            usb_handle->in_data_size -= data_size;
        //incrementa pointer do buffer pelo número de dados escritos
            usb_handle->ptr_in_buffer += data_size;
        //estágio intermediário para evitar de sempre ser escrito no buffer quando a função é chamada
            usb_handle->usb_control_stage = USB_CONTROL_STAGE_DATA_IN_IDLE;
            break;
        //esse estágio não faz nada só espera pelo host ler os dados enviados no endpoint 0
        case USB_CONTROL_STAGE_DATA_IN_IDLE:
            break;
    }
}

//Função para processar request usb
static void processRequest() {
    //Variavel para apontar para o buffer onde foi pego os dados do RxFifo que veio do host
    UsbDeviceRequest const *request = usb_handle->ptr_out_buffer;
    //checar qual tipo de request recebeu e quem vai receber ele
    switch(request->bmRequestType & (USB_BM_REQUEST_TYPE_MSK | USB_BM_REQUEST_RECIPIENT_MSK)) {
        //Para Standard device request
        case (USB_BM_REQUEST_RECIPIENT_DEVICE | USB_BM_REQUEST_TYPE_STANDARD):
            processStandardDeviceReq();
            break;

    }
}

//Handler reset
static void resetHandler() {
    //Reseta todos os valores na estrutura do dispositivo
    usb_handle->in_data_size = 0;
    usb_handle->out_data_size = 0;
    usb_handle->config_value = 0;
    usb_handle->usb_device_state = USB_DEVICE_STATE_DEFAULT;
    usb_handle->usb_device_state = USB_CONTROL_STAGE_SETUP;
    //Seta endereço para 0, toda comunicação USB começa com endereço sendo 0
    usb_driver.setDeviceAddress(0);
}

static void usbPolledHandler() {
    //sempre que for fazer polling no bus ver quais mudanças de estado teve
    processControlTransferStage();
}

static void setupDataReceived(uint8_t endpoint_number, uint16_t byte_count) {
    //Vai ler o conteudo do RxFifo e guardar em um buffer usando função implementada no driver
    usb_driver.readPacket(usb_handle->ptr_out_buffer, byte_count);
    printSetupData(usb_handle->ptr_out_buffer, byte_count);
    //Chama função para tratar de como vai processar o request lido no buffer
    processRequest();
}

//Variavel com ponteiros para as funções implementadas nesse arquivo
//Mas os Handlers vão ser chamadas no driver
UsbEvents usb_events = {
    .usbResetReceived = &resetHandler,
    .setupDataReceived = &setupDataReceived,
    .usbPolled = &usbPolledHandler
    //TODO
};