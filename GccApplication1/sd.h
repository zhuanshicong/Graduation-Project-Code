//******************************************************************
//SPI������ռ�õĶ˿�
#define SD_SS       PB6          //ѡƬ
#define SD_SCK       PB1         //ʱ��
#define SD_MOSI        PB2       //��������ӻ�����
#define SD_MISO        PB3       //�ӻ������������
//******************************************************************

#define SD_DDR       DDRB
#define SD_PORT      PORTB
#define SD_PIN       PINB

#define SD_SS_H        SD_PORT |= (1<<SD_SS)
#define SD_SS_L        SD_PORT &= ~(1<<SD_SS)
#define SD_SCK_H       SD_PORT |= (1<<SD_SCK)
#define SD_SCK_L       SD_PORT &= ~(1<<SD_SCK)
#define SD_MOSI_H      SD_PORT |= (1<<SD_MOSI)
#define SD_MOSI_L      SD_PORT &= ~(1<<SD_MOSI)
#define SD_MISO_H      SD_PORT |= (1<<SD_MISO)
#define SD_MISO_L      SD_PORT &= ~(1<<SD_MISO)

//-------------------------------------------------------------
// �����
//-------------------------------------------------------------
#define INIT_CMD0_ERROR       0xFF
#define INIT_CMD1_ERROR       0xFE
#define WRITE_BLOCK_ERROR     0xFD
#define READ_BLOCK_ERROR      0xFC
#define TRUE                    0x01
//------------------------------------------------------------- 
// SD������(����Ŵ�40��ʼ��ֻ�г����������û�ж�ʹ��)
//-------------------------------------------------------------
#define SD_RESET                0                   
#define SD_INIT                 1
#define SD_SEND_IF_COND         8 
#define SD_READ_CSD             9
#define SD_READ_CID             10
#define SD_STOP_TRANSMISSION    12
#define SD_SEND_STATUS          13
#define SD_SET_BLOCKLEN         16
#define SD_READ_BLOCK           17
#define SD_READ_MULTI_BLOCK     18
#define SD_WRITE_BLOCK          24
#define SD_WRITE_MULTI_BLOCK    25
#define SD_EARSE_WR_BLK_START_ADDR  32
#define SD_EARSE_WR_BLK_END_ADDR    33
#define SD_EARSE                38
#define SD_SEND_OP_COND         41
#define SD_APP_CMD              55
#define SD_READ_OCR             58
#define SD_CRC_ON_OFF           59



//Ƭѡ��
#define SD_Disable() SD_SS_H
//Ƭѡ�� 
#define SD_Enable() SD_SS_L