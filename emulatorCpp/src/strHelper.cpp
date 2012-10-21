#include "strHelper.h"

//Contrary to the c function strcpy, we do not pad to num chars if the source string is smaller,
//      and we return the number of characters copied
int wxStrcpy(wxChar * destination, const wxChar * source, size_t num) {
    int i;
    for (i = 0; i < num; i++) {
        if (source[i] == 0)
            break;
        destination[i] = source[i];
    }
    return i;
}
int printDecimal(wxChar* buffer, int bufferSize, long long number) {
    if (bufferSize == 0)
        return 0;
    if (number == 0) {
        buffer[0] = wxT('0');
        return 1;
    }
    wxChar tmp[100];
    int tmp_length = 0;
    for (int i = 0; (number != 0) && (i < bufferSize) && (i < 100); i++, tmp_length++) {
        switch (number % 10) {
            case 0:
                tmp[i] = wxT('0');
                break;
            case 1:
                tmp[i] = wxT('1');
                break;
            case 2:
                tmp[i] = wxT('2');
                break;
            case 3:
                tmp[i] = wxT('3');
                break;
            case 4:
                tmp[i] = wxT('4');
                break;
            case 5:
                tmp[i] = wxT('5');
                break;
            case 6:
                tmp[i] = wxT('6');
                break;
            case 7:
                tmp[i] = wxT('7');
                break;
            case 8:
                tmp[i] = wxT('8');
                break;
            case 9:
                tmp[i] = wxT('9');
                break;
            default:
                //....HOW?
                break;
        }
        number /= 10;
    }
    for (int i = 0; i < tmp_length; i++) {
        buffer[i] = tmp[tmp_length-i-1];
    }
    return tmp_length;
}
void printHex(wxChar* buffer, int bufferSize, long long number) {
    for (wxChar* c = buffer+bufferSize-1; c >= buffer; c--) {
        switch (number & 0xF) {
            case 0x0:
                *c = wxT('0');
                break;
            case 0x1:
                *c = wxT('1');
                break;
            case 0x2:
                *c = wxT('2');
                break;
            case 0x3:
                *c = wxT('3');
                break;
            case 0x4:
                *c = wxT('4');
                break;
            case 0x5:
                *c = wxT('5');
                break;
            case 0x6:
                *c = wxT('6');
                break;
            case 0x7:
                *c = wxT('7');
                break;
            case 0x8:
                *c = wxT('8');
                break;
            case 0x9:
                *c = wxT('9');
                break;
            case 0xA:
                *c = wxT('A');
                break;
            case 0xB:
                *c = wxT('B');
                break;
            case 0xC:
                *c = wxT('C');
                break;
            case 0xD:
                *c = wxT('D');
                break;
            case 0xE:
                *c = wxT('E');
                break;
            case 0xF:
                *c = wxT('F');
                break;
            default:
                //....HOW?
                break;
        }
        number >>= 4;
    }
}