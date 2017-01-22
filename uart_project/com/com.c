

#pragma warning(disable: 4996)

#include "com.h"
#include <fcntl.h>
#include <stdio.h>
#include <windows.h>

#define STRICT 1 
#define VERBOSE 1

HANDLE hCOMHnd;          // Handle serioveho portu

DWORD dwError;

// COM port settings for BCSP
static const int comBCSPDataBits = 8;
static const int comBCSPParity = EVENPARITY;
static const int comBCSPStopBits = ONESTOPBIT;
static const int comBCSPRtsControl = RTS_CONTROL_DISABLE;
static const char comBCSPErrorChar = '\xc0';

// COM port settings
static const int comDataBits = 8;
static const int comStopBits = ONESTOPBIT;
static const int comParity = EVENPARITY;

#define true 1
#define false 0


// Initialize port
int com_init(char *s)
{
	DCB DCBData;

    //hCOMHnd = CreateFile(OpenComName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);//FILE_ATTRIBUTE_NORMAL
	hCOMHnd = CreateFile(L"\\\\.\\COM10", GENERIC_READ | GENERIC_WRITE,0, NULL, OPEN_EXISTING, 0, NULL);

	if (hCOMHnd == INVALID_HANDLE_VALUE)
	{
#if VERBOSE
		dwError = GetLastError(); // 处理错误 
		printf("dwError = %d \n",dwError);
		printf("Init failed...\n");
		exit(-1);
#endif
		return(-1);
	}
	
	//设置读超时
	COMMTIMEOUTS timeouts; //add 2016-11-16
	GetCommTimeouts(hCOMHnd, &timeouts);
	timeouts.ReadIntervalTimeout = 0;// MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 10;// 10;
	timeouts.ReadTotalTimeoutConstant = 1;// 500;// 60000;  //2016-11-23 时间缩小,不能为0
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(hCOMHnd, &timeouts);
	

	// Initialize it: 9600 bauds, 8 bits of data, no parity and 1 stop bit
	ZeroMemory(&DCBData, sizeof(DCBData));
	DCBData.DCBlength = sizeof(DCBData);
	DCBData.fBinary = 1;
	DCBData.fParity = 1;
	DCBData.fOutxCtsFlow = 0;
	DCBData.fOutxDsrFlow = 0;
	DCBData.fDtrControl = DTR_CONTROL_DISABLE;//DTR_CONTROL_ENABLE;
	DCBData.fDsrSensitivity = 0;
	DCBData.fTXContinueOnXoff = 1;
	DCBData.fOutX = 0;
	DCBData.fInX = 0;
	DCBData.fErrorChar = 0;
	DCBData.fNull = 0;
	DCBData.fRtsControl = RTS_CONTROL_DISABLE; //RTS_CONTROL_ENABLE;//
	DCBData.fAbortOnError = 0;

   //以下设置替代函数BuildCommDCB
	DCBData.ByteSize = 8;
	DCBData.StopBits = ONESTOPBIT;
	DCBData.BaudRate =  CBR_115200; // CBR_256000;//460800; //
	DCBData.Parity = NOPARITY; //2016-11-15  奇偶校验位：设为0

	if (!SetCommState(hCOMHnd, &DCBData))
	{
#if VERBOSE
		dwError = GetLastError(); // 处理错误 
		printf("dwError = %d \n", dwError);
		printf("Port parameters setting failed...\n");
		exit(-1);
#endif
		return(-1);
	}

	return 0;
}


int com_write(unsigned char *data, DWORD length)
{
	BOOL bRes;
	DWORD dwWritten;
	COMSTAT ComStat;

	//char length = strlen(data);
	//printf("length=%d ", length);

	//ClearCommError(hCOMHnd, &dwError, &ComStat);
	bRes = WriteFile(hCOMHnd, data, (DWORD)length, &dwWritten, NULL);
	//bRes = WriteFile(hCOMHnd, data, (DWORD)(strlen(data)), &dwWritten, NULL);

	if (!bRes)
	{
#if VERBOSE
		dwError = GetLastError(); // 处理错误 
		printf("dwError = %d \n", dwError);
		printf("Put error...\n");
		exit(-1);
#endif
		return(-1);
	}

	if (dwWritten != length) //strlen(data))
	{
#if VERBOSE
		dwError = GetLastError(); // 处理错误 
		printf("dwError = %d \n", dwError);
		printf("Put error - buffer full...\n");
		exit(-1);
#endif
		return(-1);
	}
	
	return 0;
}


int com_read(char *data)
{
	int i = 0;
	int ch;

	while (((ch = com_get()) >= 0))
	{
		data[i++] = ch;
		if ((ch == '+') || (ch == '-')) break;
	}
	data[i] = '\0';

	if (ch == '+') return 1;
	if (ch == '-') return 0;
	return -1;
}


int com_put(char data)
{
	BOOL bRes;
	DWORD dwWritten;

	bRes = WriteFile(hCOMHnd, &data, 1, &dwWritten, NULL);

	if (!bRes)
	{
#if VERBOSE
		dwError = GetLastError(); // 处理错误 
		printf("dwError = %d \n", dwError);
		printf("Put error...\n");
		exit(-1);
#endif
		return(-1);
	}

	if (dwWritten != 1)
	{
#if VERBOSE
		dwError = GetLastError(); // 处理错误 
		printf("dwError = %d \n", dwError);
		printf("Put error - buffer full...\n");
		exit(-1);
#endif
		return(-1);
	}

	return 0;
}

/*
BOOL ReadFile(
HANDLE hFile,                                    //文件的句柄
LPVOID lpBuffer,                                //用于保存读入数据的一个缓冲区
DWORD nNumberOfBytesToRead,    //要读入的字节数
LPDWORD lpNumberOfBytesRead,    //指向实际读取字节数的指针
LPOVERLAPPED lpOverlapped
//如文件打开时指定了FILE_FLAG_OVERLAPPED，那么必须，用这个参数引用一个特殊的结构。
//该结构定义了一次异步读取操作。否则，应将这个参数设为NULL
);
*/

unsigned char com_get()
{
	BOOL  bRes;
	unsigned char data;// = 0xcc;
	DWORD dwRead;

	BOOL bResult;
	DWORD Event = 0;
	COMSTAT ComStat;        //串口状态 
	DWORD CommEvent = 0xff;

#if 1
	bRes = ReadFile(hCOMHnd, &data, 1, &dwRead, NULL); //每次只读取一个字节，并返回该值，当读不到数据时，返回0xcc
	//根据dwRead判断是否读取数据成功
	if (dwRead == 1) //读到一个数据
	{
		;
	}
#else
	//获取串口事件掩码 
	GetCommMask(hCOMHnd, &CommEvent);
	printf("CommEvent = %d  ", CommEvent);
	if (CommEvent & EV_RXCHAR) //收到一个字符 
	{
		bRes = ReadFile(hCOMHnd, &data, 1, &dwRead, NULL);
		printf("data = 0x%2x ", data);	
	}
#endif

	return (unsigned char)data;

}


int com_exit()
{
	CloseHandle(hCOMHnd);
	return 0;
}
