#include "sx1276.h"
#include "string.h"
#include "GlobalVar.h"
#include "led.h"
#include "delay.h"
/* GPIO��غ궨�� */

#define SPI_CS_PIN 			        GPIO_PIN_0
#define SPI_CS_GPIO_PORT 		    GPIOB
#define SPI_CS_GPIO_CLK 		    RCC_AHBPeriph_GPIOB

#define LORA_INT1_PIN			      GPIO_PIN_11
#define LORA_INT1_GPIO_PORT 		GPIOB
#define LORA_INT1_GPIO_CLK 		  RCC_AHBPeriph_GPIOB

#define LORA_RST_PIN			    	GPIO_PIN_10
#define LORA_RST_GPIO_PORT 		  GPIOB
#define LORA_RST_GPIO_CLK 		  RCC_AHBPeriph_GPIOB

#define LORA_TXEN_GPIO_PIN			    GPIO_PIN_1
#define LORA_TXEN_GPIO_PORT   GPIOB
#define LORA_TXMODE_GPIO_CLK 		RCC_AHBPeriph_GPIOB

#define LORA_RXEN_GPIO_PIN			    GPIO_PIN_2
#define LORA_RXEN_GPIO_PORT   GPIOB
#define LORA_RXMODE_GPIO_CLK 		RCC_AHBPeriph_GPIOB

#define  RF_REST_L			  HAL_GPIO_WritePin(LORA_RST_GPIO_PORT, LORA_RST_PIN, GPIO_PIN_RESET)	     
#define  RF_REST_H			  HAL_GPIO_WritePin(LORA_RST_GPIO_PORT, LORA_RST_PIN, GPIO_PIN_SET)	  

#define  RF_CE_L          HAL_GPIO_WritePin(SPI_CS_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_RESET)      
#define  RF_CE_H          HAL_GPIO_WritePin(SPI_CS_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET)            

#define  PA_TXD_OUT()     HAL_GPIO_WritePin( LORA_TXEN_GPIO_PORT, LORA_TXEN_GPIO_PIN, GPIO_PIN_SET );\
													HAL_GPIO_WritePin( LORA_RXEN_GPIO_PORT, LORA_RXEN_GPIO_PIN, GPIO_PIN_RESET );                                          
#define  PA_RXD_OUT()     HAL_GPIO_WritePin( LORA_TXEN_GPIO_PORT, LORA_TXEN_GPIO_PIN, GPIO_PIN_RESET );\
													HAL_GPIO_WritePin( LORA_RXEN_GPIO_PORT, LORA_RXEN_GPIO_PIN, GPIO_PIN_SET );
                                            
/*lora ��ʼ����������*/
//#define   BW_FREQUENCY     7        //6-9
//#define   SPREADINGFACTOR  12       //7-12
//#define   CODINGRATE       1        //1-4
#define   POWERVALUE       7
#define   DATARATE				 4  
//DATARATE rate
// 0       0.3kbps,
// 1       1.2kbps,
// 2       2.4kbps,
// 3       4.8kbps,
// 4       9.6kbps,
// 5       19.2kbps,
const uint8_t  power_data[8] = { 0X80, 0X80, 0X80, 0X83, 0X86, 0x89, 0x8c, 0x8f};
lpCtrlTypefunc_t lpTypefunc = {0,0,0,0};
LORA_DATA_STRUCT lora_data;

static const uint8_t Freq_Table[][3] = {
    { 0x66, 0x80, 0x00 },          //410,  0
    { 0x66, 0xC0, 0x00 },          //411,  1
    { 0x67, 0x00, 0x00 },          //412,  2
    { 0x67, 0x40, 0x00 },          //413,  3
    { 0x67, 0x80, 0x00 },          //414,  4
    { 0x67, 0xC0, 0x00 },          //415,  5
    { 0x68, 0x00, 0x00 },          //416,  6
    { 0x68, 0x40, 0x00 },          //417,  7
    { 0x68, 0x80, 0x00 },          //418,  8
    { 0x68, 0xC0, 0x00 },          //419,  9
    { 0x69, 0x00, 0x00 },          //420,  10
    { 0x69, 0x40, 0x00 },          //421,  11
    { 0x69, 0x80, 0x00 },          //422,  12
    { 0x69, 0xC0, 0x00 },          //423,  13
    { 0x6A, 0x00, 0x00 },          //424,  14
    { 0x6A, 0x40, 0x00 },          //425,  15
    { 0x6A, 0x80, 0x00 },          //426,  16
    { 0x6A, 0xC0, 0x00 },          //427,  17
    { 0x6B, 0x00, 0x00 },          //428,  18
    { 0x6B, 0x40, 0x00 },          //429,  19
    { 0x6B, 0x80, 0x00 },          //430,  20
    { 0x6B, 0xC0, 0x00 },          //431,  21
    { 0x6C, 0x00, 0x00 },          //432,  22
    { 0x6C, 0x40, 0x00 },          //433,  23
    { 0x6C, 0x80, 0x00 },          //434,  24
    { 0x6C, 0xC0, 0x00 },          //435,  25
    { 0x6D, 0x00, 0x00 },          //436,  26
    { 0x6D, 0x40, 0x00 },          //437,  27
    { 0x6D, 0x80, 0x00 },          //438,  28
    { 0x6D, 0xC0, 0x00 },          //439,  29
    { 0x6E, 0x00, 0x00 },          //440,  30
    { 0x6E, 0x40, 0x00 }           //441,  31
};

static const uint8_t LoRa_DataRate_Table[6][3] = {
	{ 7, 12, 1 },   //0.3kbps,       BW: 125KHz, Spreading Factor: 12, Error Coding value: 4/5,
	{ 8, 11, 1 },   //1.2kbps,		   BW: 250KHz, Spreading Factor: 11, Error Coding value: 4/5,
	{ 9, 11, 1 },   //2.4kbps, 	     BW: 500KHz, Spreading Factor: 11, Error Coding value: 4/5,
	{ 8, 8, 2 },    //4.8kbps, 		   BW: 250KHz, Spreading Factor: 8,  Error Coding value: 4/6,
	{ 9, 8, 2 },    //9.6kbps, 		   BW: 500KHz, Spreading Factor: 8,  Error Coding value: 4/6,
	{ 9, 7, 2 }     //19.2kbps       BW: 500KHz, Spreading Factor: 7,  Error Coding value: 4/6,
};


/*************************************************************
  Function   ��Lora_GPIO_Config  
  Description��lora module��Ӧ��MCU�ӿڶ���
	Input      : none
  return     : none 
*************************************************************/	
static void Lora_GPIO_Config(void){
  GPIO_InitTypeDef GPIO_InitStructure;
	
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull  = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  GPIO_InitStructure.Pin   =  SPI_CS_PIN ;
  HAL_GPIO_Init( SPI_CS_GPIO_PORT, &GPIO_InitStructure );
  HAL_GPIO_WritePin(SPI_CS_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET);
	  
  GPIO_InitStructure.Pin   = LORA_TXEN_GPIO_PIN ;
  HAL_GPIO_Init( LORA_TXEN_GPIO_PORT, &GPIO_InitStructure );
	
	 GPIO_InitStructure.Pin  = LORA_RXEN_GPIO_PIN ;
  HAL_GPIO_Init( LORA_RXEN_GPIO_PORT, &GPIO_InitStructure );
   
	GPIO_InitStructure.Pin   = LORA_RST_PIN;
  HAL_GPIO_Init( LORA_RST_GPIO_PORT, &GPIO_InitStructure );
   
  GPIO_InitStructure.Pin   = LORA_INT1_PIN;
	GPIO_InitStructure.Mode  = GPIO_MODE_IT_RISING;
  HAL_GPIO_Init( LORA_INT1_GPIO_PORT, &GPIO_InitStructure );
	
  HAL_NVIC_SetPriority((IRQn_Type)EXTI4_15_IRQn, 0x00, 0);
  HAL_NVIC_EnableIRQ((IRQn_Type)EXTI4_15_IRQn);
}

uint8_t lora_RWByte(uint8_t data){
	uint8_t val;
	SPIx_WriteReadData(&data, &val, 1);
	return val;
}

/*************************************************************
  Function   ��SX1276Reset  
  Description��lora module ��λ����
							        __      ____
               RESET :  |____|  
	Input      : none
  return     : none 
*************************************************************/
static void Delay1s(unsigned int ii){
   uint8_t j;
   while(ii--){
     for(j=0;j<100;j++);
   }
}
void SX1276Reset(void){
   RF_REST_L;	
   Delay1s(200);
   RF_REST_H;
   Delay1s(500);
}

/*************************************************************
  Function   ��cmdSwitchEn  
  Description��SPI chip select
	Input      : cmdEntype 
								enClose  --- disable chip
								enOpen   --- enable chip
  return     : 
*************************************************************/
void cmdSwitchEn(cmdEntype_t cmd){
   switch(cmd){
     case enOpen:{
       RF_CE_L;
     }break;
     case enClose:{
       RF_CE_H;
     }break;
     default:break;
   }
}
/*************************************************************
  Function   ��cmdSwitchEn  
  Description��SPI chip select
	Input      : cmdpatype 
								rxOpen  --- rx enable
								txOpen   ---tx enable
  return     : 
*************************************************************/
void cmdSwitchPA(cmdpaType_t cmd){
   switch(cmd){
     case rxOpen:{
       PA_RXD_OUT();			 
     }break;
     case txOpen:{      
			 PA_TXD_OUT();
     }break;     
     default:break;
   }
}

/*************************************************************
  Function   ��register_rf_func  
  Description���ṹ�������ֵ
	Input      : func lpCtrlTypefunc_t���ͱ��� 
							 ����ֵ��ȫ�ֱ���lpTypefunc
  return     : none  
*************************************************************/	
void register_rf_func(lpCtrlTypefunc_t *func){
   if(func->lpByteWritefunc != 0){
      lpTypefunc.lpByteWritefunc = func->lpByteWritefunc;
   }
   if(func->lpByteReadfunc != 0){
      lpTypefunc.lpByteReadfunc = func->lpByteReadfunc;
   }
   if(func->lpSwitchEnStatus != 0){
      lpTypefunc.lpSwitchEnStatus = func->lpSwitchEnStatus;
   }
   if(func->paSwitchCmdfunc != 0){
      lpTypefunc.paSwitchCmdfunc = func->paSwitchCmdfunc;
   }
}
/*************************************************************
  Function   ��SX1276WriteBuffer  
  Description��SPI ���ַΪaddr�ļĴ���д������buffer
	Input      : addr sx1278�ļĴ�����ַ	
							 buffer д�������
  return     : none  
*************************************************************/	
void SX1276WriteBuffer( uint8_t addr, uint8_t buffer) { 
   lpTypefunc.lpSwitchEnStatus(enOpen); //NSS = 0;
   lpTypefunc.lpByteWritefunc( addr | 0x80 );
   lpTypefunc.lpByteWritefunc( buffer);
   lpTypefunc.lpSwitchEnStatus(enClose); //NSS = 1;
}
/*************************************************************
  Function   ��SX1276ReadBuffer  
  Description��SPI ��ȡ��ַΪaddr�ļĴ�������
	Input      : addr sx1278�ļĴ�����ַ							
  return     : �Ĵ�����ŵ�����  
*************************************************************/		
uint8_t SX1276ReadBuffer(uint8_t addr) {
  uint8_t Value;
  lpTypefunc.lpSwitchEnStatus(enOpen); //NSS = 0;
  lpTypefunc.lpByteWritefunc( addr & 0x7f  );
  Value = lpTypefunc.lpByteReadfunc(0xff);
  lpTypefunc.lpSwitchEnStatus(enClose);//NSS = 1;

  return Value; 
}
/*************************************************************
  Function   ��SX1276LoRaFsk  
  Description��lora������������ ͨ�Ź���ģʽ TX RX CAD SLEEP IDLE
	Input      : opmode RFMode_SEt����							
  return     : none   
*************************************************************/		
void SX1276LoRaSetOpMode( RFMode_SET opMode ) {
   uint8_t opModePrev;
   opModePrev = SX1276ReadBuffer(REG_LR_OPMODE);
   opModePrev &= 0xf8;
   opModePrev |= (uint8_t)opMode;
   SX1276WriteBuffer( REG_LR_OPMODE, opModePrev);		
}
/*************************************************************
  Function   ��SX1276LoRaFsk  
  Description��lora������������ ͨ��ģʽ FSK ����lora
	Input      : opmode
							FSK_mode | LORA_mode
  return     : none   
*************************************************************/	
void SX1276LoRaFsk( Debugging_fsk_ook opMode ) {
   uint8_t opModePrev;
   opModePrev = SX1276ReadBuffer(REG_LR_OPMODE);
   opModePrev &= 0x7F;
   opModePrev |= (uint8_t)opMode;
   SX1276WriteBuffer( REG_LR_OPMODE, opModePrev);		
}
/*************************************************************
  Function   ��SX1276LoRaSetRFFrequency  
  Description��lora������������ ͨ��Ƶ��
							frequency = F(xosc)*Frf/2^19
	Input      : Frequency[i]
							Resolution is 61.035 Hz if F(XOSC) = 32 MHz. Default value is
							0x6c8000 = 434 MHz. Register values must be modified only
							when device is in SLEEP or STAND-BY mode.
  return     : none   
*************************************************************/		
void SX1276LoRaSetRFFrequency(void) {
   SX1276WriteBuffer( REG_LR_FRFMSB, Freq_Table[23][0]);
   SX1276WriteBuffer( REG_LR_FRFMID, Freq_Table[23][1]);
   SX1276WriteBuffer( REG_LR_FRFLSB, Freq_Table[23][2]);
}
/*************************************************************
  Function   ��SX1276LoRaSetRFPower  
  Description��lora������������ ���书������
							PA_BOOST pin.
							Maximum power of +20 dBm
							maxpower  pmax=10.8+0.6*maxpower[dbm]
							Pout=17-(15-OutputPower)
	Input      : power :
							{0X80,0X80,0X80,0X83,0X86,0x89,0x8c,0x8f}&0x0F;
  return     : none   
*************************************************************/		
void SX1276LoRaSetRFPower(uint8_t power ) {
   SX1276WriteBuffer( REG_LR_PADAC, 0x87);
   SX1276WriteBuffer( REG_LR_PACONFIG,  power_data[power] );
}
/*************************************************************
  Function   ��SX1276LoRaSetSpreadingFactor  
  Description��lora������������  ��Ƶ��������
	Input      : value :
								SF rate (expressed as a base-2 logarithm)
								6 --- 64 chips / symbol
								7 --- 128 chips / symbol
								8 --- 256 chips / symbol
								9 --- 512 chips / symbol
								10 --- 1024 chips / symbol
								11 --- 2048 chips / symbol
								12 --- 4096 chips / symbol
								other values reserved
  return     : none   
*************************************************************/
void SX1276LoRaSetSpreadingFactor(uint8_t factor ){
   uint8_t RECVER_DAT;
   SX1276LoRaSetNbTrigPeaks( 3 );
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG2);	  
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG2_SF_MASK ) | ( factor << 4 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG2, RECVER_DAT );	 
}
	
/*************************************************************
  Function   ��SX1276LoRaSetNbTrigPeaks  
  Description��lora������������ 
								2-0 bits of register 0x31
	Input      : value :
  return     : none   
*************************************************************/
void SX1276LoRaSetNbTrigPeaks(uint8_t value ){
   uint8_t RECVER_DAT;
   RECVER_DAT = SX1276ReadBuffer( 0x31);
   RECVER_DAT = ( RECVER_DAT & 0xF8 ) | value;
   SX1276WriteBuffer( 0x31, RECVER_DAT );
}
/*************************************************************
  Function   ��SX1276LoRaSetErrorCoding  
  Description��lora������������ error coding rate
							In implicit header mode should be set on receiver to determine
							expected coding rate.
	Input      : value :
									001  ----  4/5
									010  ----  4/6
									011  ----  4/7
									100  ----  4/8	
  return     : none   
*************************************************************/
void SX1276LoRaSetErrorCoding(uint8_t value ){	
   uint8_t RECVER_DAT;
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG1);
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG1_CODINGRATE_MASK ) | ( value << 1 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG1, RECVER_DAT);
}
/*************************************************************
  Function   ��SX1276LoRaSetPacketCrcOn  
  Description��lora������������ packet crc on
	Input      : enabe :
									true  --- enable
									false --- disable
  return     : none   
*************************************************************/
void SX1276LoRaSetPacketCrcOn(BOOL enable ){	
   uint8_t RECVER_DAT;
   RECVER_DAT= SX1276ReadBuffer( REG_LR_MODEMCONFIG2);
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( enable << 2 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG2, RECVER_DAT );
}

/*************************************************************
  Function   ��SX1276LoRaSetSignalBandwidth  
  Description��lora������������ signal bandwidth
  Input      : bw --- 
								 0000  ----.  7.8khz
								 0001  ----.  10.4khz
								 0010  ----.  15.6khz
								 0011  ----.  20.8khz
								 0100  ----.  31.25khz
								 0101  ----.  41.7khz
								 0110  ----.  62.5khz
								 0111  ----.  125Khz
								 1000  ----.  250khz
								 1001  ----.  500khz
  return     : none   
*************************************************************/
void SX1276LoRaSetSignalBandwidth(uint8_t bw ){
   uint8_t RECVER_DAT;
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG1);
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG1_BW_MASK ) | ( bw << 4 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG1, RECVER_DAT );
}	
/*************************************************************
  Function   ��SX1276LoRaSetImplicitHeaderOn  
  Description��lora������������ ImplicitHeaderOn
  Input      : enabe --- 
									true implicit header mode
									false  explicit header mode
  return     : none   
*************************************************************/
void SX1276LoRaSetImplicitHeaderOn(BOOL enable ){
   uint8_t RECVER_DAT;
   RECVER_DAT=SX1276ReadBuffer( REG_LR_MODEMCONFIG1 );
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( enable );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG1, RECVER_DAT );
}
	
/*************************************************************
  Function   ��SX1276LoRaSetSymbTimeout  
  Description��lora������������ ����Rx operation time-out value 
							Timeout =  symboltimeout.Ts
  Input      : value  9bit valid 
  return     : none   
*************************************************************/
void SX1276LoRaSetSymbTimeout(unsigned int value ){
   uint8_t RECVER_DAT[2];
   RECVER_DAT[0]=SX1276ReadBuffer( REG_LR_MODEMCONFIG2 );
   RECVER_DAT[1]=SX1276ReadBuffer( REG_LR_SYMBTIMEOUTLSB );
   RECVER_DAT[0] = ( RECVER_DAT[0] & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( ( value >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK );
   RECVER_DAT[1] = value & 0xFF;
   SX1276WriteBuffer( REG_LR_MODEMCONFIG2, RECVER_DAT[0]);
   SX1276WriteBuffer( REG_LR_SYMBTIMEOUTLSB, RECVER_DAT[1]);
}
/*************************************************************
  Function   ��SX1276LoRaSetPayloadLength  
  Description��lora������������ ����payload length
  Input      : value  ��0�� ������ 
  return     : none   
*************************************************************/
void SX1276LoRaSetPayloadLength(uint8_t value ){
   SX1276WriteBuffer( REG_LR_PAYLOADLENGTH, value );
} 
/*************************************************************
  Function   ��SX1276LoRaSetPreamLength  
  Description��lora������������ ����preamble length
  Input      : value ���ֽ�����MSB ���ֽ�����LSB MSB=preamble length +4.25symbol
  return     : none   
*************************************************************/
void SX1276LoRaSetPreamLength(unsigned int value ){
   uint8_t a[2];
   a[0]=(value&0xff00)>>8;
   a[1]=value&0xff;
   SX1276WriteBuffer( REG_LR_PREAMBLEMSB, a[0] );
   SX1276WriteBuffer( REG_LR_PREAMBLELSB, a[1] );
}
/*************************************************************
  Function   ��SX1276LoRaSetMobileNode  
  Description��lora������������ �����Ż���
  Input      : Bool true-�Ż� false --���Ż�
  return     : none   
*************************************************************/
void SX1276LoRaSetMobileNode(BOOL enable ){	 
   uint8_t RECVER_DAT;
   RECVER_DAT = SX1276ReadBuffer( REG_LR_MODEMCONFIG3 );
   RECVER_DAT = ( RECVER_DAT & RFLR_MODEMCONFIG3_MOBILE_NODE_MASK ) | ( enable << 3 );
   SX1276WriteBuffer( REG_LR_MODEMCONFIG3, RECVER_DAT );
}

uint8_t SX1276_GetVersion(void){
	uint8_t ret_Version;
	
	ret_Version = SX1276ReadBuffer( REG_LR_VERSION );
	return ret_Version;
}
/*************************************************************
  Function   ��SX1276LORA_INT  
  Description��lora��ʼ��,ͨ�Ų���
  Input      : none 
  return     : none   
*************************************************************/
static void SX1276LORA_INT(void){
   SX1276LoRaSetOpMode(Sleep_mode);  									//����˯��ģʽ
   SX1276LoRaFsk(LORA_mode);	     										// ������Ƶģʽ
   SX1276LoRaSetOpMode(Stdby_mode);  									// ����Ϊ��ͨģʽ
   SX1276WriteBuffer( REG_LR_DIOMAPPING1,GPIO_VARE_1);// IO ��־���� IO 0 1 2 3
   SX1276WriteBuffer( REG_LR_DIOMAPPING2,GPIO_VARE_2); // IO 5	 
	 SX1276LoRaSetRFFrequency();								        //����Ƶ�� 434MHZ 
	 SX1276LoRaSetRFPower(POWERVALUE);					        // ���ù���  
	 SX1276_LoRa_SetDataRate(DATARATE);   
//   SX1276LoRaSetSpreadingFactor(SPREADINGFACTOR);	    // ��Ƶ��������
//   SX1276LoRaSetErrorCoding(CODINGRATE);		          //��Ч���ݱ�
//   SX1276LoRaSetSignalBandwidth( BW_FREQUENCY );	    //������Ƶ���� 
	 SX1276LoRaSetPacketCrcOn(True);			              //CRC У���
   SX1276LoRaSetImplicitHeaderOn(False);		          //ͬ��ͷ������ģʽ
   SX1276LoRaSetPayloadLength( 0xff);									//���payload length 255
   SX1276LoRaSetSymbTimeout( 0x3FF );									//��ʱ����
   SX1276LoRaSetMobileNode(True); 			              // �����ݵ��Ż�
   RF_RECEIVE(); 																		  //����RX ģʽ
}
/*************************************************************
  Function   ��FUN_RF_SENDPACKET  
  Description��lora ����һ�����ȵ�����
  Input      : RF_TRAN_P ---������������ LEN--���������ݳ���
  return     : none   
*************************************************************/	
void FUN_RF_SENDPACKET(uint8_t *RF_TRAN_P,uint16_t LEN){
   uint16_t ASM_i;
   lpTypefunc.paSwitchCmdfunc(txOpen);
   SX1276LoRaSetOpMode( Stdby_mode );
   SX1276WriteBuffer( REG_LR_HOPPERIOD, 0 );								//����Ƶ������
   SX1276WriteBuffer(REG_LR_IRQFLAGSMASK, IRQN_TXD_Value);		//�򿪷����ж�
   SX1276WriteBuffer( REG_LR_PAYLOADLENGTH, LEN);	 					//������ݰ�
   SX1276WriteBuffer( REG_LR_FIFOTXBASEADDR, 0);
   SX1276WriteBuffer( REG_LR_FIFOADDRPTR, 0 );
   lpTypefunc.lpSwitchEnStatus(enOpen);
   lpTypefunc.lpByteWritefunc( 0x80 );
   for( ASM_i = 0; ASM_i < LEN; ASM_i++ ){
     lpTypefunc.lpByteWritefunc( *RF_TRAN_P );RF_TRAN_P++;
   }
   lpTypefunc.lpSwitchEnStatus(enClose);
   SX1276WriteBuffer(REG_LR_DIOMAPPING1,0x40);//DIO0 TxDone
   SX1276WriteBuffer(REG_LR_DIOMAPPING2,0x00);
   SX1276LoRaSetOpMode( Transmitter_mode );
}

/*************************************************************
  Function   ��RF_RECEIVE  
  Description��lora ����ģʽ ���մ� �жϷ�ʽ
  Input      : none 
  return     : none   
*************************************************************/
void RF_RECEIVE (void){
   SX1276LoRaSetOpMode(Stdby_mode );
   SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,IRQN_RXD_Value);  //��Rx Done�ж�
   SX1276WriteBuffer(REG_LR_HOPPERIOD,	PACKET_MIAX_Value );
   SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X00 );//DIO0 RxDone
   SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0X00 );	
   SX1276LoRaSetOpMode( Receiver_mode );
   lpTypefunc.paSwitchCmdfunc(rxOpen);
}
	
/*************************************************************
  Function   ��RF_CAD_RECEIVE  
  Description��lora CAD ��� ���մ� 
  Input      : none 
  return     : none   
*************************************************************/
void RF_CAD_RECEIVE (void){
   SX1276LoRaSetOpMode( Stdby_mode );
   SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,  IRQN_CAD_Value);	//��CAD Done �ж�
   SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X80 );//DIO0 CadDone
   SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0X00 );	
   SX1276LoRaSetOpMode( CAD_mode );
   lpTypefunc.paSwitchCmdfunc(rxOpen);
}
	
/*************************************************************
  Function   ��RF_SLEEP  
  Description��lora sleepģʽ���� 
  Input      : none 
  return     : none   
*************************************************************/
void RF_SLEEP(void){
   SX1276LoRaSetOpMode( Stdby_mode );
   SX1276WriteBuffer(REG_LR_IRQFLAGSMASK,  IRQN_SLEEP_Value);  //��Sleep�ж�
   SX1276WriteBuffer( REG_LR_DIOMAPPING1, 0X00 );
   SX1276WriteBuffer( REG_LR_DIOMAPPING2, 0X00 );	
   SX1276LoRaSetOpMode( Sleep_mode );
}

/*************************************************************
  Function   ��Lora_Send  
  Description������һ�����ȵ�����
  Input      : p_send_buf -����������buffer  len --���������ݳ���
  return     : none    
*************************************************************/
static uint8_t lora_tx_with_timeout(uint8_t* data, uint16_t len, uint32_t timeOut){
		lora_data.lora_send_flag = 0;
		lora_data.lora_send_tickTime = local_ticktime();
		FUN_RF_SENDPACKET(data, len);
		while(lora_data.lora_send_flag == 0){
			if(timeout(lora_data.lora_send_tickTime, timeOut)){
				return 1;
			}
		}
		return 0;
}
#define LORA_MAX_TX_NUM 255
uint8_t Lora_Send(uint8_t *p_send_buf, uint16_t len){
	uint8_t pkt_num = 0, pkt_remain = 0;
	uint8_t* p = p_send_buf;
	uint8_t j = 0;
	
	if(len <= LORA_MAX_TX_NUM){
		if(lora_tx_with_timeout(p, len, LORA_SEND_MAX_TIMEOUT))
				return 1;
	}	else{
		pkt_num = len/LORA_MAX_TX_NUM;
		pkt_remain = len%LORA_MAX_TX_NUM;
		
		for(j = 0; j < pkt_num; j++){
			if(lora_tx_with_timeout(p, LORA_MAX_TX_NUM, LORA_SEND_MAX_TIMEOUT))
				return 1;
			p += LORA_MAX_TX_NUM;
		}
		Delay_nms(200);
		if(lora_tx_with_timeout(p, pkt_remain, LORA_SEND_MAX_TIMEOUT))
			return 1;	
		Delay_nms(400);
	}
	return 0;
}

__weak void SX1276_InitSuccess(void){
	//todo
}
__weak void SX1276_InitFailed(void){	
	//todo
}
/*************************************************************
  Function   ��Lora_Init  
  Description��lora��ʼ����GPIO ��ͨ�Ų�������λ
  Input      : none 
  return     : none   
*************************************************************/
void Lora_Init(void){
	lpCtrlTypefunc_t  ctrlTypefunc = {
		 lora_RWByte,//RF_SPI_WRITE_BYTE,
		 lora_RWByte,//RF_SPI_READ_BYTE,
		 cmdSwitchEn,
		 cmdSwitchPA
	};
	Lora_GPIO_Config();
	register_rf_func(&ctrlTypefunc);
	SX1276Reset();
	SX1276LORA_INT();
	if(SX1276_GetVersion() == 0x12){
		SX1276_InitSuccess();
	}else{
		SX1276_InitFailed();
	}
}


//**************�������ж����洦���Ĵ���**********************
/*************************************************************
  Function   ��SX1278_Interupt  
  Description��lora��Ӧ�жϱ�־������TXDONE RXDONE CADDONE ```
  Input      : none 
  return     : none   
*************************************************************/
void SX1278_Interupt(void){
	uint8_t   RF_REC_RLEN_i = 0;
	uint8_t   CRC_Value;
	uint8_t   RF_EX0_STATUS;
	
  RF_EX0_STATUS=SX1276ReadBuffer( REG_LR_IRQFLAGS ); 
	
  if((RF_EX0_STATUS&0x40) == 0x40){														//���� RX Done
    CRC_Value=SX1276ReadBuffer( REG_LR_MODEMCONFIG2 );
    if((CRC_Value&0x04) == 0x04 ){
      SX1276WriteBuffer (REG_LR_FIFOADDRPTR,0x00);
      lora_data.lora_recv_data_buf_len = SX1276ReadBuffer(REG_LR_NBRXBYTES);
			lora_data.lora_data_arrived = 1;
      lpTypefunc.lpSwitchEnStatus(enOpen);
      lpTypefunc.lpByteWritefunc( 0x00 );
      for(RF_REC_RLEN_i = 0; RF_REC_RLEN_i < lora_data.lora_recv_data_buf_len; RF_REC_RLEN_i++){
        lora_data.lora_recv_data_buf[RF_REC_RLEN_i] = lpTypefunc.lpByteReadfunc(0xff);								
      }
      lpTypefunc.lpSwitchEnStatus(enClose);
    }       
    RF_RECEIVE();
  }else if((RF_EX0_STATUS&0x08) == 0x08){											// TX Done
    lora_data.lora_send_flag = 1;LED_TOGGLE(LED_NO_0);
		RF_RECEIVE();
  }else if((RF_EX0_STATUS&0x04) == 0x04){  										// CAD Done
    if((RF_EX0_STATUS&0x01)==0x01){     //��ʾCAD ��⵽��Ƶ�ź� ģ������˽���״̬����������
      RF_RECEIVE();
    }else{                           // û��⵽
      RF_SLEEP();
    }
  }else{
    RF_RECEIVE();
  }
  SX1276WriteBuffer( REG_LR_IRQFLAGS, 0xff  );
}

void SX1276_LoRa_SetDataRate( uint8_t rate ){
    uint8_t tmp;
    rate = rate <= 5 ? rate : 5;
    SX1276LoRaSetOpMode(Stdby_mode );
    if( ( rate == 2 ) || ( rate == 3 ) ){
        SX1276WriteBuffer( REG_LR_MODEMCONFIG3, DEF_LR_MODEMCONFIG3 );  //Regs 0x26, Low Data Rata Optimization Disabled
    }
    else{
		SX1276WriteBuffer( REG_LR_MODEMCONFIG3, 0x08 );  //Regs 0x26, Low Data Rata Optimization Enabled
    }
    if( ( rate == 2 ) || ( rate == 4 ) || ( rate == 5 ) ){
        SX1276WriteBuffer( 0x36, 0x02 );
        SX1276WriteBuffer( 0x3A, 0x7F ); //Special setting when BW = 9, 500KHz

        tmp = SX1276ReadBuffer( 0x31 );
        tmp |= ( 1<<0 );
        SX1276WriteBuffer( 0x31, tmp );
    }
    else{
        SX1276WriteBuffer( 0x36, 0x03 );  //reset setting

        tmp = SX1276ReadBuffer( 0x31 );
        tmp &= ~( 1<<0 );
        SX1276WriteBuffer( 0x31, tmp );
        SX1276WriteBuffer( 0x2F, 0x40 );
        SX1276WriteBuffer( 0x30, 0x00 );
   }
	SX1276LoRaSetSignalBandwidth( LoRa_DataRate_Table[rate][0] );
	SX1276LoRaSetSpreadingFactor( LoRa_DataRate_Table[rate][1] );
	SX1276LoRaSetErrorCoding( LoRa_DataRate_Table[rate][2] );
}