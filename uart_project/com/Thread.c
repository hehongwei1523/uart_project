#include "stdio.h"
#include "com.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "Thread.h"

extern void uart_process(void);

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
						   
						   // /*路径要用双斜杠\\ */  
						   //
const char * download_file_path = "c:\\dfu\\my.dfu";//"c:\\dfu\\no-key\\combined5.dfu";//"c:\\dfu\\no-key\\combine3.dfu"; 

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
		uart_next();
		uart_process();
	}
	return 0;
}

uint8 dfu_begin[] = "dfu_begin";
uint8 dfu_end[] = "dfu_end";
uint8 dfu_data[] = "dfu_data";

void uart_process(void)
{
	uint8 result = 0;
	result = strcmp(uart_handle, dfu_begin);
	if (result == 0)
	{
		send_flag = (uint8)1;
		printf("dfu_begin ");
	}

	result = strcmp(uart_handle, dfu_end);
	if (result == 0)
	{
		send_flag = (uint8)255;
		printf("dfu_end ");
		exit(0);
	}

	result = strcmp(uart_handle, dfu_data);
	if (result == 0)
	{
		send_flag = (uint8)20;
		//printf("dfu_data ");
	}

	uart_handle = uart_ptr;
}

long bufferLength = 1023;//Dnload函数最大只能写入1023个字节，否则会出错
uint16 blockNum = 0;

#define NUMBER 1100
uint8 * send_buff[NUMBER];

uint8 * send_flag = 0;
void * Thread(void * a)
{
	File_Handle();//读取PC中的DFU文件，加载到数组download_file_buf中

	uint8 *data = NULL; 
	uint8 length_h = 0, length_l = 0;
    uint8 times_h = 0, time_l = 0;

	long times = download_file_len / bufferLength; //计算需要循环的次数

	printf("times = %d \n",times);
	printf("len = %d \n", download_file_len);

	while(1)
	{

		if (send_flag == 1) //begin
		{
			com_write("times", 5);
			times_h = (times & 0xFF00) >> 8;
			data = &times_h;
			com_write(data, 1);
			time_l = times & 0xFF;
			data = &time_l;
			com_write(data, 1);
			Sleep(10);
			printf("times = %d \n", times);

			if (times == 0)
			{
				//发送一个数据包
				com_write("data", 4);
				length_h = (download_file_len & 0xFF00) >> 8;
                data = &length_h;
				com_write(data, 1);
				length_l = download_file_len & 0xFF;
				data = &length_l;
				com_write(data, 1);

				com_write(download_file_buf, download_file_len);


				Sleep(100);//等待发送完全结束后才结束进程  2017-1-19
				printf("only one packet ! \n");
				exit(0);
			}
			else
			{
				
				do{
					long address_offset = blockNum * bufferLength;
					if (times == 0) bufferLength = download_file_len % bufferLength;
					printf("block = %d,bufferLength= %d \n", blockNum, bufferLength);

					com_write("data", 4); Sleep(10);

					length_h = (bufferLength & 0xFF00) >> 8;
					data = &length_h;
					com_write(data, 1);
					length_l = bufferLength & 0xFF;
					data = &length_l;
					com_write(data, 1);

					com_write(download_file_buf + address_offset, bufferLength);
					Sleep(200);//等待发送结束

					blockNum++;

					while (send_flag != 20)
						;
					//printf("dfu_data \n");
					send_flag = 0;

					if (times == 0)
					{
						printf("all packet send finish \n");
						exit(0);
					}
				  }
				 while (times--);
				
			}
		}
		
	}

}

void main_run(void)
{
	while (1)
	{
       Uart_Rcv(NULL);//接收串口数据，并判断
	}
}
