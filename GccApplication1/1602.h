#define uchar unsigned char 
#define uint unsigned int 

#define RS 2
#define RW 3
#define EN 4



//查忙
void busy(void)
{
    uchar temp;
	_delay_us(500);
	PORTG&=~(1<<RS);    //RS=0
	_delay_us(500);
	PORTG|=(1<<RW);     //RW=1
	_delay_us(500);
	while(temp)
	{
		PORTG|=(1<<EN); //EN=1
		_delay_us(500);
		DDRD=0x00;      //A口变输入
		PORTD=0xff;     //上拉使能
		_delay_us(500);
		temp = PIND&0x80;    //读取A口
		_delay_us(500);      
		DDRD=0xff;      
		PORTD=0xff;        //A口变输出
		_delay_us(500);
		PORTG&=~(1<<EN);   //EN=0
		_delay_us(500);
	}
}

//写指令
void writecom(uchar	com)
{
	busy();
	_delay_us(500);
	PORTG&=~(1<<RS);   //RS=0
	_delay_us(500);
	PORTG&=~(1<<RW);   //RW=0
	_delay_us(500);
	PORTG|=(1<<EN);    //EN=1
	_delay_us(500);
	PORTD = com;       //输出指令
	_delay_us(500);
	PORTG&=~(1<<EN);   //EN=0
	_delay_us(500);
}

//1602初始化
void	LcdInit(void)
{
	writecom(0x38);
	_delay_us(1000);
	writecom(0x01);
	_delay_us(10000);
	_delay_us(1000);
	_delay_us(1000);
	_delay_us(1000);
	_delay_us(1000);
	_delay_us(1000);
	_delay_us(1000);
	writecom(0x02);
	_delay_us(1000);
	writecom(0x06);
	_delay_us(1000);
	writecom(0x0c);
	_delay_us(1000);
	writecom(0x38);	
	_delay_us(1000);
}	

//写数据
void	writedata(uchar data)
{
	busy();
	_delay_us(500);
	PORTG|=(1<<RS);   //RS=1
	_delay_us(500);
	PORTG&=~(1<<RW);   //RW=0
	_delay_us(500);
	PORTG|=(1<<EN);    //EN=1
	_delay_us(500);
	PORTD = data;      //输出数据
	_delay_us(500);
	PORTG&=~(1<<EN);   //EN=0
	_delay_us(500);
}


//读数据
uchar	readdata(void)
{
	uchar temp;
	busy();
	_delay_us(500);
	PORTG|=(1<<RS);  //RS=1
	_delay_us(500);
	PORTG|=(1<<RW);  //RW=1
	_delay_us(500);
	PORTG|=(1<<EN);  //EN=1
	_delay_us(500);
	DDRD=0x00;       //A端口变输入
	_delay_us(500);
	temp = PIND;     //读A端口
	_delay_us(500);
	DDRD=0xff;       //A端口变输出
	_delay_us(500);
	PORTG&=~(1<<EN); //EN=0
	_delay_us(500);
	return temp;	
}

//=================================================
// 描述： 写LCD内部CGRAM函数
// 入口： ‘num’要写的数据个数
//        ‘pbuffer’要写的数据的首地址
// 出口： 无
//================================================
void	WriteCGRAM(uint	num, const uint	*pBuffer)
{
	uint	i,t;
	writecom(0x40);
	PORTG|=(1<<RS);
	PORTG&=~(1<<RW);
	for(i=num;i!=0;i--)
	{
		t = *pBuffer;
		PORTG|=(1<<EN);
		PORTD = t;
		PORTG&=~(1<<EN);				
		pBuffer++;
	}
	
}

//=================================================
//描述：写菜单函数，本程序使用的LCD规格为 16 * 2
//入口：菜单数组首地址
//出口：无
//=================================================
void	WriteMenu(const uchar *pBuffer)
{
	uchar	i,t;
	writecom(0x80);   //数据地址
	
	PORTG|=(1<<RS);
	PORTG&=~(1<<RW);
	_delay_us(50);
	for(i=0;i<16;i++)
	{
		t = *pBuffer;
		PORTD = t;
		PORTG|=(1<<EN);
		_delay_us(50);
		PORTG&=~(1<<EN);				
		pBuffer++;
	}
	writecom(0xC0);

	PORTG|=(1<<RS);
	PORTG&=~(1<<RW);
	_delay_us(50);	
	for(i=0;i<16;i++)
	{
		t = *pBuffer;
		PORTD = t;
		PORTG|=(1<<EN);
		_delay_us(50);
		PORTG&=~(1<<EN);				
		pBuffer++;
	}
}
//====================================================
// 描述：在任意位置写数字函数
// 入口：’row‘表示要写数字所在的行地址，只能为1或2
//       ’col‘表示要写数字所在的列地址，只能为0--15
//		 ‘num’表示要写的数字，只能为0--9
// 出口：无
//===================================================
void WriteNum(uchar row,uchar col,uchar num)
{
	if (row == 1)	row = 0x80 + col;
	else	row = 0xC0 + col;
	writecom(row);

	PORTG|=(1<<RS);
	_delay_us(500);
	PORTG&=~(1<<RW);
	_delay_us(500);
	PORTD = num;
	_delay_us(500);
	PORTG|=(1<<EN);
	_delay_us(500);
	PORTG&=~(1<<EN);	
	_delay_us(500);			
}
//================================================================
// 描述：在任意位置写任意多个字符
// 入口：’row‘要写的字符所在的行，只能为1或2；
//       ‘col’要写的字符所在的列，只能为0---15
//       ‘num’要写字符的个数
//       ‘pbuffer’要写字符的首地址
//================================================================== 
void	WriteChar(uchar row,uchar col,uint num,uchar *pBuffer)
{
	uchar i,t;
	if (row == 1)	row = 0x80 + col;
	else	row = 0xC0 + col;
	writecom(row);


	PORTG|=(1<<RS);
	_delay_us(500);
	PORTG&=~(1<<RW);
	_delay_us(500);
	for(i=num;i!=0;i--)
	{
		t = *pBuffer;
		_delay_us(500);
		PORTD = t;
		_delay_us(500);
		PORTG|=(1<<EN);
		_delay_us(500);
		PORTG&=~(1<<EN);		
		_delay_us(500);		
		pBuffer++;
	}
	
}
