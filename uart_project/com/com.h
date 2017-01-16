/*
* LamaProg v 0.1
* Communicanion routines
* Jan Parkman   (parkmaj@users.sourceforge.net)
*/
#include <windows.h>
/*
* Inicializace portu
* 9600 bauds, 8bit, no parity, 1 stop bit
*/
int com_init(char *s);

/*
* Write data (string without '\0')
*/
int com_write(unsigned char *data, unsigned long length);

/*
* Write data (1 byte)
*/
int com_put(char data);

/*
* Read data (1 byte)
*/
unsigned char com_get();

/*
* Read message (ended [-+])
* return 0 -- '-', 1 -- '+', -1 --error
*/
int com_read(char *data);

/*
* Close port
*/
int com_exit();

extern HANDLE hCOMHnd;          // Handle serioveho portu
