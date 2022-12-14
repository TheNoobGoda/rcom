// Read from serial port in non-canonical mode
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

    // Open serial port device for reading and writing and not as controlling tty
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
    newtio.c_cc[VMIN] = 1;  // Blocking read until 5 chars received

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

    // Loop for input
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

    int n = 0;
    unsigned char c[BUF_SIZE] = {0};
    char a;
    while (STOP == FALSE)
    {
        int bytes = read(fd, &a, 1);

        buf[n] = a;
        //buf[bytes] = '\0'; // Set end of string to '\0', so we can printf

        //printf(":%s:%d\n", buf, bytes);
        
        if (buf[n] == '\0') STOP = TRUE;
        n++;
    }
    printf("%s \n",buf);
    int state =0;
    int number = 0;
    while (number < n){
        state = state_machine(buf,number,state);
        number++;
        if (state == 6) break;
    }

    printf("%d \n",state);
    
    sleep(0.5);

    int bytes = write(fd, buf, n);

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
