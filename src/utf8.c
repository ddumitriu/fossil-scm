/*
** Copyright (c) 2012 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains utilities for converting text between UTF-8 (which
** is always used internally) and whatever encodings are used by the underlying
** filesystem and operating system.
*/
#include "config.h"
#include "utf8.h"
#include <sqlite3.h>
#ifdef _WIN32
# include <windows.h>
#endif

/*
** Translate MBCS to UTF8.  Return a pointer to the translated text.
** Call fossil_mbcs_free() to deallocate any memory used to store the
** returned pointer when done.
*/
char *fossil_mbcs_to_utf8(const char *zMbcs){
#ifdef _WIN32
  extern char *sqlite3_win32_mbcs_to_utf8(const char*);
  return sqlite3_win32_mbcs_to_utf8(zMbcs);
#else
  return (char*)zMbcs;  /* No-op on unix */
#endif
}

/*
** After translating from UTF8 to MBCS, invoke this routine to deallocate
** any memory used to hold the translation
*/
void fossil_mbcs_free(char *zOld){
#ifdef _WIN32
  sqlite3_free(zOld);
#else
  /* No-op on unix */
#endif
}

/*
** Translate Unicode text into UTF8.
** Return a pointer to the translated text.
** Call fossil_unicode_free() to deallocate any memory used to store the
** returned pointer when done.
*/
char *fossil_unicode_to_utf8(const void *zUnicode){
#ifdef _WIN32
  int nByte = WideCharToMultiByte(CP_UTF8, 0, zUnicode, -1, 0, 0, 0, 0);
  char *zUtf = sqlite3_malloc( nByte );
  if( zUtf==0 ){
    return 0;
  }
  WideCharToMultiByte(CP_UTF8, 0, zUnicode, -1, zUtf, nByte, 0, 0);
  return zUtf;
#else
  return (char *)zUnicode;  /* No-op on unix */
#endif
}

/*
** Translate UTF8 to unicode for use in system calls.  Return a pointer to the
** translated text..  Call fossil_unicode_free() to deallocate any memory
** used to store the returned pointer when done.
*/
void *fossil_utf8_to_unicode(const char *zUtf8){
#ifdef _WIN32
  int nByte = MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, 0, 0);
  wchar_t *zUnicode = sqlite3_malloc( nByte * 2 );
  if( zUnicode==0 ){
    return 0;
  }
  MultiByteToWideChar(CP_UTF8, 0, zUtf8, -1, zUnicode, nByte);
  return zUnicode;
#else
  return (void *)zUtf8;  /* No-op on unix */
#endif
}

/*
** Deallocate any memory that was previously allocated by
** fossil_unicode_to_utf8().
*/
void fossil_unicode_free(void *pOld){
#ifdef _WIN32
  sqlite3_free(pOld);
#else
  /* No-op on unix */
#endif
}

#if defined(__APPLE__) && !defined(WITHOUT_ICONV)
# include <iconv.h>
#endif

/*
** Translate text from the filename character set into
** to precomposed UTF8.  Return a pointer to the translated text.
** Call fossil_filename_free() to deallocate any memory used to store the
** returned pointer when done.
*/
char *fossil_filename_to_utf8(void *zFilename){
#if defined(_WIN32)
  int nByte;
  char *zUtf;
  WCHAR *wUnicode = zFilename;
  while( *wUnicode != 0 ){
    if ( (*wUnicode & 0xFF80) == 0xF000 ){
      WCHAR converted = (*wUnicode & 0x7F);
      /* Only really convert it when the resulting char is in the given range*/
      if ( (converted < 32) || wcschr(L"\"*<>?|:", converted) ){
        *wUnicode = converted;
      }
    }
    ++wUnicode;
  }
  nByte = WideCharToMultiByte(CP_UTF8, 0, zFilename, -1, 0, 0, 0, 0);
  zUtf = sqlite3_malloc( nByte );
  if( zUtf==0 ){
    return 0;
  }
  WideCharToMultiByte(CP_UTF8, 0, zFilename, -1, zUtf, nByte, 0, 0);
  return zUtf;
#elif defined(__CYGWIN__)
  char *zOut;
  zOut = fossil_strdup(zFilename);
  return zOut;
#elif defined(__APPLE__) && !defined(WITHOUT_ICONV)
  char *zIn = (char*)zFilename;
  char *zOut;
  iconv_t cd;
  size_t n, x;
  for(n=0; zIn[n]>0 && zIn[n]<=0x7f; n++){}
  if( zIn[n]!=0 && (cd = iconv_open("UTF-8", "UTF-8-MAC"))!=(iconv_t)-1 ){
    char *zOutx;
    char *zOrig = zIn;
    size_t nIn, nOutx;
    nIn = n = strlen(zIn);
    nOutx = nIn+100;
    zOutx = zOut = fossil_malloc( nOutx+1 );
    x = iconv(cd, &zIn, &nIn, &zOutx, &nOutx);
    if( x==(size_t)-1 ){
      fossil_free(zOut);
      zOut = fossil_strdup(zOrig);
    }else{
      zOut[n+100-nOutx] = 0;
    }
    iconv_close(cd);
  }else{
    zOut = fossil_strdup(zFilename);
  }
  return zOut;
#else
  return (char *)zFilename;  /* No-op on non-mac unix */
#endif
}

/*
** Translate UTF8 to unicode for use in filename translations.
** Return a pointer to the translated text..  Call fossil_filename_free()
** to deallocate any memory used to store the returned pointer when done.
**
** On Windows, characters in the range U+0001 to U+0031 and the
** characters '"', '*', ':', '<', '>', '?' and '|' are invalid
** to be used. Therefore, translated those to characters in the
** (private use area), in the range U+F001 - U+F07F, so those
** characters never arrive in any Windows API. The filenames might
** look strange in Windows explorer, but in the cygwin shell
** everything looks as expected.
**
** See: <http://cygwin.com/cygwin-ug-net/using-specialnames.html>
**
*/
void *fossil_utf8_to_filename(const char *zUtf8){
#ifdef _WIN32
  WCHAR *zUnicode = fossil_utf8_to_unicode(zUtf8);
  WCHAR *wUnicode = zUnicode;
  /* If path starts with "<drive>:/" or "<drive>:\", don't translate the ':' */
  if( fossil_isalpha(zUtf8[0]) && zUtf8[1]==':'
           && (zUtf8[2]=='\\' || zUtf8[2]=='/')) {
    zUnicode[2] = '\\';
    wUnicode += 3;
  }
  while( *wUnicode != '\0' ){
    if ( (*wUnicode < 32) || wcschr(L"\"*<>?|:", *wUnicode) ){
      *wUnicode |= 0xF000;
    }else if( *wUnicode == '/' ){
      *wUnicode = '\\';
    }
    ++wUnicode;
  }

  return zUnicode;
#elif defined(__CYGWIN__)
  char *zPath = fossil_strdup(zUtf8);
  char *p = zPath;
  while( (*p = *zUtf8++) != 0){
    if (*p++ == '\\' ) {
      p[-1] = '/';
    }
  }
  return zPath;
#elif defined(__APPLE__) && !defined(WITHOUT_ICONV)
  return fossil_strdup(zUtf8);
#else
  return (void *)zUtf8;  /* No-op on unix */
#endif
}

/*
** Deallocate any memory that was previously allocated by
** fossil_filename_to_utf8() or fossil_utf8_to_filename().
*/
void fossil_filename_free(void *pOld){
#if defined(_WIN32)
  sqlite3_free(pOld);
#elif (defined(__APPLE__) && !defined(WITHOUT_ICONV)) || defined(__CYGWIN__)
  fossil_free(pOld);
#else
  /* No-op on all other unix */
#endif
}

/*
** Display UTF8 on the console.  Return the number of
** Characters written. If stdout or stderr is redirected
** to a file, -1 is returned and nothing is written
** to the console.
*/
int fossil_utf8_to_console(const char *zUtf8, int nByte, int toStdErr){
#ifdef _WIN32
  int nChar;
  wchar_t *zUnicode; /* Unicode version of zUtf8 */
  DWORD dummy;

  static int istty[2] = { -1, -1 };
  if( istty[toStdErr] == -1 ){
    istty[toStdErr] = _isatty(toStdErr + 1) != 0;
  }
  if( !istty[toStdErr] ){
    /* stdout/stderr is not a console. */
    return -1;
  }

  nChar = MultiByteToWideChar(CP_UTF8, 0, zUtf8, nByte, NULL, 0);
  zUnicode = malloc( (nChar + 1) *sizeof(zUnicode[0]) );
  if( zUnicode==0 ){
    return 0;
  }
  nChar = MultiByteToWideChar(CP_UTF8, 0, zUtf8, nByte, zUnicode, nChar);
  if( nChar==0 ){
    free(zUnicode);
    return 0;
  }
  zUnicode[nChar] = '\0';
  WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE - toStdErr), zUnicode, nChar,
                &dummy, 0);
  return nChar;
#else
  return -1;  /* No-op on unix */
#endif
}
