#define uchar unsigned char 
#define uint unsigned int 

#define RS 2
#define RW 3
#define EN 4



//��æ
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
		DDRD=0x00;      //A�ڱ�����
		PORTD=0xff;     //����ʹ��
		_delay_us(500);
		temp = PIND&0x80;    //��ȡA��
		_delay_us(500);      
		DDRD=0xff;      
		PORTD=0xff;        //A�ڱ����
		_delay_us(500);
		PORTG&=~(1<<EN);   //EN=0
		_delay_us(500);
	}
}

//дָ��
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
	PORTD = com;       //���ָ��
	_delay_us(500);
	PORTG&=~(1<<EN);   //EN=0
	_delay_us(500);
}

//1602��ʼ��
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

//д����
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
	PORTD = data;      //�������
	_delay_us(500);
	PORTG&=~(1<<EN);   //EN=0
	_delay_us(500);
}


//������
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
	DDRD=0x00;       //A�˿ڱ�����
	_delay_us(500);
	temp = PIND;     //��A�˿�
	_delay_us(500);
	DDRD=0xff;       //A�˿ڱ����
	_delay_us(500);
	PORTG&=~(1<<EN); //EN=0
	_delay_us(500);
	return temp;	
}

//=================================================
// ������ дLCD�ڲ�CGRAM����
// ��ڣ� ��num��Ҫд�����ݸ���
//        ��pbuffer��Ҫд�����ݵ��׵�ַ
// ���ڣ� ��
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
//������д�˵�������������ʹ�õ�LCD���Ϊ 16 * 2
//��ڣ��˵������׵�ַ
//���ڣ���
//=================================================
void	WriteMenu(const uchar *pBuffer)
{
	uchar	i,t;
	writecom(0x80);   //���ݵ�ַ
	
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
// ������������λ��д���ֺ���
// ��ڣ���row����ʾҪд�������ڵ��е�ַ��ֻ��Ϊ1��2
//       ��col����ʾҪд�������ڵ��е�ַ��ֻ��Ϊ0--15
//		 ��num����ʾҪд�����֣�ֻ��Ϊ0--9
// ���ڣ���
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
// ������������λ��д�������ַ�
// ��ڣ���row��Ҫд���ַ����ڵ��У�ֻ��Ϊ1��2��
//       ��col��Ҫд���ַ����ڵ��У�ֻ��Ϊ0---15
//       ��num��Ҫд�ַ��ĸ���
//       ��pbuffer��Ҫд�ַ����׵�ַ
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
