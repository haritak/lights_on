#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define FRAME_SIZE 44100
#define DURATION_SECONDS 45

#include <fcntl.h>  /* File Control Definitions          */
#include <termios.h>/* POSIX Terminal Control Definitions*/
#include <unistd.h> /* UNIX Standard Definitions         */
#include <errno.h>  /* ERROR Number Definitions          */
#include <sys/ioctl.h> //ioctl() call defenitions
#include <unistd.h>

struct rms_struct {
	short max;
	short min;
	short crossed;
	double rms;
};

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

short * read_frame(short * frame) {

	fread(frame, 2, FRAME_SIZE, stdin);

	/*
	int pos = 0;
	while( pos < FRAME_SIZE ) {

		printf("%d, pos:%d\n", frame[pos], pos);

		pos += 1;
	}
	*/

	return frame;
}

struct rms_struct calc_rms(short * samples, int size) {

	struct rms_struct rval;

	rval.rms = 0;
	rval.max = 0;
	rval.min = 0;
	rval.crossed = 0;
	for(int i=0; i<size; i++) {
		rval.rms += samples[i] * samples[i];

		if (rval.max<samples[i]) rval.max = samples[i];
		if (rval.min>samples[i]) rval.min = samples[i];
		if (i>0 && samples[i-1]*samples[i]<0) rval.crossed++;
	}
	/*
	printf("(max, min, zero-crosses) = (%d, %d, %d)\n", 
			rval.max, rval.min, rval.crossed);
			*/

	rval.rms = sqrt(rval.rms);
	return rval;
}

void print_current_time()
{
	time_t timer;
	char buffer[26];
	struct tm* tm_info;

	time(&timer);
	tm_info = localtime(&timer);

	strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
	puts(buffer);
}

int main(void) {

	printf( "Expecting 44kHz signed 16bit little endian (S16).\n" );

	short * frame = malloc( FRAME_SIZE * sizeof(short));
	if (!frame) {
		fprintf(stderr, "No memory\n");
		exit(1);
	}

	int total_time_on = 0;
	time_t started;
       	time(&started);
	while( 1 ) {
		short * samples_frame = read_frame(frame);
		struct rms_struct rs = calc_rms( samples_frame, FRAME_SIZE );

		if (rs.max > 0) {
			print_current_time();
			printf("(max, min, zero-crosses) = (%d, %d, %d, %f)\n", 
					rs.max, rs.min, rs.crossed, rs.rms);
		}
		/*
		 printf("(max, min, zero-crosses) = (%d, %d, %d, %f)\n", 
				rs.max, rs.min, rs.crossed, rs.rms);
				*/

		if (rs.max > 900 && rs.crossed > 80) {
			printf("(max, min, zero-crosses) = (%d, %d, %d, %f)\n", 
					rs.max, rs.min, rs.crossed, rs.rms);

			time_t now;
		       	time(&now);
			struct tm tm = *localtime(&now);

			int duration = DURATION_SECONDS;
			if (tm.tm_hour > 8 && tm.tm_hour < 18) {
				printf("During daylight, just blink\n");
				duration = 1;
			}

			turn_on( duration );
			total_time_on += duration;
			/*
			 * removed as it causes the light not to turn on for far too
			 * long
			 *
			 * Instead, add -B 0 to arecord so as not to buffer
			 * audio data at all
			while( duration ) {
				short * samples_frame = read_frame(frame);
				duration--;
			}
			*/

			int total_hours = total_time_on / 60 / 60;
			int total_minutes = (total_time_on - total_hours*3600)/60;
			int diff = (int) difftime(now, started );
			float per = (float) total_time_on / diff;
			printf("%02d:%02d (=%dseconds) of total %d seconds (%f%%)\n",
					total_hours, total_minutes,
					total_time_on,
					(int) diff,
					per);

		}
	}

	return 0;
}
