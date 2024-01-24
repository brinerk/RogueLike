#ifndef RAW_H
#define RAW_H
#endif

#define ABUF_INIT {NULL,0}

struct buffer {
	char *b;
	int len;
};

void disableRawMode(int fd);

void atProgExit(void);

void enableRawMode(void);

void bufferAppend(struct buffer *buf, const char *s, int len);

void bufferFree(struct buffer *buf);

struct winsize getWindowSize(void);
