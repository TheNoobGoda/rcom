#include <stdio.h>

int state_machine(unsigned char *buf, int n, int state){
    switch (state)
    {
    case 0: //start
        if (buf[n] == 'f') state = 1;
        n++;
        break;
    case 1: //flag
        if (buf[n] == 'a') state = 2;
        else if (buf[n] != 'f') state = 0;
        n++;
        break;

    case 2: //a
        if(buf[n] == 'c') state = 3;
        else if (buf[n] == 'f') state = 1;
        else state = 0;
        n++;
        break;

    case 3: //c
        if (buf[n] == 'b') state = 4;
        else if ( buf[n] == 'f') state = 1;
        else state = 0;
        n++;
        break;

    case 4: //bcc
        if(buf[n] == 'f') state = 5;
        else state = 0;
        n++;
        break;

    case 5: //stop
        return 1;
        break;
    
    default:
        return 0;
        break;
    }

    return 0;
}