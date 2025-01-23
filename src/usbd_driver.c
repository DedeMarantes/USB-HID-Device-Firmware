#include "usbd_driver.h"

static void usbdPinsInit() {
    //Habilitar clock para GPIOB
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
    //Habilitar Alternate function para USB_OTG_HS PB14 (D-) PB15 (D+)
    //Registrador AFR tem parte low(L) AFR[0]  dos pinos 0-7 e high(H) AFR[1] dos pinos 8-15
    //AF12 para função de USB_OTG_DM(PB14) e também AF12 para USB_OTG_DP(PB15) para AF12 é valor 12 (0x0C)
    MODIFY_REG(GPIOB->AFR[1], GPIO_AFRH_AFRH6 | GPIO_AFRH_AFRH7, _VAL2FLD(GPIO_AFRH_AFRH6, 12) | _VAL2FLD(GPIO_AFRH_AFRH7, 12));
    //Configurar pinos PB14 e PB15 para trabalharem em Alternate Function mode valor no registrador é 2 para alternate function
    MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER14 | GPIO_MODER_MODER15, _VAL2FLD(GPIO_MODER_MODER14, 2) | _VAL2FLD(GPIO_MODER_MODER15, 2));
}

static void usbdCoreInit() {
    //Habilitar clock para USB core
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_OTGHSEN);
    //Configurar USB core para rodar em device mode e usar embedded full speed PHY
    //Turnaround time igual 0x09 de acordo com datasheet para HS
    MODIFY_REG(USB_OTG_HS_GLOBAL->GUSBCFG, 
            USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYSEL | USB_OTG_GUSBCFG_TRDT,
            USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYSEL | _VAL2FLD(USB_OTG_GUSBCFG_TRDT, 0x09)
            );
    //Configurar device para rodar in full speed mode valor no campo = 0x03
    MODIFY_REG(USB_OTG_HS_DEVICE->DCFG, USB_OTG_DCFG_DSPD, _VAL2FLD(USB_OTG_DCFG_DSPD, 0x03));
    //Habilitar VBUS sensing
    SET_BIT(USB_OTG_HS_GLOBAL->GCCFG, USB_OTG_GCCFG_VBDEN);
    //Habilita interrupções necessárias para o USB Core
    /*
    desmascarar seguintes campos para interrupção:
    USB Reset
    USB Suspend
    Start of frame (SOF)
    Early Suspend
    Enumeration done 
    resume/remote Wakeup
    IN endpoints
    Rx FIFO non-empty
    */
    SET_BIT(USB_OTG_HS_GLOBAL->GINTMSK, 
            USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM | USB_OTG_GINTMSK_SOFM | USB_OTG_GINTMSK_IEPINT
            | USB_OTG_GINTMSK_USBSUSPM | USB_OTG_GINTMSK_ESUSPM | USB_OTG_GINTMSK_WUIM | USB_OTG_GINTMSK_RXFLVLM
            | USB_OTG_GINTMSK_OEPINT
            );

    //Limpa todas as interrupts pendentes no core para limpar o campo de interrupção é usado bit 1 
    //Informação tirada do manual de referência
    WRITE_REG(USB_OTG_HS_GLOBAL->GINTSTS, 0xFFFFFFFF);
    //Unmask USB Global Interrupt
    SET_BIT(USB_OTG_HS_GLOBAL->GAHBCFG, USB_OTG_GAHBCFG_GINT);
    //Desmascara interrupt para transferência completa para todos endpoints
    SET_BIT(USB_OTG_HS_DEVICE->DIEPMSK, USB_OTG_DIEPMSK_XFRCM);
    SET_BIT(USB_OTG_HS_DEVICE->DOEPMSK, USB_OTG_DOEPMSK_XFRCM);
}

static void connectDevice() {
    //Liga o transceiver para on
    SET_BIT(USB_OTG_HS_GLOBAL->GCCFG, USB_OTG_GCCFG_PWRDWN);
    //Conecta dispositivo para o barramento 0 = conectado 1 = disconectado
    CLEAR_BIT(USB_OTG_HS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);
}

static void disconnectDevice() {
    //Disconecta dispositivo para o barramento 0 = conectado 1 = disconectado
    SET_BIT(USB_OTG_HS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);
    //Desliga o transceiver para off
    CLEAR_BIT(USB_OTG_HS_GLOBAL->GCCFG, USB_OTG_GCCFG_PWRDWN);
}

//Função para pop de dados do RxFifo
static void readPacket(void *buffer, uint16_t size) {
    //Só existe um RxFifo
    __IO uint32_t *fifo = FIFO(0);
    //For loop percorrendo cada word no fifo, adicionando um word a cada iteração no buffer
    for(; size >= 4; size -= 4, buffer += 4 ) {
        //Pega dado do fifo
        uint32_t data = *fifo;
        //Adiciona dado no buffer
        *((uint32_t*) buffer) = data;
    }
    //Se ainda sobrar bytes com menos de um word
    if(size > 0) {
        //Pega os últimos bytes remanecentes que são menos de um word
        uint32_t data = *fifo;
        //Percorre agora a cada byte, e valor no data sempre tem um shift de 1 byte para a direita
        //Primeiro pega o bit menos significativo e por último os bits mais significativos
        for(; size > 0; size--, buffer++, data >>= 8) {
            //Guarda valor no buffer com alinhamento correto
            //AND com 0xFF porque quer pegar apenas os ultimos 8 bits de data qué é uma variável de 32 bits
            *((uint8_t*) buffer) = 0xFF & data;
        }
    }
}

//Função para enviar pacotes para o TxFifo
static void writePacket(uint16_t endpoint_number, void const *buffer, uint16_t size) {
    //Inicializa ponteiros para acessar o FIFO e os registradores do endpoint IN correspondente
    __IO uint32_t *fifo = FIFO(endpoint_number);
    USB_OTG_INEndpointTypeDef *in_endpoint = IN_ENDPOINT(endpoint_number);
    //Configurar transmissão Número de pacotes e tamanho da transmissão
    MODIFY_REG(in_endpoint->DIEPTSIZ,
            USB_OTG_DIEPTSIZ_PKTCNT | USB_OTG_DIEPTSIZ_XFRSIZ,
            _VAL2FLD(USB_OTG_DIEPTSIZ_PKTCNT, 1) | _VAL2FLD(USB_OTG_DIEPTSIZ_XFRSIZ, size)      
        );
    //Habilita transmissão no endpoint depois de limpar NAK e STALL do endpoint
    MODIFY_REG(in_endpoint->DIEPCTL,
            USB_OTG_DIEPCTL_STALL,
            USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA
    );
    //Obtem tamanho em formato de 32 bit words
    size = (size + 3) / 4;
    //Loop, enquanto tamanho for maior que zero, a cada word lido incrementar endereço do buffer em 4 posições
    for(; size > 0; size--, buffer += 4) {
        //Envia dados ao TxFifo
        *fifo = *((uint32_t*) buffer);
    }
}

//Função vai ser chamada sempre que o tamanho de um TxFIFO muda e atualiza automaticamente todos os endereços dos TxFIFO
static void setTxFifoAddress() {
    //Cria variavel de start_address de cada fifo
    //cada endereço vai ter Fifo_size * 4 de offset entre eles já que é em bytes e o size pe calculado em words
    //Primeiro endereço começa depois do tamanho do RxFifo
    uint16_t start_address = _FLD2VAL(USB_OTG_GRXFSIZ_RXFD, USB_OTG_HS_GLOBAL->GRXFSIZ) * 4;
    //Atualiza start address do TxFifo0
    MODIFY_REG(USB_OTG_HS_GLOBAL->DIEPTXF0_HNPTXFSIZ, USB_OTG_TX0FSA, _VAL2FLD(USB_OTG_TX0FSA, start_address));
    //Incrementa start address para o size do fifo anterior
    start_address += _FLD2VAL(USB_OTG_TX0FD, USB_OTG_HS_GLOBAL->DIEPTXF0_HNPTXFSIZ) * 4;
    //Loop para configurar o endereço dos outros TxFifo
    for(uint8_t fifo_number = 0; fifo_number < ENDPOINT_COUNT - 1; fifo_number++) {
        //configura endereço de endpoint 1 a 8
        MODIFY_REG(USB_OTG_HS_GLOBAL->DIEPTXF[fifo_number],
                USB_OTG_DIEPTXF_INEPTXSA,
                _VAL2FLD(USB_OTG_DIEPTXF_INEPTXSA, start_address)
        );
        //incrementa para próxima contagem
        start_address += _FLD2VAL(USB_OTG_DIEPTXF_INEPTXFD, USB_OTG_HS_GLOBAL->DIEPTXF[fifo_number]) * 4;
    }
}

//Todos os endpoints OUT compartilham do mesmo RxFIFO
static void configureRxFifoSize(uint16_t max_packet_size) {
    //Tamanho do espaço necessário size é em palavras (words = 32 bits = 4 bytes)
    //Considera espaço para packets STATUS que usa 10 words 
    //equação tirada da explicação do manual de referência do STM32F746
    uint16_t size = 10 + (2 * ((max_packet_size / 4) + 1));
    //Depois de feito o calculo implementar tamanho do RxFIFO no registrador correspondente 
    MODIFY_REG(USB_OTG_HS_GLOBAL->GRXFSIZ, 
    USB_OTG_GRXFSIZ_RXFD, 
    _VAL2FLD(USB_OTG_GRXFSIZ_RXFD, size)
    );
    setTxFifoAddress();
}

//Cada Endpoint IN tem um TxFIFO dedicado para eles
static void configureTxFifoSize(uint8_t endpoint_number, uint16_t max_packet_size) {
    uint16_t size = (max_packet_size + 3) / 4 ;
    //Configura tamanho do FIFO, endpoint 0 tem um registrador em endereço diferente fora do array
    if(endpoint_number == 0) {
        MODIFY_REG(USB_OTG_HS_GLOBAL->DIEPTXF0_HNPTXFSIZ,
                USB_OTG_TX0FD,
                _VAL2FLD(USB_OTG_TX0FD, size)
        );
    } else {
        //Registrador é um array todos um seguido do outro na memória
        //subtraído por 1 porque o endpoint 0 não conta no array
        MODIFY_REG(USB_OTG_HS_GLOBAL->DIEPTXF[endpoint_number - 1], 
                USB_OTG_DIEPTXF_INEPTXFD,
                _VAL2FLD(USB_OTG_DIEPTXF_INEPTXFD, size)
        );
    }
    setTxFifoAddress();
}

//Limpa fifo de recepção
static void flushRxFifo() {
    //seta bit para flush no registrador global de reset
    SET_BIT(USB_OTG_HS_GLOBAL->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);
}

//Limpa fifo de transmissão
static void flushTxFifo(uint8_t endpoint_number) {
    //seta bit para flush e indica qual txfifo vai ser limpado
    MODIFY_REG(USB_OTG_HS_GLOBAL->GRSTCTL, 
    USB_OTG_GRSTCTL_TXFNUM,
    _VAL2FLD(USB_OTG_GRSTCTL_TXFNUM, endpoint_number) | USB_OTG_GRSTCTL_TXFFLSH
    );
}
//Configuração do endpoint0 IN endpoint = Host para device, OUT endpoint = device para host
static void endpoint0_Config(uint16_t endpoint_size) {
    //Habilitar interrupts do Endpoint0 IN e OUT
    SET_BIT(USB_OTG_HS_DEVICE->DAINTMSK, USB_OTG_DAINTMSK_INEN0 | USB_OTG_DAINTMSK_OUTEN0);
    /*Configurar tamanho máximo do pacote, ativar endpoint Ie fazer NAK no endpoint (não pode enviar dados)*/
    MODIFY_REG(IN_ENDPOINT(0)->DIEPCTL, 
    USB_OTG_DIEPCTL_MPSIZ,
    USB_OTG_DIEPCTL_USBAEP | _VAL2FLD(USB_OTG_DIEPCTL_MPSIZ, endpoint_size) | USB_OTG_DIEPCTL_SNAK
    );
    //Limpa NAK e libera transmissão de dados para o endpoint
    SET_BIT(OUT_ENDPOINT(0)->DOEPCTL, USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
    //Configura Fifos de Recepção e Transmissão para endpoint 0
    configureRxFifoSize(MAX_PACKET_SIZE);
    configureTxFifoSize(0, endpoint_size);
}



static void inEndpointConfig(uint8_t endpoint_number, UsbTransferType endpoint_type, uint16_t endpoint_size) {
    //Habilitar interrupts do Endpoint IN correspondente
    SET_BIT(USB_OTG_HS_DEVICE->DAINTMSK, 1 << endpoint_number);
    //Configurar tipo de endpoint, tamanho máximo do pacote, ativar endpoint
    //Atribuir um TxFIFO, setar endpoint handshake para NAK e setar pacote DATA0
    MODIFY_REG(IN_ENDPOINT(endpoint_number)->DIEPCTL,
            USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_EPTYP | USB_OTG_DIEPCTL_TXFNUM,
            IN_EP_REG_CONFIG(endpoint_number,endpoint_size, endpoint_type)
    );
    //Configurar tamanho do TxFifo
    configureTxFifoSize(endpoint_number, endpoint_size);
}

static void outEndpointConfig(uint8_t endpoint_number, UsbTransferType endpoint_type, uint16_t endpoint_size) {
    //Habilitar interrupts do Endpoint OUT correspondente
    SET_BIT(USB_OTG_HS_DEVICE->DAINTMSK, 1 << (16 + endpoint_number));
    //Configurar tipo de endpoint, tamanho máximo do pacote, ativar endpoint
    //Setar endpoint handshake para NAK e setar pacote DATA0
    MODIFY_REG(OUT_ENDPOINT(endpoint_number)->DOEPCTL,
            USB_OTG_DOEPCTL_MPSIZ | USB_OTG_DOEPCTL_EPTYP,
            OUT_EP_REG_CONFIG(endpoint_size, endpoint_type)
    );
    //Configurar tamanho RxFifo
    configureRxFifoSize(endpoint_size);
}

static void deconfigureEndpoint(uint8_t endpoint_number) {
    //Ponteiros para auxilio
    USB_OTG_INEndpointTypeDef *in_endpoint = IN_ENDPOINT(endpoint_number);
    USB_OTG_OUTEndpointTypeDef *out_endpoint = OUT_ENDPOINT(endpoint_number);
    //Mascarar (desabilitar) interrupções nos endpoints IN e OUT
    CLEAR_BIT(USB_OTG_HS_DEVICE->DAINTMSK, (1 << endpoint_number) | (1 << (endpoint_number + 16)));
    //Limpa todas interrupções do endpoint
    SET_BIT(in_endpoint->DIEPINT, 0x29FF);
    SET_BIT(out_endpoint->DOEPINT, 0xF17F);
    //Desabilitar endpoint
    //Checa se está com enable habilitado
    // e seta como 1 o bit de desabilitar transmissão no endpoint no registrador
    if(READ_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_EPENA)) {
        SET_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_EPDIS);
    }
    //Desativa endpoint
    CLEAR_BIT(in_endpoint->DIEPCTL, USB_OTG_DIEPCTL_USBAEP);
    //Desativar endpoint OUT apenas para endpoints não que não sejam o 0
    //Endpoint 0 OUT sempre tem que estar habilitado para receber informação do host
    if(endpoint_number != 0) {
        //Desabilita transmissão em endpoint OUT
        if(READ_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_EPENA)) {
            SET_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_EPDIS);
        }
        //Desativa endpoint OUT 
        CLEAR_BIT(out_endpoint->DOEPCTL, USB_OTG_DOEPCTL_USBAEP);
    }
    //Limpa os flushes 
    flushTxFifo(endpoint_number);
    flushRxFifo();
}



static void enumDoneHandler() {
    //Configurar tamanho de pacote máximo para 8 bytes no endpoint 0
    endpoint0_Config(8);
}



static void usbRstHandler() {
    //loop em todos os endpoints e desconfigurar todos eles
    for(uint8_t i = 0; i <= ENDPOINT_COUNT; i++) {
        deconfigureEndpoint(i);
    }
}

static void rxflvlHandler() {
    //Pega informação de status do word do RxFifo
    uint32_t received_status = USB_OTG_HS_GLOBAL->GRXSTSP;
    //Obter numero endpoint
    uint8_t endpoint_number = _FLD2VAL(USB_OTG_GRXSTSP_EPNUM, received_status);
    //Obter contagem de bytes recebido no pacote de dados
    uint16_t bytes_count = _FLD2VAL(USB_OTG_GRXSTSP_BCNT, received_status);
    //obter status do pacote
    uint8_t packet_status = _FLD2VAL(USB_OTG_GRXSTSP_PKTSTS, received_status);
    switch (packet_status)
    {
    //caso receba um pacote SETUP seguido de dados
    case STATUS_SETUP_PKT_RCVD:
        //TODO
        break;
    //caso receba um pacote OUT seguido de dados
    case STATUS_OUT_PKT_RCVD:
        //TODO
        break;
    //estágio de SETUP foi completado
    case STATUS_SETUP_PKT_COMPLETED:
    //Reabilita a transmissão no endpoint
    //Pelo manual de referência o core limpa esse bit sempre SETUP ou uma transferência é completa
        SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPCTL, 
                USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK
            );
        break;
    //Estágio de OUT foi completado
    case STATUS_OUT_PKT_COMPLETED:
    //Mesma coisa reabilitar transmissão no endpoint
        SET_BIT(OUT_ENDPOINT(endpoint_number)->DOEPCTL, 
                USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK
            );
        break;
    default:
        break;
    }
}

//Handles Global interrupts handlers
void globalIntHandler() {
    //Variavel para guardar valor do registrador
    /*__IO = volatile
    variavel pode alterar a qualquer momento então usar isso 
    para dizer ao compilador para não fazer otimizações*/
    __IO uint32_t gintsts = USB_OTG_HS_GLOBAL->GINTSTS;
    //interrupt reset 
    if(gintsts & USB_OTG_GINTSTS_USBRST) {
        //Chama handler de reset 
        usbRstHandler();
        //limpa a interrupção valor 1 = limpa
        SET_BIT(USB_OTG_HS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_USBRST);
    }
    //interrupt enumeration
    else if(gintsts & USB_OTG_GINTSTS_ENUMDNE) {
        enumDoneHandler();
        //limpa a interrupção valor 1 = limpa
        SET_BIT(USB_OTG_HS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_ENUMDNE);
    }
    //interrupt rx fifo non empty
    else if(gintsts & USB_OTG_GINTSTS_RXFLVL) {
        //Limpa interrupt
        rxflvlHandler();
        //limpa a interrupção valor 1 = limpa
        SET_BIT(USB_OTG_HS_GLOBAL->GINTSTS, USB_OTG_GINTSTS_RXFLVL);
    }
    //interrupt IN endpoint
    else if(gintsts & USB_OTG_GINTSTS_IEPINT) {

    }
    //interrupt OUT endpoint
    else if(gintsts & USB_OTG_GINTSTS_OEPINT) {

    }
}

//Declarando instância que vai ser usada em outras camadas
const UsbDriver usb_driver = {
    .connectDevice = &connectDevice,
    .disconnectDevice = &disconnectDevice,
    .flushRxFifo = &flushRxFifo,
    .flushTxFifo = &flushTxFifo,
    .usbdCoreInit = &usbdCoreInit,
    .usbdPinsInit = &usbdPinsInit,
    .writePacket = &writePacket,
    .readPacket = &readPacket,
    .inEndpointConfig = &inEndpointConfig,
    .poll = &globalIntHandler
};