/*
 * GccApplication1.c
 *
 * Created: 2016/4/26 17:11:18
 * Author : zhuanshicong
 */ 
#include <avr/io.h>
#include <sd.h>
#include <DHT11.h>
#include <util/delay.h>
#include <1602.h>
#include <avr/interrupt.h>  
/*������Ҫ��ȫ�ֱ���������Ҫ�ɼ������ݡ�SD����ַ����Ҫ�м�����ȵ�*/
unsigned int w_data_t[128],w_data_h[128],w_time=512,w_i,w_f=1,w_timeup=0;
unsigned char w_data_adc_int[128],w_data_adc_sn1[128],w_data_adc_sn2[128],w_data_button[128],SDHC;
unsigned long w_addr,File1Addr,File2Addr,File3Addr;
unsigned char button_stop=0;
/* SD��SPIģʽ����˿ڵĳ�ʼ��*/
void SD_Port_Init(void)
{      
      SD_DDR=(1<<SD_MOSI)|(1<<SD_SCK)|(1<<SD_SS)|(1<<PB0); //����MOSI��SCK��SSΪ���
	  SD_DDR&= ~(1<<SD_MISO);  //����MISOΪ����
	  
	  SD_SCK_H;
	  SD_SS_H;    //����SS�Լ�SCK�˿�
	  PORTB|= (1<<PB0); //����Ϊ���ߵ�Ƭ����Ϊ�ӻ�ʱ��ѡƬ�˿ڣ���Ƭ����Ϊ����ʱ���˶˿ڲ���ΪѡƬʹ�ã�������ֳ���
	  
	  SPCR=0x53;                         
      SPSR=0x00;     //ͬʱ����SPI�Ĵ���ʹ����SD����ʼ��ʱ�������ٳ�ʼ��
	                       
}
/* DHT11��ʪ�ȴ�������ʼ��*/
void DHT_Init(void)
{   
	/* ����DHT11ʹ��˵������Ҫ�������������ͱ������18MS,��֤DHT11�ܼ�⵽��ʼ�źţ�Ȼ��������20~40us���ȴ�DHT11Ӧ��*/
	DHT_DATA_O;  //����DHT���ݶ˿�Ϊ���
	DHT_L;       //����DHT���ݶ˿�
	_delay_ms(20);
	DHT_H;       //����DHT���ݶ˿�      
	_delay_us(30);
	DHT_DATA_I; //����DHT���ݶ�Ϊ���룬�ȴ�DHT11Ӧ��
	while(CHECK_DHT_H);//���յ�DHT11�������ݶ˿�ʱ��˵����ʼ�����
}
/* �ⲿ�жϳ�ʼ�� */
void Ext_Int_Init(void)
{
	DDRE&= ~(1<<PE6);  
	PORTE&= ~(1<<PE6);  //ʹ��PE6�˿ڲ�������Ϊ���벢����
	EICRB=(1<<ISC61)|(1<<ISC60); //����Ϊ�½��ز����ж�
	EIMSK|=(1<<INT6); //����PE6���ⲿ�жϹ���
	
}
/* ��ʱ����ʼ�� */
void TC1_Init(void)
{
	TCCR1A=0x00;  //����Ϊ��ͨģʽ
	TCCR1B=(1<<CS10)|(1<<CS12);//����Ϊ1024��Ƶ
	TCCR1C=0x00;
	TCNT1=0; //��ʱ��1��ʼֵ
	TIMSK|=(1<<TOIE1); //������ʱ��1�ж�
}
/* ADת����ʼ�� */
void ADC_Init(void)
{
	ADMUX=(1<<REFS0); //ѡ��ο���ѹΪAVCC ����ѡ��ADC0����Ϊģ��ת����
	ADCSRA=(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);//ѡ��128��Ƶ ��������ADת��
}
/*SPIģʽ����1BYTE���ݺ���*/
unsigned char SPI_TransferByte(unsigned char byte)
{
SPDR = byte; //��һ�ֽ�����SPI���ݼĴ���
while (!(SPSR&(1<<SPIF)));//�ȴ��������                                            
return SPDR;//���ؽ��յ�������
}
/*
unsigned char GET_Response(void)
{ 
  int i=0;
  unsigned char response;
  while(i<=64)
  {
   response=SPI_TransferByte(0xff);
   if(response==0x00)break;
   if(response==0x01)break;
   i++;
  }
  return response;
}
*/
/*��SD�����������*/
unsigned char SD_SendCommand(unsigned char cmd, unsigned long arg,unsigned char CRC)
{
	unsigned char r1;
	unsigned char retry1 = 0;      	//�ظ���������

	SD_Enable();                        	//ʹ��Ƭѡ�ź�

	SPI_TransferByte(cmd | 0x40);   	//�ֱ�д������
	SPI_TransferByte(arg >> 24);      	//���ݶε�4�ֽ�
	SPI_TransferByte(arg >> 16);      	//���ݶε�3�ֽ�
	SPI_TransferByte(arg >> 8);       	//���ݶε�2�ֽ�
	SPI_TransferByte(arg);          	//���ݶε�1�ֽ�
	SPI_TransferByte(CRC);         	//CRCЧ��

	while ((r1 = SPI_TransferByte(0xff)) == 0xff) //�ȴ���Ӧ
		if (retry1++ > 200) break; 	//��ʱ�˳�

	SD_Disable();                        	//���Ƭѡ�ź�
	return r1;                   	//����״ֵ̬ ������Ĭ�Ͻ��յ�����Ӧ�ĵ�һ���ֽ� ��R1��Ӧ �緢�͵�������Ӧ�����Ĳ���R1 ��Ҫ�ٴο���Ƭѡ������SPI_TransferByte(0XFF)����ʣ�µ���Ӧ����
}
/*���SD���Ƿ�ΪSDHC������*/
void SD_Check_If_SDHC(void)
{
	unsigned char temp;
	temp=SD_SendCommand(SD_READ_OCR,0,0xFF);
	temp=SPI_TransferByte(0xff);
	SPI_TransferByte(0xff);
	SPI_TransferByte(0xff);
	SPI_TransferByte(0xff);
	if((temp&0x40)==0)   {SDHC=0;}
	if(!(temp&0x40)==0)  {SDHC=1;}
	
}
/*SD����λ����*/
unsigned char SD_reset(void)
{     
	 unsigned char r1,i;
       
	   SD_Disable();
	   
       for (i=0;i<200;i++)          
       {
            SPI_TransferByte(0xff); 
       }                                  //����SD��Э�飬ȷ��SD��ȷ��λ��Ҫ�ӳ�74�����ϵ�ʱ��
	                                      
	   
	r1=SD_SendCommand(SD_RESET,0,0x95);   //���͸�λ���CRCУ����̶�λ0X95
	SPI_TransferByte(0xff);	 	
		return r1;                        //������Ӧ����
		
}
/*SD����ʼ������*/
unsigned char SD_Init(void)
{  
    unsigned char temp;
	unsigned int  i;
	unsigned long SD_Init_Argument;
     temp=SD_SendCommand(SD_SEND_IF_COND,0X000001AA,0x87);  //����SD��Э�飬��ʼ��SD����Ҫ�ȷ���CMD8ȷ��SD���Ƿ�2.0Э�鲢ȷ��SD��ʹ�õĵ�ѹ��Ĭ��3.3���͸ÿ��Ƿ����
	 for(i=0;i<20;i++)
	   {SPI_TransferByte(0xff);}
		SD_Check_If_SDHC();
		if(SDHC==0)SD_Init_Argument=0x00000000;
		if(SDHC==1)SD_Init_Argument=0x40000000;
     i=0;
	 while(temp!=0x00)   //��ʼ���������ĳ�ʼ������
	 {
	  temp=SD_SendCommand(SD_APP_CMD,0,0xff);  //�ȷ���CMD58����SD��ʶ��ACMD����
	  temp=SD_SendCommand(SD_SEND_OP_COND,SD_Init_Argument,0xff);  //�ٷ���ACMD41����
	  i++;        
	  if(i>200)  break;           
	 }
	 SPI_TransferByte(0xff);
	 PORTC=temp;
     return (temp);                 //����R1��ӦΪ0X00ȷ�ϳ�ʼ���ɹ�
}
/*SD���������ݿ��С����*/
unsigned char SD_Set_data_size(void)
{
 unsigned char temp;
 
 temp=SD_SendCommand(SD_SET_BLOCKLEN,512,0xff); //����CMD16�����������ݿ��С��ͨ��Ϊ�˷������ݴ���������ֵ���ó�512�ֽڼ�һ�������Ĵ�С

 SPI_TransferByte(0xff);
 return temp;
}
/*SD�������Ƿ�����CRCУ�麯��*/
unsigned char SD_Set_CRC(void)
{ 
 unsigned  char temp;
 
 temp=SD_SendCommand(SD_CRC_ON_OFF,0,0x95);//����CMD59��������CRCУ��
 
 SPI_TransferByte(0xff);
 return temp;
}
/*���SD����ǰ״̬����*/
unsigned char SD_Check_status(void)
{ 
  unsigned char temp1,temp2;
  temp1=SD_SendCommand(SD_SEND_STATUS,0,0XFF);//����CMD13������R2��һ�ֽڴ���temp1
  SD_Enable();
  temp2=SPI_TransferByte(0xff);  //R2�ڶ��ֽڴ���temp2
  SD_Disable();
  return temp2;
}
/*SD����������*/
void SD_erase(unsigned long addr1,unsigned long addr2)
{   
	unsigned char temp;
	temp=SD_SendCommand(SD_EARSE_WR_BLK_START_ADDR,addr1,0xff); //����cmd32�趨SD��������ʼ��ַ
	temp=SD_SendCommand(SD_EARSE_WR_BLK_END_ADDR,addr2,0xff);   //����cmd33�趨SD������������ַ
	temp=SD_SendCommand(SD_EARSE,0x00,0xff);                   // ����CMD38ִ�в�������
	SD_Enable();
	while(temp==0x00)
	{temp=SPI_TransferByte(0xff);
	 PORTC=temp;}
	SD_Disable();
	
}
/*DHT11��ʪ�������ռ�����*/
unsigned int  Collect_DHT_Data(void)
 {  
	unsigned char dht_data[5];
	unsigned char i,j;
	unsigned char Ccode,temp_l,temp_h,hum_l,hum_h;
	unsigned int final_dht_data;
	
	DHT_Init();    //ÿ�βɼ����ݶ�Ҫ��ִ��DHT11�������ĳ�ʼ��
	while(!CHECK_DHT_H);  //��ʼ����DHT11����80us�ĵ͵�ƽ��Ӧ�ź� �ȴ���Ӧ���
	while(CHECK_DHT_H);   //��Ӧ��ɺ� DHT11����80us �ȴ����
	for(i=0;i<5;i++)     //��ʼ�ռ��վ�
	{ 
		dht_data[i]=0x00;
		for(j=0;j<8;j++)
		{ 
			while(!CHECK_DHT_H);  //�ȴ���һbit���ݸߵ�ƽ����
			_delay_us(50);       //�ӳ�50us
			if(CHECK_DHT_H)     // �����ʱ�ź���ȻΪ�ߣ���ô�����λ����ӦΪ1
			{
			 dht_data[i]|=(1<<(7-j));
			}
			while(CHECK_DHT_H);
		}
	  	
	}
	DHT_DATA_O;   
	DHT_H;     //ֹͣ�ɼ� ���ö˿�Ϊ���������
	Ccode=(dht_data[0]+dht_data[1]+dht_data[2]+dht_data[3]);//�õ����ݼ�����
	if(Ccode==dht_data[4]) //���������������վ�
	{
	hum_h=dht_data[0];
	hum_l=dht_data[1];
	temp_h=dht_data[2];
	temp_l=dht_data[3];
	}
	final_dht_data=hum_h;  //�վݴ���
	final_dht_data<<=8;
	final_dht_data|=temp_h;
	  
	return final_dht_data; //���صõ����ݣ�16bit ��8λΪʪ�ȣ���8λΪ�¶�
 }
 /*�ռ���ģת�����ݺ���*/
unsigned char Collect_ADC_Data(void)
{ 
  unsigned int adc_data,adc_l,adc_h;
  
  ADCSRA|=(1<<ADSC);   //��ʼADת��
  while(!(ADCSRA & 0x10));  //�ȴ�ת������
  _delay_us(500);
  adc_l=ADCL;          //��ȡ��λ����
  adc_h=ADCH;           //��ȡ��λ����
  adc_data=adc_h<<8|adc_l;
  adc_data=adc_data>>1;               //����һλ�ľ���
  adc_data-=35;                        //����
  ADCSRA&=~(1<<ADIF);    //���־
  return adc_data;      //����ת��������
}
/*�жϰ�ť�Ƿ��º���*/
unsigned char Collect_BUTTON_Data(void)
{   
	unsigned char button;
	DDRE&= ~(1<<PE5);
	PORTE&= ~(1<<PE5);
	_delay_ms(10);
	if ((PINE&(1<<PE5))==0) button=0;  //������ʾ0
	if (!(PINE&(1<<PE5))==0) button=1; //û������ʾ1
	_delay_ms(10);
	return button;
}
/*�¶�����ת��Ϊ��λ��*/
unsigned int  data_to_dec(unsigned char data)
{
	unsigned char temp,temp_units, temp_tens;
	unsigned int t;
	//temp=(data>>8);
	  temp=data;
	 temp_units=temp%10;
	 temp_tens=temp/10;
	 t=temp_units*10+temp_tens;
	 return t;
	 
}
/*SD��д�����ݺ���*/
unsigned char SD_Write(void)
{ 
 unsigned char temp;
 unsigned int i,t0,t1,h0,h1,stop_temp;
 unsigned long addr;
 //addr=420048<<9;
 addr=w_addr;
 
 SD_SendCommand(SD_WRITE_BLOCK,addr,0xFF);  //����CMD24���ָ����ʼд��ĵ�ַ
 
 SD_Enable();
 
 SPI_TransferByte(0xff);  
 SPI_TransferByte(0xff);
 SPI_TransferByte(0xff);
 SPI_TransferByte(0xff);
 SPI_TransferByte(0xff);
 SPI_TransferByte(0xff); //�ʵ��ӳ�һ��ʱ�� �������
 
 SPI_TransferByte(0xfe);  //��������ͷ��ʶȷ�Ͽ�ʼ¼������
 
   
	  for (i=0;i<32;i++)
      {  
		if(i<=w_time||button_stop==0)
		 {
			 t0=w_data_t[i]%10;
		     t1=w_data_t[i]/10;
		     SPI_TransferByte(t0+48);
		     SPI_TransferByte(t1+48);  //¼���¶�����
			 
			 SPI_TransferByte(0x00);
			 
			 h0=w_data_h[i]%10;
			 h1=w_data_h[i]/10;
			 SPI_TransferByte(h0+48);  
			 SPI_TransferByte(h1+48);  //¼��ʪ������
			 
			 SPI_TransferByte(0x00);
			 
			 
			 SPI_TransferByte(w_data_adc_int[i]+48);
			 SPI_TransferByte(0x2E);
			 SPI_TransferByte(w_data_adc_sn1[i]+48);
			 SPI_TransferByte(w_data_adc_sn2[i]+48); //¼��adc����
			 
			 SPI_TransferByte(0x00);
			 
			 SPI_TransferByte(0x00);
			 SPI_TransferByte(w_data_button[i]+48);  //¼�밴ť����
			 
		 }
		 
	    if(i>w_time)
		 {  
			  for(stop_temp=0;stop_temp<13;stop_temp++)
		     { 
				 SPI_TransferByte(0x00);  //ֹͣ������ʱ��Ŀ�����
			 }
		 }
		SPI_TransferByte(0);
	    SPI_TransferByte(0);
		SPI_TransferByte(0);
	  }
 
  
 
 SPI_TransferByte(0x95);
 SPI_TransferByte(0x95);
 
 while ((temp = SPI_TransferByte(0xff)) == 0xff);//�ȴ��������
 //temp=SPI_TransferByte(0xff); 
 //while (!((temp&0x0f) == 5));
 while (!(SPI_TransferByte(0xff)));
 SD_Disable();
 return temp;
}
/*ȷ��SD���ļ��ڵ�ַ����*/
void Set_SD_File_Addr(void)
{   
	unsigned int i,j,RsvdSecCnt,temp0;
	unsigned char SecPerClus,temp;
	unsigned long RootClus,FAtSz32,RootDirAddr,temp1,check_file_name[8],FileClus;
	/*�������������ÿ����������������������fat������������Ŀ¼�غ�*/
	SD_SendCommand(SD_READ_BLOCK,0x00000000,0xFF); //����CMD17 ���õ�ַΪ0x00000000��SD������������
	SD_Enable(); 
	
	while ((SPI_TransferByte(0xff))!= 0xfe); //����SD����ʼ�������ݱ�־
	for(i=0;i<512;i++)
	{    
		//if(i==13) {temp=SPI_TransferByte(0xff);PORTC=temp;}
		//if(i!=13)SPI_TransferByte(0xff);
		switch (i)
		{
	      case 13: {SecPerClus=SPI_TransferByte(0xff); break;}  //ƫ����13 һ�ֽ� ÿ��������
		  case 14: {RsvdSecCnt=SPI_TransferByte(0xff); break;}  //ƫ����14 ���ֽ� ����������
		  case 15: {temp0=SPI_TransferByte(0xff);temp0<<=8;RsvdSecCnt|=temp0;break;}
		  case 36: {FAtSz32=SPI_TransferByte(0xff);break;}      //ƫ����36  ���ֽ� FAT��������
		  case 37: {temp1=SPI_TransferByte(0xff);temp1<<=8;FAtSz32|=temp1;break;}
		  case 38: {temp1=SPI_TransferByte(0xff);temp1<<=16;FAtSz32|=temp1;break;}
		  case 39: {temp1=SPI_TransferByte(0xff);temp1<<=24;FAtSz32|=temp1;break;}
		  case 44: {RootClus=SPI_TransferByte(0xff);break;}	    //ƫ����44 ���ֽ� ��Ŀ¼�غ�
		  case 45: {temp1=SPI_TransferByte(0xff);temp1<<=8;RootClus|=temp1;break;}
		  case 46: {temp1=SPI_TransferByte(0xff);temp1<<=16;RootClus|=temp1;break;}
		  case 47: {temp1=SPI_TransferByte(0xff);temp1<<=24;RootClus|=temp1;break; }
		  default:{SPI_TransferByte(0xff);}
		}
	}
	SPI_TransferByte(0xff);
	SPI_TransferByte(0xff);
	SD_Disable();
	/*�ҳ�file1~3��ȷ����ʼ��ַ*/
	
	if(SDHC==0) RootDirAddr=(FAtSz32*2+RsvdSecCnt)*512;  //����ȷ�ϸ�Ŀ¼����ʼ��ַ
	if(SDHC==1) RootDirAddr=(FAtSz32*2+RsvdSecCnt);
	
	SD_SendCommand(SD_READ_BLOCK,RootDirAddr,0xFF); //����CMD24 �趨��ַΪ��Ŀ¼��ַ ��ȡ��Ŀ¼����
	SD_Enable();
	//PORTC=SecPerClus;
	while ((SPI_TransferByte(0xff))!= 0xfe); //����SD���������ݱ�ʶ ��ʼ��������
	for(i=0;i<16;i++)                        //����FAT32�ļ�ϵͳ�ṹ����Ŀ¼��Ϊ32���ֽ�һ���ʾһ���ļ����ļ�������Ϊ�ļ����ĸ������� �����ļ������ļ���ʼ�صȵ�
	{                                           
		for(j=0;j<8;j++)
		{
			check_file_name[j]=SPI_TransferByte(0xff);   //��4��unsigned long���������ռ�32λ�ļ���������
			temp1=SPI_TransferByte(0xff);temp1<<=8;check_file_name[j]|=temp1;
			temp1=SPI_TransferByte(0xff);temp1<<=16;check_file_name[j]|=temp1;
			temp1=SPI_TransferByte(0xff);temp1<<=24;check_file_name[j]|=temp1;
		}
		if((check_file_name[0]==0x454c4946)&&((check_file_name[1]<<24)==0x31000000)) //ȷ��file1����ʼ��ַ
		 {   
			 FileClus=(check_file_name[5]<<16);
			 FileClus|=(check_file_name[6]>>16);
			 if(SDHC==0) File1Addr=RootDirAddr+(FileClus-RootClus)*512*SecPerClus;
			 if(SDHC==1) File1Addr=(RootDirAddr+(FileClus-RootClus)*SecPerClus);      
		 }
		 if((check_file_name[0]==0x454c4946)&&((check_file_name[1]<<24)==0x32000000))//ȷ��file2����ʼ��ַ
		 {
			 FileClus=(check_file_name[5]<<16);
			 FileClus|=(check_file_name[6]>>16);
			 if(SDHC==0) File2Addr=RootDirAddr+(FileClus-RootClus)*512*SecPerClus;
			 if(SDHC==1) File2Addr=(RootDirAddr+(FileClus-RootClus)*SecPerClus);
		 }
		 if((check_file_name[0]==0x454c4946)&&((check_file_name[1]<<24)==0x33000000))//ȷ��file3����ʼ��ַ
		 {
			 FileClus=(check_file_name[5]<<16);
			 FileClus|=(check_file_name[6]>>16);
			 if(SDHC==0) File3Addr=RootDirAddr+(FileClus-RootClus)*512*SecPerClus;
			 if(SDHC==1) File3Addr=(RootDirAddr+(FileClus-RootClus)*SecPerClus);
		 }
		
	}
	SPI_TransferByte(0xff);
	SPI_TransferByte(0xff);
	SD_Disable();
}
/*�ⲿ�жϷ������*/
ISR(INT6_vect)
{   
	if(w_timeup==0) //����ʱδ����ʱ���°�ť���ü�ʱֵ��ѡ������¼����ļ�
	{
		TCNT1=0;
		_delay_ms(50);
		w_f++;
		if(w_f==4) w_f=1;
		_delay_ms(50);
		
	}
	
	if(w_timeup==1) //����ʱ����ʱ���°�ť�ͽ�������¼��
	{
		button_stop=1;
		w_time=w_i;
		_delay_ms(50);
	}
}
/*��ʱ���жϷ������*/
ISR(TIMER1_OVF_vect)
{
	w_timeup=1; //��ʱ����
}
void main(void)
{   
	 unsigned int i=0,dis_h,dis0_h,dis1_h,dis_t,dis0_t,dis1_t,adc_data,temp;
	 unsigned char led=0;
	 
	 unsigned char T[]={"T:"};
	 unsigned char H[]={"H:"};
	 unsigned char AI[]={"AI:    v"};	 
	 unsigned char DI[]={"DI:"};	 
	 unsigned char dht_data_temp_t,dht_data_temp_h;
	 
	 unsigned char cnm[]={"STOP COLLECTING "};
	 unsigned char clr[]={"                "}; //������Ҫ�ı����Լ�LCD�Ļ�����ʾ����

	 
	
	 DDRD=0xff;
	 PORTD=0xff;
	 DDRG=0xff;
	 PORTG=0xff; //lcd�˿ڳ�ʼ��
	 
	 DDRC=0XFF;
	 PORTC=0X7f;  //��ˮ�ƶ˿ڳ�ʼ��
	 
	 
     SD_Port_Init();//SD���˿ڳ�ʼ��
     SD_reset();   //SD����λ
	 SD_Init(); //SD����ʼ��
	 SPCR=0X50;  //��ʼ���ٶ�д
	 SD_Set_CRC();  //����CRCУ��
	 SD_Set_data_size(); //�������ݿ��С
	 //SD_erase(w_addr,w_addr+52428800*2);
	 Set_SD_File_Addr();  //ȷ��file1~3��ַ
	if(SDHC==0)
	{
		SD_erase(File1Addr,File1Addr+52428800-512);
		SD_erase(File2Addr,File2Addr+52428800-512);
		SD_erase(File3Addr,File3Addr+52428800-512);//�����������
		
	}
	
	if(SDHC==1)
	{
		SD_erase(File1Addr,File1Addr+102399);
		SD_erase(File2Addr,File2Addr+102399);
		SD_erase(File3Addr,File3Addr+102399);//�����������
	}
	
	 
	 LcdInit(); //LCD��ʼ��
	 Ext_Int_Init();//�ⲿ�жϳ�ʼ��
	 TC1_Init();   //��ʱ����ʼ��
	 sei();    //��ȫ���ж�
	  
	 ADC_Init();  //adת����ʼ��
	   
	 while(w_timeup==0)  //��ʱδ����ʱ ��ʾ�ļ�ѡ����沢�ð�ť����ѡ��
	 {
		 unsigned char wait[]={"file choose"};
		 WriteChar(1,0,11,wait);
		 WriteNum(1,13,w_f+48);	
		 if(w_f==1) w_addr=File1Addr;
		 if(w_f==2) w_addr=File2Addr;
		 if(w_f==3) w_addr=File3Addr;
	 }
	 TIMSK&= ~(1<<TOIE1);//�ض�ʱ���ж�
	 LcdInit();  //����
	 while(w_timeup==1)//��ʱ���� ��ʼ�ռ��վ�
	 { 
		if(button_stop==0) //δ���½�����ťʱ
	    {
		 temp=(Collect_DHT_Data());
		 dht_data_temp_t=temp;
		 w_data_t[i]=data_to_dec(dht_data_temp_t);
	     dis_t=w_data_t[i];
	     dis0_t=dis_t%10;
	     dis1_t=dis_t/10;
	     WriteChar(1,0,2,T);
	     WriteNum(1,2,dis0_t+48);
	     WriteNum(1,3,dis1_t+48);
	     WriteNum(1,4,0xdf);
	     WriteNum(1,5,0x43);  //��ʾ�¶�����
		 
		 
		 dht_data_temp_h=(temp>>8);
		 w_data_h[i]=data_to_dec(dht_data_temp_h);
		 dis_h=w_data_h[i];
		 dis0_h=dis_h%10;
		 dis1_h=dis_h/10;
		 WriteChar(1,7,2,H);
		 WriteNum(1,9,dis0_h+48);
		 WriteNum(1,10,dis1_h+48);
		 WriteNum(1,11,0x25);//��ʾʪ������
		 
		 
		 
		 adc_data=Collect_ADC_Data();
		 w_data_adc_int[i]=(adc_data)/100;
		 w_data_adc_sn1[i]=(adc_data-(w_data_adc_int[i]*100))/10;
		 w_data_adc_sn2[i]=adc_data-(w_data_adc_int[i]*100)-w_data_adc_sn1[i]*10; 
		 WriteChar(2,0,8,AI);
	     WriteNum(2,3,(w_data_adc_int[i]+48));
		 WriteNum(2,4,0X2E);
		 WriteNum(2,5,(w_data_adc_sn1[i]+48));
		 WriteNum(2,6,(w_data_adc_sn2[i]+48));   //��ʾADC����
		 
		 w_data_button[i]=Collect_BUTTON_Data();
		 WriteChar(2,9,3,DI);
		 WriteNum(2,12,(w_data_button[i]+48)); //��ʾ��ť����
		  
	     PORTC=(~(0x01<<led));
	     led++;
	     if(led==8) led=0;  //������Ʊ�ʾ��������ִ�в����ڶ�ȡ����
		 
		 _delay_ms(500);
		 
	    }
		
	   
		 w_i=i;
		
	   
	    i++;
		
	   if(i==32)   //���ݹ�һ�����ݿ���վݺ� д��SD��
		{
		 PORTC=SD_Write();
		 if(SDHC==0)w_addr=w_addr+512;
		 if(SDHC==1)w_addr=w_addr+1;
		 i=0;  
		 _delay_ms(500);
		 if(button_stop==1) break;  //�����;���°�ť ��������
		}
	  
	  
	  
	}  
		 WriteChar(1,0,16,cnm);
		 WriteChar(2,0,16,clr);

}
	   
	   
	 
	 
	 
	 
	 
	 
	 

	 
	 
	 
	 

