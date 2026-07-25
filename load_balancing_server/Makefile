all:
	gcc upingc.c inter_sleep.c error.c pingrespcb.c -g -o upingc
	gcc upings.c error.c -g -o upings
	gcc tunneld.c  error_out.c error.c -g -o tunneld
	gcc tunnelcli.c  error.c error_out.c -g -o tunnelcli
	gcc tripfs.c  error.c error_out.c dropornot.c invalid_filename.c -g -o tripfs
	gcc tripfr.c  error.c error_out.c -g -o tripfr

clean:
	rm -f upings upingc tunneld tunnelcli tripfs tripfr
