#include "stdio.h"
#include "com.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "Thread.h"

char * ComName = "COM10";  //not useful,just for debug
void Device_Init()
{
	int result;
	result = com_init(ComName);
	if (result == 0)
	{
		printf("Com Open ! \n");
	}
}

uint8* download_file_buf;  //要获取的字符串  
long download_file_len;    //获取的长度  类型设为long,否则会显示不完整 2016-12-5
const char * download_file_path = "c:\\dfu\\no-key\\combined5.dfu";//"c:\\dfu\\my.dfu";// /*路径要用双斜杠\\ */  

void * File_Handle(void)
{
	//创建一个文件指针  
	FILE* download_file = (FILE*)fopen(download_file_path, "rb"); //以二进制格式读取，避免fread读取长度变少

	if (download_file != NULL) {

		fseek(download_file, 0, SEEK_END);   //移到尾部  
		download_file_len = ftell(download_file);          //提取长度  
		rewind(download_file);               //回归原位  
											 //printf("count the file content len = %d \n", download_file_len);
											 //分配buf空间  
		download_file_buf = (uint8*)malloc(sizeof(uint8) * download_file_len + 1);//申请内存空间，存放dfu文件内容
		if (!download_file_buf) {
			printf("malloc space is not enough. \n");
			return NULL;
		}

		//读取文件  
		//读取进的buf，单位大小，长度，文件指针  
		long rLen = fread(download_file_buf, sizeof(uint8), download_file_len, download_file);
		download_file_buf[rLen] = '\0';
		printf("has read Length = %d \n", rLen);
		//printf("has read content = %s \n", download_file_buf);

		for (int i = rLen; i > 0; i--) //去掉dfu文件后缀的长度
		{
			if ((download_file_buf[i + 4] == 0x00) && (download_file_buf[i + 3] == 0x01) &&
				(download_file_buf[i + 2] == 0xff) && (download_file_buf[i + 1] == 0xff))
			{
				download_file_len = i + 1;
				//printf("download_file_len = %d ", download_file_len);
			}
		}
#if 0
		for (int i = 0; i < 100; i++)
		{
			printf("buf[%d]= 0x%x \n", i, (unsigned char)download_file_buf[i]);
		}
#endif

		return 0;
	}
	else
	{
		printf("open file error.");
		exit(0);
	}

	return NULL;
}

#define uart_buf_size 128
uint8 uart_buf[uart_buf_size];
uint8 * uart_ptr = uart_buf;
uint8 * uart_end = uart_buf + uart_buf_size;
uint8 uart_data;

uint8 * uart_handle = uart_buf;
void uart_next(void)
{
	uart_handle++;
	if (uart_handle > uart_end)
		uart_handle = uart_buf;
}

void * Uart_Rcv(void* g)
{
	unsigned char data;
	DWORD lRead;

	if (uart_ptr != uart_end)
	{
		ReadFile(hCOMHnd, &data, 1, &lRead, NULL);
		if (lRead != 1)
		{
			return NULL; //如果读不到数据就直接退出
		}
		if (data == 0xc0)
			uart_data++;
		*uart_ptr++ = data;

		//printf("(0x%x)", *(uart_ptr-1));
	}
	else
	{
		uart_ptr = uart_buf;
	}

	if (uart_data > 1)
	{
		uart_data = 0;
		while (uart_handle != uart_ptr)
		{
			int i = 0, j = 0;
			if (*uart_handle == 0xc0)
			{
				uart_next();
				while (*uart_handle != 0xc0)
				{
					//printf("(0x%x)", *uart_handle);
					i++;
					if ((i == 1) && (*uart_handle == 'o')) j++;
					if ((i == 2) && (*uart_handle == 'k')) j++;

					if (j == 2) //表示接收到OK字符串
					{
						send_flag = 1;
						//printf("rcv ok");
					}

					uart_next();
				}
			}
			uart_next();
		}
	}
	return 0;
}

long bufferLength = 1024;
uint16 blockNum = 0;

#define NUMBER 1200
uint8 * send_buff[NUMBER];

uint8 * send_flag = 0;
void * Thread(void * a)
{
	File_Handle();//读取PC中的DFU文件，加载到数组download_file_buf中

	long times = download_file_len / bufferLength; //计算需要循环的次数
	printf("times = %d \n",times);
	
	if (times == 0)
	{
		//发送一个数据包
		printf("only one packet ! \n");
		exit(0);
	}

	while (1)
	{
		
		while (times--)
		{
			long address_offset = blockNum * bufferLength;
			if (times == 0) bufferLength = download_file_len % bufferLength;
			printf("block = %d,bufferLength= %d \n", blockNum, bufferLength);
			//Dnload(blockNum, download_file_buf + address_offset, bufferLength);
			blockNum++;
			if (times == 0)
			{
				printf("all packet send finish \n");
				exit(0);
			}
		}
		

#if 0
		if (send_flag) //发送标志位
		{
			send_flag = 0;

			//Sleep(300);
			printf("kk  ");
		}
#endif
	}

}

void main_run(void)
{
	while (1)
	{
       Uart_Rcv(NULL);//接收串口数据，并判断
	}
}
