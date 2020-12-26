#ifndef iostream_h

/* begin iostream.h */
#define iostream_h

/*
  Improved basic IO type.

  Main features:

    - support UTF-8
    - provide a more general IO type than FILE
    - provide more information about the stream type (binary/text).
 
*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>



typedef enum {
  IOTYPE_BIN,
  IOTYPE_ASCII,
  IOTYPE_UTF_8,
  IOTYPE_UTF_16,
  IOTYPE_UTF_32,
} iotype_t;

// this flag controls when data actually moves out to the underlying I/O
// channel. memory streams are a special case of this where the data
// never moves out.

typedef enum {
  BM_NONE,
  BM_LINE,
  BM_BLOCK,
  BM_MEM,
} bufmode_t;

typedef enum {
  BST_NONE,
  BST_RD,
  BST_WR,
} bufstate_t;

#define IOSTREAM_INLSIZE 54
#define IOSTREAM_BUFSIZE 131072

typedef struct {
  unsigned char txt   : 1;
  unsigned char bin   : 1;
  unsigned char rd    : 1;
  unsigned char wrt   : 1;
  unsigned char trunc : 1;
  char path[1];
} finfo_t;

typedef struct {
  /* error information */

  int errno;
  
  /* buffer information */

  bufmode_t bm;

  // the state only indicates where the underlying file position is relative
  // to the buffer. reading: at the end. writing: at the beginning.
  // in general, you can do any operation in any state.

  bufstate_t bufstate;
  unsigned char* buf;   // the data buffer
  size_t maxsize;       // space allocated for the buffer  
  size_t size;          // length of valid data in the buffer
  size_t bufpos;        // the current offset within the buffer
  size_t ndirty;        // number of bytes from the buffer that need to be written
  
  /* file information */

  intptr_t fd;            // the file descriptor
  off_t filepos;          // the file position
  size_t lineno;          // the line number
  finfo_t finfo;          // general high level information about the file and stream mode
} iostream_t;

/* forward declarations */
// basic iostream interface
iostream_t* ios_open(char*,int,int,int,int);
int ios_close(iostream_t*);
int ios_eof(iostream_t*);
int ios_errno(iostream_t*);

/* low level interface for bytes, characters, and codepoints */
int sgetc(iostream_t*);
int sputc(char,iostream_t*);
int sungetc(char,iostream_t*);
int speekc(iostream_t*);

int sgetb(iostream_t*);
int sputb(unsigned char,iostream_t*);
int sungetb(unsigned char,iostream_t*);
int speekb(unsigned char,iostream_t*);

/* */


/* end iostream.h */
#endif
