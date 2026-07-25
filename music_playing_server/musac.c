/* Aakash Patel - pate2011 */

#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE (4096)
#define LOG_LEN (4096)
#define LOG_FILE_LEN (64)
#define MICROF (1000000.0)
#define MICRO (1000000)
#define MILLI (1000)

#define mulawwrite(x) snd_pcm_writei(mulawdev, x, mulawfrms)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MICRO_TIME(i) (((i).tv_sec * 1000000L) + (i).tv_usec)
void error(int, const char *);
void error_out(int, const char *);

/* audio codec library functions */

static snd_pcm_t *mulawdev;
static snd_pcm_uframes_t mulawfrms;

/* Initializes audio device */

void mulawopen(size_t *bufsiz) {
  snd_pcm_hw_params_t *p;
  unsigned int rate = 8000;
  snd_pcm_open(&mulawdev, "default", SND_PCM_STREAM_PLAYBACK, 0);
  snd_pcm_hw_params_alloca(&p);
  snd_pcm_hw_params_any(mulawdev, p);
  snd_pcm_hw_params_set_access(mulawdev, p, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(mulawdev, p, SND_PCM_FORMAT_MU_LAW);
  snd_pcm_hw_params_set_channels(mulawdev, p, 1);
  snd_pcm_hw_params_set_rate_near(mulawdev, p, &rate, 0);
  snd_pcm_hw_params(mulawdev, p);
  snd_pcm_hw_params_get_period_size(p, &mulawfrms, 0);
  *bufsiz = (size_t)mulawfrms;
  snd_pcm_drop(mulawdev);
  snd_pcm_prepare(mulawdev);
}

/* Closes audio device */

void mulawclose(void) {
  snd_pcm_drain(mulawdev);
  snd_pcm_close(mulawdev);
}

int main(int argc, char *argv[]) {

  /* Checks for invalid command-line input */

  error_out(argc != 8, "Invalid number of arguments");

  /* Populate command-line input */

  char *file = argv[1];
  char *ip = argv[2];
  unsigned short port = atoi(argv[3]);
  int igamma = atoi(argv[4]);
  int bufsize = atoi(argv[5]);
  int targetbf = atoi(argv[6]);
  char *ctrace = argv[7];

  error_out((bufsize % BUFSIZE) || (targetbf % BUFSIZE) || (targetbf > bufsize),
            "Buffer and target buffer size not multiple of 4096");

  /* Read control parameters */

  FILE *fp = fopen("control-param.dat", "r");
  error(!fp, "fopen control-param.dat");
  float ilambda, epsilon, beta;
  error(fscanf(fp, "%f %f %f", &ilambda, &epsilon, &beta) != 3, "fscanf");
  fclose(fp);
  fp = NULL;

  /* Create and connect TCP socket */

  int TCPSocket = socket(PF_INET, SOCK_STREAM, 0);
  error(TCPSocket < 0, "socket");

  struct sockaddr_in IPAddress;
  memset(&IPAddress, 0, sizeof(IPAddress));
  IPAddress.sin_family = AF_INET;
  IPAddress.sin_port = htons(port);
  inet_pton(AF_INET, ip, &IPAddress.sin_addr);

  error(connect(TCPSocket, (struct sockaddr *)&IPAddress, sizeof(IPAddress)) < 0,
        "connect");

  /* Send audio file name to server */

  write(TCPSocket, file, strlen(file) + 1);

  char c = '\0';
  error(read(TCPSocket, &c, 1) != 1, "read letter");
  error_out(c == 'q', "Invalid File Name");

  if (c == 'K') {

    /* Create UDP connection to server */

    struct sockaddr_in clientIPAddress;
    memset(&clientIPAddress, 0, sizeof(clientIPAddress));
    clientIPAddress.sin_family = AF_INET;
    clientIPAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    unsigned short clientPort = 51234;
    clientIPAddress.sin_port = htons(clientPort);

    int UDPSocket = socket(PF_INET, SOCK_DGRAM, 0);
    error(UDPSocket < 0, "UDPSocket");

    int receiveBuffer = 128 * BUFSIZE;
    setsockopt(UDPSocket, SOL_SOCKET, SO_RCVBUF, &receiveBuffer,
               sizeof(receiveBuffer));

    while (bind(UDPSocket, (struct sockaddr *)&clientIPAddress,
                sizeof(clientIPAddress))) {
      clientPort++;
      clientIPAddress.sin_port = htons(clientPort);
    }

    /* Send UDP port to server */

    clientPort = htons(clientPort);
    write(TCPSocket, &clientPort, 2);

    struct sockaddr_in serverIPAddress;
    socklen_t alen = sizeof(serverIPAddress);

    /* Open audio writer */

    size_t bufsiz;
    mulawopen(&bufsiz);
    error_out(bufsiz != BUFSIZE, "mulawopen buffer size mismatch");

    /* Allocate memory for buffer */

    char *audio_buffer = malloc(bufsize);
    error_out(!audio_buffer, "malloc");
    struct timeval timestamps[LOG_LEN] = {0};
    int Q_log[LOG_LEN] = {0};
    int Q = 0;
    int curr_ptr = 0;
    int playback_ptr = 0;
    int num_entries = 0;
    int num_bytes = 0;
    gettimeofday(timestamps, NULL);

    struct timeval last_playback;
    gettimeofday(&last_playback, NULL);
    int is_streaming = 1;
    int started = 0;

    /* select() loop to play audio */

    while (is_streaming || Q > 0) {
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(UDPSocket, &readfds);
      int maxfd = UDPSocket + 1;
      if (is_streaming) {
        FD_SET(TCPSocket, &readfds);
        maxfd = MAX(UDPSocket, TCPSocket) + 1;
      }

      /* Set up select() timer */

      struct timeval igamma_timer;
      igamma_timer.tv_sec = igamma / MICRO;
      igamma_timer.tv_usec = igamma % MICRO;
      error(select(maxfd, &readfds, NULL, NULL, &igamma_timer) < 0, "select");

      /* Quit the loop when server finishes sending audio file */

      if (FD_ISSET(TCPSocket, &readfds)) {
        c = '\0';
        error(read(TCPSocket, &c, 1) != 1, "read Q");
        if (c == 'Q') {
          is_streaming = 0;
        }
      }

      /* Write audio bytes */

      if (FD_ISSET(UDPSocket, &readfds)) {
        char packet[BUFSIZE];
        num_bytes = recvfrom(UDPSocket, packet, BUFSIZE, 0,
                             (struct sockaddr *)&serverIPAddress, &alen);

        /* Copy received bytes into circular buffer */

        if (num_bytes > 0 && Q + num_bytes <= bufsize) {
          if (num_bytes + curr_ptr <= bufsize) {
            memcpy(audio_buffer + curr_ptr, packet, num_bytes);
          } else {
            memcpy(audio_buffer + curr_ptr, packet, bufsize - curr_ptr);
            memcpy(audio_buffer, packet + bufsize - curr_ptr,
                   num_bytes - bufsize + curr_ptr);
          }
          curr_ptr += num_bytes;
          curr_ptr %= bufsize;
          Q += num_bytes;
        }
      }


      if (!started && Q >= targetbf) {
        started ++;
      }
      if (started){
      struct timeval current_time;
      gettimeofday(&current_time, NULL);
      long microseconds = MICRO_TIME(current_time) - MICRO_TIME(last_playback);

      /* Streams bytes if there are any */

      if ((is_streaming && (Q >= BUFSIZE) && (microseconds >= igamma)) ||
          (!is_streaming && (Q > 0) && (microseconds >= igamma))) {
        if (Q >= BUFSIZE) {

          /* Full block case */
          if (playback_ptr + BUFSIZE <= bufsize) {
            mulawwrite(audio_buffer + playback_ptr);
          } else {
            char temp[BUFSIZE] = "";
            memcpy(temp, audio_buffer + playback_ptr, bufsize - playback_ptr);
            memcpy(temp + bufsize - playback_ptr, audio_buffer,
                   BUFSIZE - bufsize + playback_ptr);
           mulawwrite(temp);
          }
          playback_ptr += BUFSIZE;
          Q -= BUFSIZE;
        } else {

          /* Fractured block case */

          char temp[BUFSIZE] = "";
          if (playback_ptr + Q <= bufsize) {
            memcpy(temp, audio_buffer + playback_ptr, Q);
          } else {
            memcpy(temp, audio_buffer + playback_ptr, bufsize - playback_ptr);
            memcpy(temp + bufsize - playback_ptr, audio_buffer,
                   Q - bufsize + playback_ptr);
          }
          memset(temp + Q, 0xFF, BUFSIZE - Q);
          mulawwrite(temp);
          playback_ptr += Q;
          Q = 0;
        }
        playback_ptr %= bufsize;
        gettimeofday(&last_playback, NULL);

        /* Log buffer occupancy */

        if (num_entries < LOG_LEN - 1) {
          num_entries++;
          timestamps[num_entries] = last_playback;
          Q_log[num_entries] = Q;
        }

        /* Control Law D formula */

        ilambda +=  epsilon * ((Q - targetbf) / (float) BUFSIZE) +
                  beta * ((igamma / MICROF) - ilambda);
        sendto(UDPSocket, &ilambda, sizeof(float), 0,
               (struct sockaddr *)&serverIPAddress, alen);
      }
      }
    }
   mulawclose();
    close(UDPSocket);
    free(audio_buffer);


    /* Write log file */

    char new_logfile[LOG_FILE_LEN] = "";
    snprintf(new_logfile, LOG_FILE_LEN, "%s", ctrace);
    fp = fopen(new_logfile, "w");
    error(!fp, "fopen ctrace");
    long start_time = MICRO_TIME(timestamps[0]);

    for (int i = 0; i < num_entries; i++) {
      fprintf(fp, "Buffer Occupancy at Time %.3f: %d\n",
              (MICRO_TIME(timestamps[i]) - start_time) / 1000.0, Q_log[i]);
    }
    fclose(fp);
    fp = NULL;
  }
  close(TCPSocket);
}
