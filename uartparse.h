#ifndef UARTPARSE_H_
#define UARTPARSE_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
#define MAX_CHARS 100
#define MAX_FIELDS 6
typedef struct _USER_DATA
{
char buffer[MAX_CHARS+1];
uint8_t fieldCount;
uint8_t fieldPosition[MAX_FIELDS];
char fieldType[MAX_FIELDS];
} USER_DATA;


void getsUart0(USER_DATA *data);
char charType(char c);
void parseFields(USER_DATA *data);
char* getFieldString(USER_DATA* data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
bool isCommand(USER_DATA* data, const char strCommand[],uint8_t minArguments);


#endif
