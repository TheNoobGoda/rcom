// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

volatile int STOP = FALSE;

int state_machine(unsigned char *buf, int n, int state){
    switch (state)
    {
    case 0: //start
        if (buf[n] == 'f') return 1;
        break;
    case 1: //flag
        if (buf[n] == 'a') return 2;
        else if (buf[n] != 'f') return 0;
        break;

    case 2: //a
        if(buf[n] == 'c') return 3;
        else if (buf[n] == 'f') return 1;
        else return 0;
        break;

    case 3: //c
        if (buf[n] == 'b') return 4;
        else if ( buf[n] == 'f') return 1;
        return 0;
        break;

    case 4: //bcc
        if(buf[n] == 'f') return 5;
        return 0;
        break;

    case 5: //stop
        return 6;
        break;
    
    default:
        return 0;
        break;
    }

}


int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};

    /*for (int i = 0; i < BUF_SIZE; i++)
    {
        buf[i] = 'a' + i % 26;
    }
*/
    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    //buf[5] = '\n';

    //char c =getchar();
    int n =0;
    /*
    while (c!= '\n')
    {
        buf[n] = c;
        c = getchar();
        n++;
        if (n == BUF_SIZE) break;   
    }
    */
    buf[0] = 'f';
    buf[1] = 'a';
    buf[2] = 'c';
    buf[3] = 'b';
    buf[4] = 'f';
    
    n = 5;
    buf[n] = '\0';
    n++;
    printf("%s \n",buf);

    int bytes = write(fd, buf, n);

    printf("%d bytes written\n", bytes);

    // Wait until all bytes have been written to the serial port
    //sleep(1);
    n = 0;
    char a;
    int STOP = FALSE;
    while (STOP == FALSE)
    {
        int bytes = read(fd, &a, 1);

        buf[n] = a;
        //buf[bytes] = '\0'; // Set end of string to '\0', so we can printf

        //printf(":%s:%d\n", buf, bytes);
        
        if (buf[n] == '\0') STOP = TRUE;
        n++;
    }

    int state =0;
    int number = 0;
    while (number < n){
        state = state_machine(buf,number,state);
        number++;
        if (state == 6) break;
    }

    printf("%s \n",buf);
    if (state == 6) printf("connection sucecefull");
    else printf ("conncetion falied");
    
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
