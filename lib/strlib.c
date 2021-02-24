#include "../include/strlib.h"


inline size_t strsz(const char* s)
{
  return s ? strlen(s) + 1 : 0;
}

inline int u8len(const char* s)
{
  return mblen(s,4);
}

inline wint_t nextu8(const char* s)
{
  wchar_t b;
  if (mbtowc(&b,s,4) == -1) return WEOF;
  return b;
}


inline int incu8(wchar_t*  b, const char* s)
{
  return mbtowc(b,s,4);
}


inline wint_t nthu8(const char* s, size_t n)
{
  int inc;
  for (size_t i = 0; i < n; i++) {
    if ((inc = u8len(s)) == -1) return WEOF;
    s += inc;
  }
  return nextu8(s);
}

int u8strlen(const char* s)
{
  int next_cp;
  int count = 0;
  while ((next_cp = u8len(s)) != 0) {
    if (next_cp == -1) return -1;
    count++;
    s++;
  }

  return count;
}

int u8strcmp(const char* sx, const char* sy)
{
  wchar_t wcx, wcy;
  int32_t xul, yul;
  while ((*sx != '\0') && (*sy != '\0'))
    {
      xul = incu8(&wcx,sx);
      yul = incu8(&wcy,sy);
      if (xul == -1)
	return -2;
      if (yul == -1)
	return 2;
      if (xul < yul)
	return -1;
      if (xul > yul)
	return 1;

      sx += xul;
      sy += yul;
    }

  if (*sx == '\0')
    return -1;
  else if (*sy == '\0')
    return 1;
  else
    return 0;
}
