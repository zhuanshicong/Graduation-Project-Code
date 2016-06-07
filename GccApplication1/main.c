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
/*定义需要的全局变量，包括要采集的数据、SD卡地址、必要中间变量等等*/
unsigned int w_data_t[128],w_data_h[128],w_time=512,w_i,w_f=1,w_timeup=0;
unsigned char w_data_adc_int[128],w_data_adc_sn1[128],w_data_adc_sn2[128],w_data_button[128],SDHC;
unsigned long w_addr,File1Addr,File2Addr,File3Addr;
unsigned char button_stop=0;
/* SD卡SPI模式所需端口的初始化*/
void SD_Port_Init(void)
{      
      SD_DDR=(1<<SD_MOSI)|(1<<SD_SCK)|(1<<SD_SS)|(1<<PB0); //定义MOSI，SCK，SS为输出
	  SD_DDR&= ~(1<<SD_MISO);  //定义MISO为输入
	  
	  SD_SCK_H;
	  SD_SS_H;    //拉高SS以及SCK端口
	  PORTB|= (1<<PB0); //此行为拉高单片机作为从机时的选片端口，单片机作为主机时，此端口不作为选片使用，避免各种出错
	  
	  SPCR=0x53;                         
      SPSR=0x00;     //同时设置SPI寄存器使其在SD卡初始化时进行慢速初始化
	                       
}
/* DHT11温湿度传感器初始化*/
void DHT_Init(void)
{   
	/* 根据DHT11使用说明，需要主机把总线拉低必须大于18MS,保证DHT11能检测到起始信号，然后再拉高20~40us，等待DHT11应答*/
	DHT_DATA_O;  //设置DHT数据端口为输出
	DHT_L;       //拉低DHT数据端口
	_delay_ms(20);
	DHT_H;       //拉高DHT数据端口      
	_delay_us(30);
	DHT_DATA_I; //设置DHT数据端为输入，等待DHT11应答
	while(CHECK_DHT_H);//接收到DHT11拉高数据端口时，说明初始化完成
}
/* 外部中断初始化 */
void Ext_Int_Init(void)
{
	DDRE&= ~(1<<PE6);  
	PORTE&= ~(1<<PE6);  //使用PE6端口并且设置为输入并拉高
	EICRB=(1<<ISC61)|(1<<ISC60); //设置为下降沿产生中断
	EIMSK|=(1<<INT6); //开启PE6口外部中断功能
	
}
/* 定时器初始化 */
void TC1_Init(void)
{
	TCCR1A=0x00;  //设置为普通模式
	TCCR1B=(1<<CS10)|(1<<CS12);//设置为1024分频
	TCCR1C=0x00;
	TCNT1=0; //定时器1初始值
	TIMSK|=(1<<TOIE1); //开启定时器1中断
}
/* AD转换初始化 */
void ADC_Init(void)
{
	ADMUX=(1<<REFS0); //选择参考电压为AVCC 并且选择ADC0口作为模数转换端
	ADCSRA=(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);//选择128分频 并且启动AD转换
}
/*SPI模式发送1BYTE数据函数*/
unsigned char SPI_TransferByte(unsigned char byte)
{
SPDR = byte; //将一字节数据SPI数据寄存器
while (!(SPSR&(1<<SPIF)));//等待传输完成                                            
return SPDR;//返回接收到的数据
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
/*向SD卡发送命令函数*/
unsigned char SD_SendCommand(unsigned char cmd, unsigned long arg,unsigned char CRC)
{
	unsigned char r1;
	unsigned char retry1 = 0;      	//重复操作次数

	SD_Enable();                        	//使能片选信号

	SPI_TransferByte(cmd | 0x40);   	//分别写入命令
	SPI_TransferByte(arg >> 24);      	//数据段第4字节
	SPI_TransferByte(arg >> 16);      	//数据段第3字节
	SPI_TransferByte(arg >> 8);       	//数据段第2字节
	SPI_TransferByte(arg);          	//数据段第1字节
	SPI_TransferByte(CRC);         	//CRC效验

	while ((r1 = SPI_TransferByte(0xff)) == 0xff) //等待响应
		if (retry1++ > 200) break; 	//超时退出

	SD_Disable();                        	//清初片选信号
	return r1;                   	//返回状态值 这里我默认接收的是响应的第一个字节 即R1响应 如发送的命令响应回来的不是R1 需要再次开启片选并发送SPI_TransferByte(0XFF)接收剩下的响应数据
}
/*检查SD卡是否为SDHC卡函数*/
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
/*SD卡复位函数*/
unsigned char SD_reset(void)
{     
	 unsigned char r1,i;
       
	   SD_Disable();
	   
       for (i=0;i<200;i++)          
       {
            SPI_TransferByte(0xff); 
       }                                  //根据SD卡协议，确保SD正确复位需要延迟74个以上的时钟
	                                      
	   
	r1=SD_SendCommand(SD_RESET,0,0x95);   //发送复位命令，CRC校验码固定位0X95
	SPI_TransferByte(0xff);	 	
		return r1;                        //返回响应代码
		
}
/*SD卡初始化函数*/
unsigned char SD_Init(void)
{  
    unsigned char temp;
	unsigned int  i;
	unsigned long SD_Init_Argument;
     temp=SD_SendCommand(SD_SEND_IF_COND,0X000001AA,0x87);  //根据SD卡协议，初始化SD卡需要先发送CMD8确定SD卡是否2.0协议并确认SD卡使用的电压（默认3.3）和该卡是否可用
	 for(i=0;i<20;i++)
	   {SPI_TransferByte(0xff);}
		SD_Check_If_SDHC();
		if(SDHC==0)SD_Init_Argument=0x00000000;
		if(SDHC==1)SD_Init_Argument=0x40000000;
     i=0;
	 while(temp!=0x00)   //开始发送真正的初始化命令
	 {
	  temp=SD_SendCommand(SD_APP_CMD,0,0xff);  //先发送CMD58启用SD卡识别ACMD命令
	  temp=SD_SendCommand(SD_SEND_OP_COND,SD_Init_Argument,0xff);  //再发送ACMD41命令
	  i++;        
	  if(i>200)  break;           
	 }
	 SPI_TransferByte(0xff);
	 PORTC=temp;
     return (temp);                 //返回R1响应为0X00确认初始化成功
}
/*SD卡设置数据块大小函数*/
unsigned char SD_Set_data_size(void)
{
 unsigned char temp;
 
 temp=SD_SendCommand(SD_SET_BLOCKLEN,512,0xff); //发送CMD16命令设置数据块大小，通常为了方便数据处理，都将其值设置成512字节即一个扇区的大小

 SPI_TransferByte(0xff);
 return temp;
}
/*SD卡设置是否启用CRC校验函数*/
unsigned char SD_Set_CRC(void)
{ 
 unsigned  char temp;
 
 temp=SD_SendCommand(SD_CRC_ON_OFF,0,0x95);//发送CMD59，并忽略CRC校验
 
 SPI_TransferByte(0xff);
 return temp;
}
/*检查SD卡当前状态函数*/
unsigned char SD_Check_status(void)
{ 
  unsigned char temp1,temp2;
  temp1=SD_SendCommand(SD_SEND_STATUS,0,0XFF);//发送CMD13，并将R2第一字节存入temp1
  SD_Enable();
  temp2=SPI_TransferByte(0xff);  //R2第二字节存入temp2
  SD_Disable();
  return temp2;
}
/*SD卡擦除函数*/
void SD_erase(unsigned long addr1,unsigned long addr2)
{   
	unsigned char temp;
	temp=SD_SendCommand(SD_EARSE_WR_BLK_START_ADDR,addr1,0xff); //发送cmd32设定SD卡擦除起始地址
	temp=SD_SendCommand(SD_EARSE_WR_BLK_END_ADDR,addr2,0xff);   //发送cmd33设定SD卡擦除结束地址
	temp=SD_SendCommand(SD_EARSE,0x00,0xff);                   // 发送CMD38执行擦除操作
	SD_Enable();
	while(temp==0x00)
	{temp=SPI_TransferByte(0xff);
	 PORTC=temp;}
	SD_Disable();
	
}
/*DHT11温湿度数据收集函数*/
unsigned int  Collect_DHT_Data(void)
 {  
	unsigned char dht_data[5];
	unsigned char i,j;
	unsigned char Ccode,temp_l,temp_h,hum_l,hum_h;
	unsigned int final_dht_data;
	
	DHT_Init();    //每次采集数据都要先执行DHT11传感器的初始化
	while(!CHECK_DHT_H);  //初始化后DHT11会有80us的低电平响应信号 等待响应完成
	while(CHECK_DHT_H);   //响应完成后 DHT11拉高80us 等待完成
	for(i=0;i<5;i++)     //开始收集收据
	{ 
		dht_data[i]=0x00;
		for(j=0;j<8;j++)
		{ 
			while(!CHECK_DHT_H);  //等待上一bit数据高电平结束
			_delay_us(50);       //延迟50us
			if(CHECK_DHT_H)     // 如果此时信号依然为高，那么代表此位数据应为1
			{
			 dht_data[i]|=(1<<(7-j));
			}
			while(CHECK_DHT_H);
		}
	  	
	}
	DHT_DATA_O;   
	DHT_H;     //停止采集 设置端口为输出切上拉
	Ccode=(dht_data[0]+dht_data[1]+dht_data[2]+dht_data[3]);//得到数据检验码
	if(Ccode==dht_data[4]) //检查数据无误后获得收据
	{
	hum_h=dht_data[0];
	hum_l=dht_data[1];
	temp_h=dht_data[2];
	temp_l=dht_data[3];
	}
	final_dht_data=hum_h;  //收据处理
	final_dht_data<<=8;
	final_dht_data|=temp_h;
	  
	return final_dht_data; //返回得到数据，16bit 高8位为湿度，第8位为温度
 }
 /*收集数模转换数据函数*/
unsigned char Collect_ADC_Data(void)
{ 
  unsigned int adc_data,adc_l,adc_h;
  
  ADCSRA|=(1<<ADSC);   //开始AD转换
  while(!(ADCSRA & 0x10));  //等待转换结束
  _delay_us(500);
  adc_l=ADCL;          //读取低位数据
  adc_h=ADCH;           //读取高位数据
  adc_data=adc_h<<8|adc_l;
  adc_data=adc_data>>1;               //放弃一位的精度
  adc_data-=35;                        //修正
  ADCSRA&=~(1<<ADIF);    //清标志
  return adc_data;      //返回转换后数据
}
/*判断按钮是否按下函数*/
unsigned char Collect_BUTTON_Data(void)
{   
	unsigned char button;
	DDRE&= ~(1<<PE5);
	PORTE&= ~(1<<PE5);
	_delay_ms(10);
	if ((PINE&(1<<PE5))==0) button=0;  //按下显示0
	if (!(PINE&(1<<PE5))==0) button=1; //没按下显示1
	_delay_ms(10);
	return button;
}
/*温度数据转换为两位数*/
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
/*SD卡写入数据函数*/
unsigned char SD_Write(void)
{ 
 unsigned char temp;
 unsigned int i,t0,t1,h0,h1,stop_temp;
 unsigned long addr;
 //addr=420048<<9;
 addr=w_addr;
 
 SD_SendCommand(SD_WRITE_BLOCK,addr,0xFF);  //发送CMD24命令，指定开始写入的地址
 
 SD_Enable();
 
 SPI_TransferByte(0xff);  
 SPI_TransferByte(0xff);
 SPI_TransferByte(0xff);
 SPI_TransferByte(0xff);
 SPI_TransferByte(0xff);
 SPI_TransferByte(0xff); //适当延迟一定时间 避免错误
 
 SPI_TransferByte(0xfe);  //发送数据头标识确认开始录入数据
 
   
	  for (i=0;i<32;i++)
      {  
		if(i<=w_time||button_stop==0)
		 {
			 t0=w_data_t[i]%10;
		     t1=w_data_t[i]/10;
		     SPI_TransferByte(t0+48);
		     SPI_TransferByte(t1+48);  //录入温度数据
			 
			 SPI_TransferByte(0x00);
			 
			 h0=w_data_h[i]%10;
			 h1=w_data_h[i]/10;
			 SPI_TransferByte(h0+48);  
			 SPI_TransferByte(h1+48);  //录入湿度数据
			 
			 SPI_TransferByte(0x00);
			 
			 
			 SPI_TransferByte(w_data_adc_int[i]+48);
			 SPI_TransferByte(0x2E);
			 SPI_TransferByte(w_data_adc_sn1[i]+48);
			 SPI_TransferByte(w_data_adc_sn2[i]+48); //录入adc数据
			 
			 SPI_TransferByte(0x00);
			 
			 SPI_TransferByte(0x00);
			 SPI_TransferByte(w_data_button[i]+48);  //录入按钮数据
			 
		 }
		 
	    if(i>w_time)
		 {  
			  for(stop_temp=0;stop_temp<13;stop_temp++)
		     { 
				 SPI_TransferByte(0x00);  //停止键按下时候的空数据
			 }
		 }
		SPI_TransferByte(0);
	    SPI_TransferByte(0);
		SPI_TransferByte(0);
	  }
 
  
 
 SPI_TransferByte(0x95);
 SPI_TransferByte(0x95);
 
 while ((temp = SPI_TransferByte(0xff)) == 0xff);//等待传输结束
 //temp=SPI_TransferByte(0xff); 
 //while (!((temp&0x0f) == 5));
 while (!(SPI_TransferByte(0xff)));
 SD_Disable();
 return temp;
}
/*确认SD卡文件内地址函数*/
void Set_SD_File_Addr(void)
{   
	unsigned int i,j,RsvdSecCnt,temp0;
	unsigned char SecPerClus,temp;
	unsigned long RootClus,FAtSz32,RootDirAddr,temp1,check_file_name[8],FileClus;
	/*读引导扇区获得每簇扇区数，保留扇区数，fat表扇区数，根目录簇号*/
	SD_SendCommand(SD_READ_BLOCK,0x00000000,0xFF); //发送CMD17 设置地址为0x00000000即SD卡的引导扇区
	SD_Enable(); 
	
	while ((SPI_TransferByte(0xff))!= 0xfe); //接收SD卡开始发送数据标志
	for(i=0;i<512;i++)
	{    
		//if(i==13) {temp=SPI_TransferByte(0xff);PORTC=temp;}
		//if(i!=13)SPI_TransferByte(0xff);
		switch (i)
		{
	      case 13: {SecPerClus=SPI_TransferByte(0xff); break;}  //偏移量13 一字节 每簇扇区数
		  case 14: {RsvdSecCnt=SPI_TransferByte(0xff); break;}  //偏移量14 两字节 保留扇区数
		  case 15: {temp0=SPI_TransferByte(0xff);temp0<<=8;RsvdSecCnt|=temp0;break;}
		  case 36: {FAtSz32=SPI_TransferByte(0xff);break;}      //偏移量36  四字节 FAT表扇区数
		  case 37: {temp1=SPI_TransferByte(0xff);temp1<<=8;FAtSz32|=temp1;break;}
		  case 38: {temp1=SPI_TransferByte(0xff);temp1<<=16;FAtSz32|=temp1;break;}
		  case 39: {temp1=SPI_TransferByte(0xff);temp1<<=24;FAtSz32|=temp1;break;}
		  case 44: {RootClus=SPI_TransferByte(0xff);break;}	    //偏移量44 四字节 根目录簇号
		  case 45: {temp1=SPI_TransferByte(0xff);temp1<<=8;RootClus|=temp1;break;}
		  case 46: {temp1=SPI_TransferByte(0xff);temp1<<=16;RootClus|=temp1;break;}
		  case 47: {temp1=SPI_TransferByte(0xff);temp1<<=24;RootClus|=temp1;break; }
		  default:{SPI_TransferByte(0xff);}
		}
	}
	SPI_TransferByte(0xff);
	SPI_TransferByte(0xff);
	SD_Disable();
	/*找出file1~3的确切起始地址*/
	
	if(SDHC==0) RootDirAddr=(FAtSz32*2+RsvdSecCnt)*512;  //首先确认根目录的起始地址
	if(SDHC==1) RootDirAddr=(FAtSz32*2+RsvdSecCnt);
	
	SD_SendCommand(SD_READ_BLOCK,RootDirAddr,0xFF); //发送CMD24 设定地址为根目录地址 读取根目录数据
	SD_Enable();
	//PORTC=SecPerClus;
	while ((SPI_TransferByte(0xff))!= 0xfe); //接收SD卡发送数据标识 开始接收数据
	for(i=0;i<16;i++)                        //根据FAT32文件系统结构，根目录表为32个字节一组表示一个文件（文件夹亦视为文件）的各种属性 包括文件名，文件起始簇等等
	{                                           
		for(j=0;j<8;j++)
		{
			check_file_name[j]=SPI_TransferByte(0xff);   //用4个unsigned long型数据来收集32位文件属性数据
			temp1=SPI_TransferByte(0xff);temp1<<=8;check_file_name[j]|=temp1;
			temp1=SPI_TransferByte(0xff);temp1<<=16;check_file_name[j]|=temp1;
			temp1=SPI_TransferByte(0xff);temp1<<=24;check_file_name[j]|=temp1;
		}
		if((check_file_name[0]==0x454c4946)&&((check_file_name[1]<<24)==0x31000000)) //确认file1的起始地址
		 {   
			 FileClus=(check_file_name[5]<<16);
			 FileClus|=(check_file_name[6]>>16);
			 if(SDHC==0) File1Addr=RootDirAddr+(FileClus-RootClus)*512*SecPerClus;
			 if(SDHC==1) File1Addr=(RootDirAddr+(FileClus-RootClus)*SecPerClus);      
		 }
		 if((check_file_name[0]==0x454c4946)&&((check_file_name[1]<<24)==0x32000000))//确认file2的起始地址
		 {
			 FileClus=(check_file_name[5]<<16);
			 FileClus|=(check_file_name[6]>>16);
			 if(SDHC==0) File2Addr=RootDirAddr+(FileClus-RootClus)*512*SecPerClus;
			 if(SDHC==1) File2Addr=(RootDirAddr+(FileClus-RootClus)*SecPerClus);
		 }
		 if((check_file_name[0]==0x454c4946)&&((check_file_name[1]<<24)==0x33000000))//确认file3的起始地址
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
/*外部中断服务程序*/
ISR(INT6_vect)
{   
	if(w_timeup==0) //当计时未结束时按下按钮重置计时值并选择数据录入的文件
	{
		TCNT1=0;
		_delay_ms(50);
		w_f++;
		if(w_f==4) w_f=1;
		_delay_ms(50);
		
	}
	
	if(w_timeup==1) //当计时结束时按下按钮就结束数据录入
	{
		button_stop=1;
		w_time=w_i;
		_delay_ms(50);
	}
}
/*计时器中断服务程序*/
ISR(TIMER1_OVF_vect)
{
	w_timeup=1; //计时结束
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
	 unsigned char clr[]={"                "}; //设置需要的变量以及LCD的基本显示内容

	 
	
	 DDRD=0xff;
	 PORTD=0xff;
	 DDRG=0xff;
	 PORTG=0xff; //lcd端口初始化
	 
	 DDRC=0XFF;
	 PORTC=0X7f;  //流水灯端口初始化
	 
	 
     SD_Port_Init();//SD卡端口初始化
     SD_reset();   //SD卡复位
	 SD_Init(); //SD卡初始化
	 SPCR=0X50;  //开始高速读写
	 SD_Set_CRC();  //屏蔽CRC校验
	 SD_Set_data_size(); //设置数据块大小
	 //SD_erase(w_addr,w_addr+52428800*2);
	 Set_SD_File_Addr();  //确认file1~3地址
	if(SDHC==0)
	{
		SD_erase(File1Addr,File1Addr+52428800-512);
		SD_erase(File2Addr,File2Addr+52428800-512);
		SD_erase(File3Addr,File3Addr+52428800-512);//清除已有数据
		
	}
	
	if(SDHC==1)
	{
		SD_erase(File1Addr,File1Addr+102399);
		SD_erase(File2Addr,File2Addr+102399);
		SD_erase(File3Addr,File3Addr+102399);//清除已有数据
	}
	
	 
	 LcdInit(); //LCD初始化
	 Ext_Int_Init();//外部中断初始化
	 TC1_Init();   //定时器初始化
	 sei();    //开全局中断
	  
	 ADC_Init();  //ad转换初始化
	   
	 while(w_timeup==0)  //计时未结束时 显示文件选择界面并用按钮进行选择
	 {
		 unsigned char wait[]={"file choose"};
		 WriteChar(1,0,11,wait);
		 WriteNum(1,13,w_f+48);	
		 if(w_f==1) w_addr=File1Addr;
		 if(w_f==2) w_addr=File2Addr;
		 if(w_f==3) w_addr=File3Addr;
	 }
	 TIMSK&= ~(1<<TOIE1);//关定时器中断
	 LcdInit();  //清屏
	 while(w_timeup==1)//计时结束 开始收集收据
	 { 
		if(button_stop==0) //未按下结束按钮时
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
	     WriteNum(1,5,0x43);  //显示温度数据
		 
		 
		 dht_data_temp_h=(temp>>8);
		 w_data_h[i]=data_to_dec(dht_data_temp_h);
		 dis_h=w_data_h[i];
		 dis0_h=dis_h%10;
		 dis1_h=dis_h/10;
		 WriteChar(1,7,2,H);
		 WriteNum(1,9,dis0_h+48);
		 WriteNum(1,10,dis1_h+48);
		 WriteNum(1,11,0x25);//显示湿度数据
		 
		 
		 
		 adc_data=Collect_ADC_Data();
		 w_data_adc_int[i]=(adc_data)/100;
		 w_data_adc_sn1[i]=(adc_data-(w_data_adc_int[i]*100))/10;
		 w_data_adc_sn2[i]=adc_data-(w_data_adc_int[i]*100)-w_data_adc_sn1[i]*10; 
		 WriteChar(2,0,8,AI);
	     WriteNum(2,3,(w_data_adc_int[i]+48));
		 WriteNum(2,4,0X2E);
		 WriteNum(2,5,(w_data_adc_sn1[i]+48));
		 WriteNum(2,6,(w_data_adc_sn2[i]+48));   //显示ADC数据
		 
		 w_data_button[i]=Collect_BUTTON_Data();
		 WriteChar(2,9,3,DI);
		 WriteNum(2,12,(w_data_button[i]+48)); //显示按钮数据
		  
	     PORTC=(~(0x01<<led));
	     led++;
	     if(led==8) led=0;  //用跑马灯表示程序正在执行并正在读取数据
		 
		 _delay_ms(500);
		 
	    }
		
	   
		 w_i=i;
		
	   
	    i++;
		
	   if(i==32)   //数据够一个数据块的收据后 写入SD卡
		{
		 PORTC=SD_Write();
		 if(SDHC==0)w_addr=w_addr+512;
		 if(SDHC==1)w_addr=w_addr+1;
		 i=0;  
		 _delay_ms(500);
		 if(button_stop==1) break;  //如果中途按下按钮 跳出程序
		}
	  
	  
	  
	}  
		 WriteChar(1,0,16,cnm);
		 WriteChar(2,0,16,clr);

}
	   
	   
	 
	 
	 
	 
	 
	 
	 

	 
	 
	 
	 

