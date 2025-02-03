#include "usbd_framework.h"

/*Adicionando variavel global para ser usada em todas as funções nesse arquivo,
 assim não precisa colocar esse parametro em todas as funções é um ponteiro */
static UsbDeviceType *usb_handle;

void printSetupData(const uint8_t *buffer, size_t byte_count) {
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

void usbdConfigure() {
    //Configurar endpoint com base nos descritores enviados para o host
    usb_driver.inEndpointConfig(
        (configuration_descriptor.usb_endpoint_descriptor.bEndpointAddress & 0x0F), 
        (configuration_descriptor.usb_configuration_descriptor.bmAttributes & 0x03),
        configuration_descriptor.usb_endpoint_descriptor.wMaxPacketSize
         );
    //depois de configurado enviar pacote vazio para host nesse endpoint conforme manual referência
    usb_driver.writePacket(
        (configuration_descriptor.usb_endpoint_descriptor.bEndpointAddress & 0x0F),
        NULL, 0);
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
            //Últimos 8 bits são o indice do descritor
            //const uint8_t descriptor_index = request->wValue & 0xFF;
            switch(descriptor_type) {
                case USB_DESCRIPTOR_TYPE_DEVICE:
                //ponteiro para buffer onde vai ser transmitido dados do TxFIFO para host
                    usb_handle->ptr_in_buffer = &device_descriptor;
                //Tamanho
                    usb_handle->in_data_size = length;
                //Altera estágio para DATA IN
                    usb_handle->usb_control_stage = USB_CONTROL_STAGE_DATA_IN;
                    break;
                case USB_DESCRIPTOR_TYPE_CONFIGURATION:
                //ponteiro para buffer onde vai ser transmitido dados do TxFIFO para host
                /*Esse descritor terá combinado:
                Configuration descriptor
                Interface descriptor
                Endpoint descriptor 
                HID descriptor
                todos eles serão enviados de uma vez
                */
                    usb_handle->ptr_in_buffer = &configuration_descriptor;
                //Tamanho
                    usb_handle->in_data_size = length;
                //Altera estágio para DATA IN
                    usb_handle->usb_control_stage = USB_CONTROL_STAGE_DATA_IN;
                    break;
            }
            break;
        case SET_ADDRESS:;
        //extraindo valor do endereço definido pelo host no request
            const uint16_t device_address = request->wValue;
        //aplicando função no driver para setar endereço no dispositivo
            usb_driver.setDeviceAddress((uint8_t) device_address);
        //Altera estado de default para addressed
            usb_handle->usb_device_state = USB_DEVICE_STATE_ADRESSED;
        //Estágio status sempre oposto ao de data stage então como é uma transferência out status é IN
            usb_handle->usb_control_stage = USB_CONTROL_STAGE_STATUS_IN;
            break;
        case SET_CONFIGURATION:;
        //Setando valor da configuração que é recebido no request
            usb_handle->config_value = request->wValue;
            usbdConfigure();
        //Troca estado de addressed para configured
            usb_handle->usb_device_state = USB_DEVICE_STATE_CONFIGURED;
            usb_handle->usb_control_stage = USB_CONTROL_STAGE_STATUS_IN;
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
        //Se tamanho for menor que  o tamanho máximo do pacote é porque é o ultimo a ser escrito
            uint8_t data_size = MIN(usb_handle->in_data_size,device_descriptor.bMaxPacketSize0);
            usb_driver.writePacket(0, usb_handle->ptr_in_buffer, (uint8_t) data_size);
        //subtraindo size do buffer pelo número de dados escritos
            usb_handle->in_data_size -= data_size;
        //incrementa pointer do buffer pelo número de dados escritos
            usb_handle->ptr_in_buffer += data_size;
        //estágio intermediário para evitar de sempre ser escrito no buffer quando a função é chamada
            usb_handle->usb_control_stage = USB_CONTROL_STAGE_DATA_IN_IDLE;
            //Se tamanho for 0 depois alterado o tamanho 
            //ou seja se o tamanho so buffer restante for igual ao tamanho máximo do pacote 
            if(usb_handle->in_data_size == 0) {
                if(data_size == device_descriptor.bMaxPacketSize0) {
                    usb_handle->usb_control_stage = USB_CONTROL_STAGE_DATA_IN_ZERO;
                }
                //Se tamanho restante for menor que o máximo acaba a transferência
                else {
                    usb_handle->usb_control_stage = USB_CONTROL_STAGE_STATUS_OUT;
                }
            }
            break;
        //esse estágio só volta para o setup stage para esperar por mais pacotes
        case  USB_CONTROL_STAGE_STATUS_OUT:
            usb_handle->usb_control_stage = USB_CONTROL_STAGE_SETUP;
            break;
        //esse estágio não faz nada só espera pelo host ler os dados enviados no endpoint 0
        case USB_CONTROL_STAGE_DATA_IN_IDLE:
            break;
        case USB_CONTROL_STAGE_STATUS_IN:
        //envia pacote de dados de tamanho 0
            usb_driver.writePacket(0, NULL, 0);
            usb_handle->usb_control_stage = USB_CONTROL_STAGE_SETUP;
            break;
    }
}

//Processar requests específicos de classe
static void processClassInterfaceReq() {
    UsbDeviceRequest const *request = usb_handle->ptr_out_buffer;
    //Analisar qual request específico a interface recebeu
    switch(request->bRequest) {
        //Set idle request
        case HID_SET_IDLE:
            usb_handle->usb_control_stage = USB_CONTROL_STAGE_STATUS_IN;
            break;
    }
}

//Função para lidar com request standard para uma interface
static void processStandardInterfaceReq() {
    UsbDeviceRequest const *request = usb_handle->ptr_out_buffer;
    switch(request->wValue >> 8) {
        case USB_DESCRIPTOR_TYPE_HID_REPORT:
            usb_handle->ptr_in_buffer = &hid_report_descriptor;
            usb_handle->in_data_size = hid_report_descriptor_size;
            usb_handle->usb_control_stage = USB_CONTROL_STAGE_DATA_IN;
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
        //Para request específico da classe que no caso é HID 
        //que vai ser só para uma interface específica e não pro device
        case (USB_BM_REQUEST_TYPE_CLASS | USB_BM_REQUEST_RECIPIENT_INTERFACE):
            processClassInterfaceReq();
            break;
        case (USB_BM_REQUEST_TYPE_STANDARD | USB_BM_REQUEST_RECIPIENT_INTERFACE):
            processStandardInterfaceReq();
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

static void writeMouseReport() {
    HID_Report hid_report = {
        .x_value = 5,
    };
    usb_driver.writePacket(
        (configuration_descriptor.usb_endpoint_descriptor.bEndpointAddress & 0x0F),
        &hid_report,
        sizeof(hid_report)
    );
}

static void inTransferCompleteHandler(uint8_t endpoint_number) {
    //Se ainda tem dados para ser transmitidos para o host
    if(usb_handle->in_data_size) {
        //Trocar estágio para voltar a escrever pacotes para envio para o host
        usb_handle->usb_control_stage = USB_CONTROL_STAGE_DATA_IN;
    }
    //conferir se estiver estágio DATA_IN_ZERO e apenas escrever um pacote vazio para encerrar transferência
    else if(usb_handle->usb_control_stage == USB_CONTROL_STAGE_DATA_IN_ZERO) {
        usb_driver.writePacket(0, NULL, 0);
        usb_handle->usb_control_stage = USB_CONTROL_STAGE_STATUS_OUT;
    }
    //Checar se endpoint é o de número 3 onde vai ser feito transferência de dados
    if(endpoint_number == (configuration_descriptor.usb_endpoint_descriptor.bEndpointAddress & 0x0F)) {
        writeMouseReport(); //Envia report do mouse
    }
}

static void outTransferCompleteHandler(uint8_t endpoint_number) {}

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
    .usbPolled = &usbPolledHandler,
    .inTransferComplete = &inTransferCompleteHandler,
    .outTransferComplete = &outTransferCompleteHandler
    //TODO
};