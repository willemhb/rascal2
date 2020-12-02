#include "error.h"

inline bool _checkerr_null(void* v) {
  return (v == NULL) && ERRORCODE;
}

inline bool _checkerr_val(val_t v) {
  return (v == NONE) && ERRORCODE;
}

inline bool _checkerr_int(int_t i) {
  return (i == -2) && ERRORCODE;
}

inline int_t _seterr_int(int_t i, int_t s) {
  ERRORCODE = i;
  return s;
}

inline val_t _seterr_none(int_t i, val_t s) {
  ERRORCODE = i;
  return s;
}

inline void* _seterr_null(int_t i, void* s) {
  ERRORCODE = i;
  return s;
}

void init_log() {
  static chr_t timebuffer[256];
  const time_t ts = time(NULL);
  struct tm date = *localtime(&ts);
  strftime(timebuffer, 255, "%D [%r]: ", &date);
  
  stdlog = fopen(STDLOG_PATH, "w+");

  if (stdlog == NULL) {
    perror("Failed to initialize rascal log.");
    exit(EXIT_FAILURE);
  }

  fprintf(stdlog,"--- BEGIN LOG FOR RASCAL SESSION AT %s ---", timebuffer);
  return;
}

// read the log into a buffer
chr_t* read_log(chr_t* buffer, int_t length) {
  stdlog = freopen(STDLOG_PATH,"r",stdlog);

  if (stdlog == NULL) {
    escapef(IO_ERR,stderr,"Couldn't reopen stdlog");
  }
  
  int_t i;
  for (i = 0; (i < length) && (!feof(stdlog)); i++) {
    buffer[i] = fgetc(stdlog);
  }

  buffer[i] = '\0';

  // resume writing at the point where stdlog was closed
  stdlog = freopen(STDLOG_PATH,"a",stdlog);

  if (stdlog == NULL) {
    escapef(IO_ERR,stderr,"Couldn't resume log.");
  }
  
  return buffer;
}


void dump_log(FILE* out) {
  stdlog = freopen(STDLOG_PATH,"r",stdlog);

  if (stdlog == NULL) {
    escapef(IO_ERR,stderr,"Couldn't reopen stdlog");
  }

  while (!feof(stdlog)) {
    chr_t c = fgetc(stdlog);
    fputc(c, out);
  }

  // resume writing at the point where stdlog was closed
  stdlog = freopen(STDLOG_PATH,"a",stdlog);

  if (stdlog == NULL) {
    escapef(IO_ERR,stderr,"Couldn't resume log.");
  }
  
  return;
}


int_t finalize_log() {
  static chr_t timebuffer[256];
  const time_t ts = time(NULL);
  struct tm date = *localtime(&ts);
  strftime(timebuffer, 255, "%D [%r]: ", &date);
  fprintf(stdlog,"--- END LOG FOR RASCAL SESSION AT %s ---", timebuffer);
  
  FILE* history = fopen(STDHIST_PATH,"a+");

  if (history == NULL) {
    eprintf(IO_ERR,stderr,"Couldn't open history.");
    return EOF;
  }

  stdlog = freopen(STDLOG_PATH,"r",stdlog);

  if (stdlog == NULL) {
    eprintf(IO_ERR,stderr,"Couldn't reopen log.");
    fclose(history);
    return EOF;
  }

  while (!feof(stdlog)) {
    chr_t c = fgetc(stdlog);
    fputc(c, history);
  }

  if (fclose(stdlog) == EOF) {
    return EOF;
  } else if (fclose(history) == EOF){
    return EOF;
  } else {
    return 0;
  }
}
