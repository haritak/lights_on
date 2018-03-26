monitor: monitor.c
	gcc -g -Wall -lm monitor.c -o monitor

run: monitor
	arecord -t raw -f S16 -r 44100 -B 0 | ./monitor
