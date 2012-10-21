#include "wx/wx.h"

//Contrary to the c function strcpy, we do not pad to num chars if the source string is smaller,
//      and we return the number of characters copied
int wxStrcpy(wxChar * destination, const wxChar * source, size_t num);
int printDecimal(wxChar* buffer, int bufferSize, long long number);
void printHex(wxChar* buffer, int bufferSize, long long number);