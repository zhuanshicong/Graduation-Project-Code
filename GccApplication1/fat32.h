/*
 * CFile1.c
 *
 * Created: 2016/4/10 20:46:26
 *  Author: zhuanshicong
 */ 
struct FAT32_DBR
{
	unsigned char BS_jmpBoot[3];    //跳转指令            offset: 0
	unsigned char BS_OEMName[8];  //                    offset: 3
	unsigned char BPB_BytesPerSec[2];//每扇区字节数        offset:11
	unsigned char BPB_SecPerClus[1]; //每簇扇区数          offset:13
	unsigned char BPB_RsvdSecCnt[2]; //保留扇区数目        offset:14
	unsigned char BPB_NumFATs[1];  //此卷中FAT表数      offset:16
	unsigned char BPB_RootEntCnt[2]; //FAT32为0           offset:17
	unsigned char BPB_TotSec16[2];   //FAT32为0           offset:19
	unsigned char BPB_Media[1];     //存储介质            offset:21
	unsigned char BPB_FATSz16[2];    //FAT32为0          offset:22
	unsigned char BPB_SecPerTrk[2];  //磁道扇区数          offset:24
	unsigned char BPB_NumHeads[2];   //磁头数             offset:26
	unsigned char BPB_HiddSec[4];    //FAT区前隐扇区数    offset:28
	unsigned char BPB_TotSec32[4];   //该卷总扇区数        offset:32
	unsigned char BPB_FATSz32[4];    //一个FAT表扇区数   offset:36
	unsigned char BPB_ExtFlags[2];   //FAT32特有           offset:40
	unsigned char BPB_FSVer[2];      //FAT32特有          offset:42
	unsigned char BPB_RootClus[4];   //根目录簇号          offset:44
	unsigned char FSInfo[2];         //保留扇区FSINFO扇区数offset:48
	unsigned char BPB_BkBootSec[2];  //通常为6            offset:50
	unsigned char BPB_Reserved[12];  //扩展用              offset:52
	unsigned char BS_DrvNum[1];      //                   offset:64
	unsigned char BS_Reserved1[1];   //                    offset:65
	unsigned char BS_BootSig[1];     //                    offset:66
	unsigned char BS_VolID[4];       //                    offset:67
	unsigned char BS_FilSysType[11]; //                   offset:71
	unsigned char BS_FilSysType1[8]; //"FAT32    "         offset:82
};

unsigned long lb2bb(unsigned char *dat,unsigned char len) //小端转为大端
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
 unsigned char BPB_Sector_No;    //BPB所在扇区号 
 unsigned long Total_Size;         //磁盘的总容量 
 unsigned long FirstDirClust;      //根目录的开始簇 
 unsigned long FirstDataSector;  //文件数据开始扇区号 
 unsigned int  BytesPerSector;  //每个扇区的字节数
 unsigned int  FATsectors;        //FAT表所占扇区数
 unsigned int  SectorsPerClust;  //每簇的扇区数
 unsigned long FirstFATSector;  //第一个FAT表所在扇区
 unsigned long FirstDirSector;  //第一个目录所在扇区
 unsigned long RootDirSectors;  //根目录所占扇区数
 unsigned long RootDirCount;  //根目录下的目录与文件数
 };

void FAT32_Init(struct FAT32_Init_Arg *arg)
{
	struct FAT32_BPB *bpb=(struct FAT32_BPB *)(FAT32_Buffer); //将数据缓冲区指针转为struct FAT32_BPB 型指针
	arg->BPB_Sector_No   =FAT32_FindBPB();        //FAT32_FindBPB()可以返回BPB所在的扇区号
	arg->Total_Size      =FAT32_Get_Total_Size();     //FAT32_Get_Total_Size()可以返回磁盘的总容量单位是兆
	arg->FATsectors      =lb2bb((bpb->BPB_FATSz32)    ,4);   //装入FAT表占用的扇区数到FATsectors中
	arg->FirstDirClust   =lb2bb((bpb->BPB_RootClus)   ,4);      //装入根目录簇号到FirstDirClust中
	arg->BytesPerSector  =lb2bb((bpb->BPB_BytesPerSec),2);    //装入每扇区字节数到BytesPerSector中
	arg->SectorsPerClust =lb2bb((bpb->BPB_SecPerClus) ,1);      //装入每簇扇区数到SectorsPerClust 中
	arg->FirstFATSector=lb2bb((bpb->BPB_RsvdSecCnt) ,2)+arg->BPB_Sector_No;
	//装入第一个FAT表扇区号到FirstFATSector 中
	arg->RootDirCount    =lb2bb((bpb->BPB_RootEntCnt) ,2);  //装入根目录项数到RootDirCount中
	arg->RootDirSectors  =(arg->RootDirCount)*32>>9;   //装入根目录占用的扇区数到RootDirSectors中
	arg->FirstDirSector=(arg->FirstFATSector)+(bpb->BPB_NumFATs[0])*(arg->FATsectors);
	//装入第一个目录扇区到FirstDirSector中
	arg->FirstDataSector =(arg->FirstDirSector)+(arg->RootDirSectors);
	//装入第一个数据扇区到FirstDataSector中
}

unsigned long FAT32_GetNextCluster(unsigned long LastCluster)
{
	unsigned long temp;
	struct FAT32_FAT *pFAT;
	struct FAT32_FAT_Item *pFAT_Item;
	temp=((LastCluster/128)+Init_Arg.FirstFATSector);
	//计算给定簇号对应的簇项的扇区号
	 FAT32_ReadSector(temp,FAT32_Buffer);
	 pFAT=(struct FAT32_FAT *)FAT32_Buffer;
	 pFAT_Item=&((pFAT->Items)[LastCluster%128]);
	 //在算出的扇区中提取簇项
	 return lb2bb(pFAT_Item,4);  //返回下一簇号
 }
 
 
 struct direntry
 {
	 unsigned char deName[8];       // 文件名
	 unsigned char deExtension[3];  // 扩展名
	 unsigned char deAttributes;    // 文件属性
	 unsigned char deLowerCase;     // 系统保留
	 unsigned char deCHundredth;    // 创建时间的10 毫秒位
	 unsigned char deCTime[2];      // 文件创建时间
	 unsigned char deCDate[2];      // 文件创建日期
	 unsigned char deADate[2];      // 文件最后访问日期
	 unsigned char deHighClust[2];  // 文件起始簇号的高16 位
	 unsigned char deMTime[2];      // 文件的最近修改时间
	 unsigned char deMDate[2];      // 文件的最近修改日期
	 unsigned char deLowCluster[2];// 文件起始簇号的低16 位
	 unsigned char deFileSize[4];   // 表示文件的长度
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
		iFile+=sizeof(struct direntry)) //对记录逐个扫描
		{
			pFile=((struct direntry *)(FAT32_Buffer+iFile));
			if(FAT32_CompareName(filepath+index,pFile->deName))
			//对文件名进行匹配
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
	unsigned char  FileName[12];        //文件名
	unsigned long  FileStartCluster;   //文件首簇号
	unsigned long  FileCurCluster;   //文件当前簇号
	unsigned long  FileNextCluster;   //下一簇号
	unsigned long  FileSize;       //文件大小
	unsigned char  FileAttr;       //文件属性
	unsigned short FileCreateTime;   //文件建立时间
	unsigned short FileCreateDate;   //文件建立日期
	unsigned short FileMTime;     //文件修改时间
	unsigned short FileMDate;     //文件修改日期
	unsigned long  FileSector;     //文件当前扇区
	unsigned int   FileOffset;     //文件偏移量
};

void FAT32_ReadFile(struct FileInfoStruct *pstru,unsigned long len)
{
	unsigned long Sub=pstru->FileSize-pstru->FileOffset;
	unsigned long iSectorInCluster=0;
	unsigned long i=0;
	while(pstru->FileNextCluster!=0x0fffffff)
	//如果FAT中的簇项为0x0fffffff说明无后继簇
	{
		for(iSectorInCluster=0;
		iSectorInCluster<Init_Arg.SectorsPerClust;
		iSectorInCluster++)
		//读出整簇数据
		{
			FAT32_ReadSector((((pstru->FileCurCluster)-2)*(Init_Arg.SectorsPerClust))+Init_Arg.FirstDataSector+(iSectorInCluster),FAT32_Buffer);
			pstru->FileOffset+=Init_Arg.BytesPerSector;
			Sub=pstru->FileSize-pstru->FileOffset;
			for(i=0;i<Init_Arg.BytesPerSector;i++)
			{
				send(FAT32_Buffer[i]);   //将数据发送到终端上显示
			}
		}
		pstru->FileCurCluster=pstru->FileNextCluster;
		pstru->FileNextCluster=FAT32_GetNextCluster(
		pstru->FileCurCluster);
		//这里是FAT簇链的传递
	}
	iSectorInCluster=0;
	while(Sub>=Init_Arg.BytesPerSector)
	//处理不足一簇而足扇区的数据
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
	//读取最后一个扇区
	for(i=0;i<Sub;i++)    //Sub为最后剩余的字节数
	{
		send(FAT32_Buffer[i]);
	}
}