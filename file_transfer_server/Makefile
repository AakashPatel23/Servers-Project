all:
	gcc send/tripfs.c send/invalid_filename.c error_out.c error.c send/alarmcb.c send/dropornot.c -g -o tripfs
	gcc receive/tripfr.c error.c error_out.c receive/alarmcb.c -g -o tripfr
clean:
	rm -f tripfs tripfr
