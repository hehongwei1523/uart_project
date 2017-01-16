
#include "stdio.h"
#include "com.h"
#include <windows.h>

using namespace std;

extern "C" void Device_Init(void); //C++ 调用 C函数
extern "C" void * Thread(void *);
extern "C" void main_run(void);

int main()
{
	Device_Init();
	::CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Thread, (LPVOID)0, 0, (LPDWORD)0);  // 建立线程

	main_run();

	return 0;
}