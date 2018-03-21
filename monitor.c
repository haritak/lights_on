#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define DEBUG_not
#define FRAME_SIZE 44100
#define THRESHOLD 610000
#define DURATION_SECONDS 45

#include <fcntl.h>  /* File Control Definitions          */
#include <termios.h>/* POSIX Terminal Control Definitions*/
#include <unistd.h> /* UNIX Standard Definitions         */
#include <errno.h>  /* ERROR Number Definitions          */
#include <sys/ioctl.h> //ioctl() call defenitions
#include <unistd.h>

void turn_on(int seconds) {
	int fd;
	int RTS_flag;
	printf("Turning on RTS\n");
	fd = open("/dev/ttyS0",O_RDWR | O_NOCTTY );//Open Serial Port
	RTS_flag = TIOCM_RTS;
	ioctl(fd,TIOCMBIS,&RTS_flag);//Set RTS pin

	sleep(seconds);

	close(fd);
}

int * read_frame(int * frame) {
	int pos = 0;
	while( pos < FRAME_SIZE ) {
		char a = getc(stdin);
		char b = getc(stdin);

		frame[ pos ] = (a << 8) | b;

		pos += 1;
	}

	return frame;
}

float calc_rms(int * samples, int size) {

	float rms = 0;
	for(int i=0; i<size; i++) {
		rms += samples[i] * samples[i];
	}
	
	return sqrt(rms);	
}

int main(void) {

	printf( "Expecting 44kHz signed 16bit little endian (S16).\n" );

	int * frame = malloc( FRAME_SIZE * sizeof(int));
	if (!frame) {
		fprintf(stderr, "No memory\n");
		exit(1);
	}

	int total_time_on = 0;
	time_t started;
       	time(&started);
	while( 1 ) {
		int * samples_frame = read_frame(frame);
		double rms = calc_rms( samples_frame, FRAME_SIZE );

		if (rms>10000) printf("%f\n", rms);

		
		if (rms > THRESHOLD) {
			time_t now;
		       	time(&now);
			struct tm tm = *localtime(&now);

			if (tm.tm_hour > 8 && tm.tm_hour < 18) {
				printf("During daylight, this is skipped\n");
				continue;
			}

			turn_on( DURATION_SECONDS );
			total_time_on += DURATION_SECONDS;

			int total_hours = total_time_on / 60 / 60;
			int total_minutes = total_time_on - total_hours*3600;
			total_minutes = total_minutes / 60;
			printf("Total time on : %02d:%02d\n",
					total_hours, total_minutes);
			time_t diff = difftime(now, started );
			printf("Total time : %ld seconds (of which %d seconds on)\n",
				       	diff, total_time_on);

			float per = (float) total_time_on / diff;
			printf("Percentage on : %f\n", per);

		}
	}

	return 0;
}
