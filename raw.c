#include <raw.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

static struct termios orig_termios;

void disableRawMode(int fd) {
	tcsetattr(fd, TCSAFLUSH, &orig_termios);
}

void atProgExit(void) {
	disableRawMode(STDIN_FILENO);
}

void enableRawMode(void) {
	struct termios raw;

	atexit(atProgExit);

	raw = orig_termios;

	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void bufferAppend(struct buffer *buf, const char *s, int len) {
	char *new = realloc(buf->b,buf->len+len);

	if (new == NULL) return;
	memcpy(new+buf->len,s,len);
	buf->b = new;
	buf->len += len;
}


void bufferFree(struct buffer *buf) {
	free(buf->b);
}


struct winsize getWindowSize(void) {
	struct winsize ws;

	ioctl(1, TIOCGWINSZ, &ws);

	//*cols = ws.ws_col;
	//*rows = ws.ws_row;
	return ws;
}
