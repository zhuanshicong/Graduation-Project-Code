/*
 * CFile1.c
 *
 * Created: 2016/4/10 20:46:26
 *  Author: zhuanshicong
 */ 
struct FAT32_DBR
{
	unsigned char BS_jmpBoot[3];    //��תָ��            offset: 0
	unsigned char BS_OEMName[8];  //                    offset: 3
	unsigned char BPB_BytesPerSec[2];//ÿ�����ֽ���        offset:11
	unsigned char BPB_SecPerClus[1]; //ÿ��������          offset:13
	unsigned char BPB_RsvdSecCnt[2]; //����������Ŀ        offset:14
	unsigned char BPB_NumFATs[1];  //�˾���FAT����      offset:16
	unsigned char BPB_RootEntCnt[2]; //FAT32Ϊ0           offset:17
	unsigned char BPB_TotSec16[2];   //FAT32Ϊ0           offset:19
	unsigned char BPB_Media[1];     //�洢����            offset:21
	unsigned char BPB_FATSz16[2];    //FAT32Ϊ0          offset:22
	unsigned char BPB_SecPerTrk[2];  //�ŵ�������          offset:24
	unsigned char BPB_NumHeads[2];   //��ͷ��             offset:26
	unsigned char BPB_HiddSec[4];    //FAT��ǰ��������    offset:28
	unsigned char BPB_TotSec32[4];   //�þ���������        offset:32
	unsigned char BPB_FATSz32[4];    //һ��FAT��������   offset:36
	unsigned char BPB_ExtFlags[2];   //FAT32����           offset:40
	unsigned char BPB_FSVer[2];      //FAT32����          offset:42
	unsigned char BPB_RootClus[4];   //��Ŀ¼�غ�          offset:44
	unsigned char FSInfo[2];         //��������FSINFO������offset:48
	unsigned char BPB_BkBootSec[2];  //ͨ��Ϊ6            offset:50
	unsigned char BPB_Reserved[12];  //��չ��              offset:52
	unsigned char BS_DrvNum[1];      //                   offset:64
	unsigned char BS_Reserved1[1];   //                    offset:65
	unsigned char BS_BootSig[1];     //                    offset:66
	unsigned char BS_VolID[4];       //                    offset:67
	unsigned char BS_FilSysType[11]; //                   offset:71
	unsigned char BS_FilSysType1[8]; //"FAT32    "         offset:82
};

unsigned long lb2bb(unsigned char *dat,unsigned char len) //С��תΪ���
{
	unsigned long temp=0;
	unsigned long fact=1;
	unsigned char i=0;
	for(i=0;i<len;i++)
	{
		temp+=dat[i]*fact;
		fact*=256;
	}
	return temp;
}


	
	struct FAT32_Init_Arg 
{ 
 unsigned char BPB_Sector_No;    //BPB���������� 
 unsigned long Total_Size;         //���̵������� 
 unsigned long FirstDirClust;      //��Ŀ¼�Ŀ�ʼ�� 
 unsigned long FirstDataSector;  //�ļ����ݿ�ʼ������ 
 unsigned int  BytesPerSector;  //ÿ���������ֽ���
 unsigned int  FATsectors;        //FAT����ռ������
 unsigned int  SectorsPerClust;  //ÿ�ص�������
 unsigned long FirstFATSector;  //��һ��FAT����������
 unsigned long FirstDirSector;  //��һ��Ŀ¼��������
 unsigned long RootDirSectors;  //��Ŀ¼��ռ������
 unsigned long RootDirCount;  //��Ŀ¼�µ�Ŀ¼���ļ���
 };

void FAT32_Init(struct FAT32_Init_Arg *arg)
{
	struct FAT32_BPB *bpb=(struct FAT32_BPB *)(FAT32_Buffer); //�����ݻ�����ָ��תΪstruct FAT32_BPB ��ָ��
	arg->BPB_Sector_No   =FAT32_FindBPB();        //FAT32_FindBPB()���Է���BPB���ڵ�������
	arg->Total_Size      =FAT32_Get_Total_Size();     //FAT32_Get_Total_Size()���Է��ش��̵�����������λ����
	arg->FATsectors      =lb2bb((bpb->BPB_FATSz32)    ,4);   //װ��FAT��ռ�õ���������FATsectors��
	arg->FirstDirClust   =lb2bb((bpb->BPB_RootClus)   ,4);      //װ���Ŀ¼�غŵ�FirstDirClust��
	arg->BytesPerSector  =lb2bb((bpb->BPB_BytesPerSec),2);    //װ��ÿ�����ֽ�����BytesPerSector��
	arg->SectorsPerClust =lb2bb((bpb->BPB_SecPerClus) ,1);      //װ��ÿ����������SectorsPerClust ��
	arg->FirstFATSector=lb2bb((bpb->BPB_RsvdSecCnt) ,2)+arg->BPB_Sector_No;
	//װ���һ��FAT�������ŵ�FirstFATSector ��
	arg->RootDirCount    =lb2bb((bpb->BPB_RootEntCnt) ,2);  //װ���Ŀ¼������RootDirCount��
	arg->RootDirSectors  =(arg->RootDirCount)*32>>9;   //װ���Ŀ¼ռ�õ���������RootDirSectors��
	arg->FirstDirSector=(arg->FirstFATSector)+(bpb->BPB_NumFATs[0])*(arg->FATsectors);
	//װ���һ��Ŀ¼������FirstDirSector��
	arg->FirstDataSector =(arg->FirstDirSector)+(arg->RootDirSectors);
	//װ���һ������������FirstDataSector��
}

unsigned long FAT32_GetNextCluster(unsigned long LastCluster)
{
	unsigned long temp;
	struct FAT32_FAT *pFAT;
	struct FAT32_FAT_Item *pFAT_Item;
	temp=((LastCluster/128)+Init_Arg.FirstFATSector);
	//��������غŶ�Ӧ�Ĵ����������
	 FAT32_ReadSector(temp,FAT32_Buffer);
	 pFAT=(struct FAT32_FAT *)FAT32_Buffer;
	 pFAT_Item=&((pFAT->Items)[LastCluster%128]);
	 //���������������ȡ����
	 return lb2bb(pFAT_Item,4);  //������һ�غ�
 }
 
 
 struct direntry
 {
	 unsigned char deName[8];       // �ļ���
	 unsigned char deExtension[3];  // ��չ��
	 unsigned char deAttributes;    // �ļ�����
	 unsigned char deLowerCase;     // ϵͳ����
	 unsigned char deCHundredth;    // ����ʱ���10 ����λ
	 unsigned char deCTime[2];      // �ļ�����ʱ��
	 unsigned char deCDate[2];      // �ļ���������
	 unsigned char deADate[2];      // �ļ�����������
	 unsigned char deHighClust[2];  // �ļ���ʼ�غŵĸ�16 λ
	 unsigned char deMTime[2];      // �ļ�������޸�ʱ��
	 unsigned char deMDate[2];      // �ļ�������޸�����
	 unsigned char deLowCluster[2];// �ļ���ʼ�غŵĵ�16 λ
	 unsigned char deFileSize[4];   // ��ʾ�ļ��ĳ���
 }


struct FileInfoStruct * FAT32_OpenFile(char *filepath)
{
	unsigned char depth=0;
	unsigned char i,index=1;
	unsigned long iFileSec,iCurFileSec,iFile;
	struct direntry *pFile;
	iCurFileSec=Init_Arg.FirstDirSector;
	for(iFileSec=iCurFileSec;
	iFileSec<iCurFileSec+(Init_Arg.SectorsPerClust);
	iFileSec++)
	{
		FAT32_ReadSector(iFileSec,FAT32_Buffer);
		for(iFile=0;
		iFile<Init_Arg.BytesPerSector;
		iFile+=sizeof(struct direntry)) //�Լ�¼���ɨ��
		{
			pFile=((struct direntry *)(FAT32_Buffer+iFile));
			if(FAT32_CompareName(filepath+index,pFile->deName))
			//���ļ�������ƥ��
			{
				FileInfo.FileSize=lb2bb(pFile->deFileSize,4);
				strcpy(FileInfo.FileName,filepath+index);
				FileInfo.FileStartCluster=lb2bb(pFile->deLowCluster,2)+lb2bb(pFile->deHighClust,2)*65536;
				FileInfo.FileCurCluster=FileInfo.FileStartCluster;
				FileInfo.FileNextCluster=FAT32_GetNextCluster(FileInfo.FileCurCluster);
				FileInfo.FileOffset=0;
				return &FileInfo;
			}
		}
	}
}



struct FileInfoStruct
{
	unsigned char  FileName[12];        //�ļ���
	unsigned long  FileStartCluster;   //�ļ��״غ�
	unsigned long  FileCurCluster;   //�ļ���ǰ�غ�
	unsigned long  FileNextCluster;   //��һ�غ�
	unsigned long  FileSize;       //�ļ���С
	unsigned char  FileAttr;       //�ļ�����
	unsigned short FileCreateTime;   //�ļ�����ʱ��
	unsigned short FileCreateDate;   //�ļ���������
	unsigned short FileMTime;     //�ļ��޸�ʱ��
	unsigned short FileMDate;     //�ļ��޸�����
	unsigned long  FileSector;     //�ļ���ǰ����
	unsigned int   FileOffset;     //�ļ�ƫ����
};

void FAT32_ReadFile(struct FileInfoStruct *pstru,unsigned long len)
{
	unsigned long Sub=pstru->FileSize-pstru->FileOffset;
	unsigned long iSectorInCluster=0;
	unsigned long i=0;
	while(pstru->FileNextCluster!=0x0fffffff)
	//���FAT�еĴ���Ϊ0x0fffffff��˵���޺�̴�
	{
		for(iSectorInCluster=0;
		iSectorInCluster<Init_Arg.SectorsPerClust;
		iSectorInCluster++)
		//������������
		{
			FAT32_ReadSector((((pstru->FileCurCluster)-2)*(Init_Arg.SectorsPerClust))+Init_Arg.FirstDataSector+(iSectorInCluster),FAT32_Buffer);
			pstru->FileOffset+=Init_Arg.BytesPerSector;
			Sub=pstru->FileSize-pstru->FileOffset;
			for(i=0;i<Init_Arg.BytesPerSector;i++)
			{
				send(FAT32_Buffer[i]);   //�����ݷ��͵��ն�����ʾ
			}
		}
		pstru->FileCurCluster=pstru->FileNextCluster;
		pstru->FileNextCluster=FAT32_GetNextCluster(
		pstru->FileCurCluster);
		//������FAT�����Ĵ���
	}
	iSectorInCluster=0;
	while(Sub>=Init_Arg.BytesPerSector)
	//������һ�ت���������������
	{
		FAT32_ReadSector((((pstru->FileCurCluster)-2)
		*(Init_Arg.SectorsPerClust))
		+Init_Arg.FirstDataSector
		+(iSectorInCluster++),FAT32_Buffer);
		pstru->FileOffset+=Init_Arg.BytesPerSector;
		Sub=pstru->FileSize-pstru->FileOffset;
		for(i=0;i<Init_Arg.BytesPerSector;i++)
		{
			send(FAT32_Buffer[i]);
		}
	}
	FAT32_ReadSector((((pstru->FileCurCluster)-2)
	*(Init_Arg.SectorsPerClust))
	+Init_Arg.FirstDataSector
	+(iSectorInCluster),FAT32_Buffer);
	//��ȡ���һ������
	for(i=0;i<Sub;i++)    //SubΪ���ʣ����ֽ���
	{
		send(FAT32_Buffer[i]);
	}
}