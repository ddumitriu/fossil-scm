/*
** Copyright (c) 2006 D. Richard Hipp
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
** This file implements several non-trivial file handling wrapper functions
** on Windows using the Win32 API.
*/
#include "config.h"
#ifdef _WIN32
/* This code is for win32 only */
#include <sys/stat.h>
#include <windows.h>
#include "winfile.h"

#if !defined(S_IFLNK)
# define S_IFLNK 0120000
#endif
#if !defined(SYMBOLIC_LINK_FLAG_DIRECTORY)
# define SYMBOLIC_LINK_FLAG_DIRECTORY (0x1)
#endif

#ifndef LABEL_SECURITY_INFORMATION
# define LABEL_SECURITY_INFORMATION (0x00000010L)
#endif

#ifndef FSCTL_GET_REPARSE_POINT
# define FSCTL_GET_REPARSE_POINT (((0x00000009) << 16) | ((0x00000000) << 14) | ((42) << 2) | (0))
#endif

static HANDLE dllhandle = NULL;
static DWORD (WINAPI *getFinalPathNameByHandleW) (HANDLE, LPWSTR, DWORD, DWORD) = NULL;
static BOOLEAN (APIENTRY *createSymbolicLinkW) (LPCWSTR, LPCWSTR, DWORD) = NULL;

/* a couple defines to make the borrowed struct below compile */
#ifndef _ANONYMOUS_UNION
# define _ANONYMOUS_UNION
#endif
#define DUMMYUNIONNAME

/*
** this structure copied on 20 Sept 2014 from
** https://reactos-mirror.googlecode.com/svn-history/r54752/branches/usb-bringup/include/ddk/ntifs.h
** which is a public domain file from the ReactOS DDK package.
*/

typedef struct {
  ULONG ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  _ANONYMOUS_UNION union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} FOSSIL_REPARSE_DATA_BUFFER;

#define LINK_BUFFER_SIZE 1024

static int isVistaOrLater(){
    if( !dllhandle ){
        HANDLE h = LoadLibraryW(L"KERNEL32");
        createSymbolicLinkW = (BOOLEAN (APIENTRY *) (LPCWSTR, LPCWSTR, DWORD)) GetProcAddress(h, "CreateSymbolicLinkW");
        getFinalPathNameByHandleW = (DWORD (WINAPI *) (HANDLE, LPWSTR, DWORD, DWORD)) GetProcAddress(h, "GetFinalPathNameByHandleW");
        dllhandle = h;
    }
    return createSymbolicLinkW != NULL;
}

/*
** Fill stat buf with information received from GetFileAttributesExW().
** Does not follow symbolic links, returning instead information about
** the link itself.
** Returns 0 on success, 1 on failure.
*/
int win32_lstat(const wchar_t *zFilename, struct fossilStat *buf){
  WIN32_FILE_ATTRIBUTE_DATA attr;
  int rc = GetFileAttributesExW(zFilename, GetFileExInfoStandard, &attr);
  if( rc ){
    ssize_t tlen = 0; /* assume it is not a symbolic link */

    /* if it is a reparse point it *might* be a symbolic link */
    /* so defer to win32_readlink to actually check */
    if( attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ){
      char *tname = fossil_path_to_utf8(zFilename);
      char tlink[LINK_BUFFER_SIZE];
      tlen = win32_readlink(tname, tlink, sizeof(tlink));
      fossil_path_free(tname);
    }

    ULARGE_INTEGER ull;

    /* if a link was retrieved, it is a symlink, otherwise a dir or file */
    if( tlen == 0 ){
      buf->st_mode = ((attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
                       S_IFDIR : S_IFREG);

      buf->st_size = (((i64)attr.nFileSizeHigh)<<32) | attr.nFileSizeLow;
    }else{
      buf->st_mode = S_IFLNK;
      buf->st_size = tlen;
    }

    ull.LowPart = attr.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = attr.ftLastWriteTime.dwHighDateTime;
    buf->st_mtime = ull.QuadPart / 10000000ULL - 11644473600ULL;
  }
  return !rc;
}

/*
** Fill stat buf with information received from win32_lstat().
** If a symbolic link is found, follow it and return information about
** the target, repeating until an actual target is found.
** Limit the number of loop iterations so as to avoid an infinite loop
** due to circular links. This should never happen because
** GetFinalPathNameByHandleW() should always preclude that need, but being
** prepared to loop seems prudent, or at least not harmful.
** Returns 0 on success, 1 on failure.
*/
int win32_stat(const wchar_t *zFilename, struct fossilStat *buf){
  int rc;
  HANDLE file;
  wchar_t nextFilename[LINK_BUFFER_SIZE];
  DWORD len;
  int iterationsRemaining = 8; /* 8 is arbitrary, can be modified as needed */

  while (iterationsRemaining-- > 0){
    rc = win32_lstat(zFilename, buf);

    /* exit on error or not link */
    if( (rc != 0) || (buf->st_mode != S_IFLNK) )
      break;

    /* it is a link, so open the linked file */
    file = CreateFileW(zFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if( (file == NULL) || (file == INVALID_HANDLE_VALUE) ){
      rc = 1;
      break;
    }

    /* get the final path name and close the handle */
    if( isVistaOrLater() ){
      len = getFinalPathNameByHandleW(file, nextFilename, LINK_BUFFER_SIZE - 1, 0);
    }else{
      len = -1;
    }
    CloseHandle(file);

    /* if any problems getting the final path name error so exit */
    if( (len <= 0) || (len > LINK_BUFFER_SIZE - 1) ){
      rc = 1;
      break;
    }

    /* prepare to try again just in case we have a chain to follow */
    /* this shouldn't happen, but just trying to be safe */
    zFilename = nextFilename;
  }

  return rc;
}

/*
** An implementation of a posix-like readlink function for win32.
** Copies the target of a symbolic link to buf if possible.
** Returns the length of the link copied to buf on success, -1 on failure.
*/
ssize_t win32_readlink(const char *path, char *buf, size_t bufsiz){
  /* assume we're going to fail */
  ssize_t rv = -1;

  /* does path reference a reparse point? */
  WIN32_FILE_ATTRIBUTE_DATA attr;
  int rc = GetFileAttributesEx(path, GetFileExInfoStandard, &attr);
  if( rc && (attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ){

    /* since it is a reparse point, open it */
    HANDLE file = CreateFile(path, FILE_READ_EA, 0, NULL, OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if( (file != NULL) && (file != INVALID_HANDLE_VALUE) ){

      /* use DeviceIoControl to get the reparse point data */

      int data_size = sizeof(FOSSIL_REPARSE_DATA_BUFFER) + LINK_BUFFER_SIZE * sizeof(wchar_t);
      FOSSIL_REPARSE_DATA_BUFFER* data = fossil_malloc(data_size);
      DWORD data_used;

      data->ReparseTag = IO_REPARSE_TAG_SYMLINK;
      data->ReparseDataLength = 0;
      data->Reserved = 0;

      int rc = DeviceIoControl(file, FSCTL_GET_REPARSE_POINT, NULL, 0,
        data, data_size, &data_used, NULL);

      /* did the reparse point data fit into the desired buffer? */
      if( rc && (data_used < data_size) ){
        /* it fit, so setup the print name for further processing */
        USHORT
          offset = data->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(wchar_t),
          length = data->SymbolicLinkReparseBuffer.PrintNameLength / sizeof(wchar_t);
        char *temp;
        data->SymbolicLinkReparseBuffer.PathBuffer[offset + length] = 0;

        /* convert the filename to utf8, copy it, and discard the converted copy */
        temp = fossil_path_to_utf8(data->SymbolicLinkReparseBuffer.PathBuffer + offset);
        rv = strlen(temp);
        if( rv >= bufsiz )
          rv = bufsiz;
        memcpy(buf, temp, rv);
        fossil_path_free(temp);
      }

      fossil_free(data);

      /* all done, close the reparse point */
      CloseHandle(file);
    }
  }

  return rv;
}

/*
** Either unlink a file or remove a directory on win32 systems.
** To delete a symlink on a posix system, you simply unlink the entry.
** Unfortunately for our purposes, win32 differentiates between symlinks for
** files and for directories. Thus you must unlink a file symlink or rmdir a
** directory symlink. This is a convenience function used when we know we're
** deleting a symlink of some type.
** Returns 0 on success, 1 on failure.
*/
int win32_unlink_rmdir(const wchar_t *zFilename){
  int rc = 0;
  WIN32_FILE_ATTRIBUTE_DATA attr;
  if( GetFileAttributesExW(zFilename, GetFileExInfoStandard, &attr) ){
    if( (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY )
      rc = RemoveDirectoryW(zFilename);
    else
      rc = DeleteFileW(zFilename);
  }
  return !rc;
}

/*
** An implementation of a posix-like symlink function for win32.
** Attempts to create a file or directory symlink based on the target.
** Defaults to a file symlink if the target does not exist / can't be checked.
** Finally, if the symlink cannot be created for whatever reason (perhaps
** newpath is on a network share or a FAT derived file system), default to
** creation of a text file with the context of the link.
** Returns 0 on success, 1 on failure.
*/
int win32_symlink(const char *oldpath, const char *newpath){
  fossilStat stat;
  int created = 0;
  DWORD flags = 0;
  wchar_t *zMbcs, *zMbcsOld;

  /* does oldpath exist? is it a dir or a file? */
  zMbcsOld = fossil_utf8_to_path(oldpath, 0);
  if( win32_stat(zMbcsOld, &stat) == 0 ){
    if( stat.st_mode == S_IFDIR ){
      flags = SYMBOLIC_LINK_FLAG_DIRECTORY;
    }
  }

  /* remove newpath before creating the symlink */
  zMbcs = fossil_utf8_to_path(newpath, 0);
  win32_unlink_rmdir(zMbcs);
  if( isVistaOrLater() ){
    created = createSymbolicLinkW(zMbcs, zMbcsOld, flags);
  }
  fossil_path_free(zMbcs);
  fossil_path_free(zMbcsOld);

  /* if the symlink was not created, create a plain text file */
  if( !created ){
    Blob content;
    blob_set(&content, oldpath);
    blob_write_to_file(&content, newpath);
    blob_reset(&content);
    created = 1;
  }

  return !created;
}

/*
** Given a pathname to a file, return true if:
**   1. the file exists
**   2. the file is a symbolic link
**   3. the symbolic link's attributes can be acquired
**   4. the symbolic link type is different than the target type
*/
int win32_check_symlink_type_changed(const char* zName){
  int changed = 0;
  wchar_t* zMbcs;
  fossilStat lstat_buf, stat_buf;
  WIN32_FILE_ATTRIBUTE_DATA lstat_attr;
  zMbcs = fossil_utf8_to_path(zName, 0);
  if( win32_stat(zMbcs, &stat_buf) != 0 ){
    stat_buf.st_mode = S_IFREG;
  }
  changed =
    (win32_lstat(zMbcs, &lstat_buf) == 0) &&
    (lstat_buf.st_mode == S_IFLNK) &&
    GetFileAttributesExW(zMbcs, GetFileExInfoStandard, &lstat_attr) &&
    ((stat_buf.st_mode == S_IFDIR) != ((lstat_attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
  fossil_path_free(zMbcs);
  return changed;
}

/*
** Check if symlinks are potentially supported on the current OS for the given file.
** Theoretically this code should work on any NT based version of windows
** but I have no way of testing that. The initial check for
** IsWindowsVistaOrGreater() should in theory eliminate any system prior to
** Windows Vista, but I have no way to test that at this time.
** Return 1 if supported, 0 if not.
*/
int win32_symlinks_supported(const char* zFilename){
  TOKEN_PRIVILEGES tp;
  LUID luid;
  HANDLE process, token;
  DWORD status;
  int success;
  wchar_t *pFilename;
  wchar_t fullName[MAX_PATH+1];
  DWORD fullLength;
  wchar_t volName[MAX_PATH+1];
  DWORD fsFlags;

  /* symlinks only supported on vista or greater */
  if( !isVistaOrLater() ){
    return 0;
  }

  /* next we need to check to see if the privilege is available */

  /* can't check privilege if we can't lookup its value */
  if( !LookupPrivilegeValue(NULL, SE_CREATE_SYMBOLIC_LINK_NAME, &luid) ){
    return 0;
  }

  /* can't check privilege if we can't open the process token */
  process = GetCurrentProcess();
  if( !OpenProcessToken(process, TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &token) ){
    return 0;
  }

  /* by this point, we have a process token and the privilege value */
  /* try to enable the privilege then close the token */

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
  status = GetLastError();

  CloseHandle(token);

  /* any error means we failed to enable the privilege, symlinks not supported */
  if( status != ERROR_SUCCESS ){
    return 0;
  }

  /* assume no support for symlinks */
  success = 0;

  pFilename = fossil_utf8_to_path(zFilename, 0);

  /* given the filename we're interested in, symlinks are supported if */
  /* 1. we can get the full name of the path from the given path */
  fullLength = GetFullPathNameW(pFilename, sizeof(fullName), fullName, NULL);
  if( (fullLength > 0) && (fullLength < sizeof(fullName)) ){
    /* 2. we can get the volume path name from the full name */
    if( GetVolumePathNameW(fullName, volName, sizeof(volName)) ){
      /* 3. we can get volume information from the volume path name */
      if( GetVolumeInformationW(volName, NULL, 0, NULL, NULL, &fsFlags, NULL, 0) ){
        /* 4. the given volume support reparse points */
        if( fsFlags & FILE_SUPPORTS_REPARSE_POINTS ){
          /* all four conditions were true, so we support symlinks; success! */
          success = 1;
        }
      }
    }
  }

  fossil_path_free(pFilename);

  return success;
}

/*
** Wrapper around the access() system call.  This code was copied from Tcl
** 8.6 and then modified.
*/
int win32_access(const wchar_t *zFilename, int flags){
  int rc = 0;
  PSECURITY_DESCRIPTOR pSd = NULL;
  unsigned long size = 0;
  PSID pSid = NULL;
  BOOL sidDefaulted;
  BOOL impersonated = FALSE;
  SID_IDENTIFIER_AUTHORITY unmapped = {{0, 0, 0, 0, 0, 22}};
  GENERIC_MAPPING genMap;
  HANDLE hToken = NULL;
  DWORD desiredAccess = 0, grantedAccess = 0;
  BOOL accessYesNo = FALSE;
  PPRIVILEGE_SET pPrivSet = NULL;
  DWORD privSetSize = 0;
  DWORD attr = GetFileAttributesW(zFilename);

  if( attr==INVALID_FILE_ATTRIBUTES ){
    /*
     * File might not exist.
     */

    if( GetLastError()!=ERROR_SHARING_VIOLATION ){
      rc = -1; goto done;
    }
  }

  if( flags==F_OK ){
    /*
     * File exists, nothing else to check.
     */

    goto done;
  }

  if( (flags & W_OK)
      && (attr & FILE_ATTRIBUTE_READONLY)
      && !(attr & FILE_ATTRIBUTE_DIRECTORY) ){
    /*
     * The attributes say the file is not writable.  If the file is a
     * regular file (i.e., not a directory), then the file is not
     * writable, full stop.  For directories, the read-only bit is
     * (mostly) ignored by Windows, so we can't ascertain anything about
     * directory access from the attrib data.
     */

    rc = -1; goto done;
  }

  /*
   * It looks as if the permissions are ok, but if we are on NT, 2000 or XP,
   * we have a more complex permissions structure so we try to check that.
   * The code below is remarkably complex for such a simple thing as finding
   * what permissions the OS has set for a file.
   */

  /*
   * First find out how big the buffer needs to be.
   */

  GetFileSecurityW(zFilename,
      OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
      DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
      0, 0, &size);

  /*
   * Should have failed with ERROR_INSUFFICIENT_BUFFER
   */

  if( GetLastError()!=ERROR_INSUFFICIENT_BUFFER ){
    /*
     * Most likely case is ERROR_ACCESS_DENIED, which we will convert to
     * EACCES - just what we want!
     */

    rc = -1; goto done;
  }

  /*
   * Now size contains the size of buffer needed.
   */

  pSd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), 0, size);

  if( pSd==NULL ){
    rc = -1; goto done;
  }

  /*
   * Call GetFileSecurity() for real.
   */

  if( !GetFileSecurityW(zFilename,
          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
          DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
          pSd, size, &size) ){
    /*
     * Error getting owner SD
     */

    rc = -1; goto done;
  }

  /*
   * As of Samba 3.0.23 (10-Jul-2006), unmapped users and groups are
   * assigned to SID domains S-1-22-1 and S-1-22-2, where "22" is the
   * top-level authority.  If the file owner and group is unmapped then
   * the ACL access check below will only test against world access,
   * which is likely to be more restrictive than the actual access
   * restrictions.  Since the ACL tests are more likely wrong than
   * right, skip them.  Moreover, the unix owner access permissions are
   * usually mapped to the Windows attributes, so if the user is the
   * file owner then the attrib checks above are correct (as far as they
   * go).
   */

  if( !GetSecurityDescriptorOwner(pSd, &pSid, &sidDefaulted) ||
      memcmp(GetSidIdentifierAuthority(pSid), &unmapped,
             sizeof(SID_IDENTIFIER_AUTHORITY))==0 ){
    goto done; /* Attrib tests say access allowed. */
  }

  /*
   * Perform security impersonation of the user and open the resulting
   * thread token.
   */

  if( !ImpersonateSelf(SecurityImpersonation) ){
    /*
     * Unable to perform security impersonation.
     */

    rc = -1; goto done;
  }
  impersonated = TRUE;

  if( !OpenThreadToken(GetCurrentThread(),
      TOKEN_DUPLICATE | TOKEN_QUERY, FALSE, &hToken) ){
    /*
     * Unable to get current thread's token.
     */

    rc = -1; goto done;
  }

  /*
   * Setup desiredAccess according to the access priveleges we are
   * checking.
   */

  if( flags & R_OK ){
    desiredAccess |= FILE_GENERIC_READ;
  }
  if( flags & W_OK){
    desiredAccess |= FILE_GENERIC_WRITE;
  }

  memset(&genMap, 0, sizeof(GENERIC_MAPPING));
  genMap.GenericRead = FILE_GENERIC_READ;
  genMap.GenericWrite = FILE_GENERIC_WRITE;
  genMap.GenericExecute = FILE_GENERIC_EXECUTE;
  genMap.GenericAll = FILE_ALL_ACCESS;

  AccessCheck(pSd, hToken, desiredAccess, &genMap, 0,
                   &privSetSize, &grantedAccess, &accessYesNo);
  /*
   * Should have failed with ERROR_INSUFFICIENT_BUFFER
   */

  if( GetLastError()!=ERROR_INSUFFICIENT_BUFFER ){
    rc = -1; goto done;
  }
  pPrivSet = (PPRIVILEGE_SET)HeapAlloc(GetProcessHeap(), 0, privSetSize);

  if( pPrivSet==NULL ){
    rc = -1; goto done;
  }

  /*
   * Perform access check using the token.
   */

  if( !AccessCheck(pSd, hToken, desiredAccess, &genMap, pPrivSet,
                   &privSetSize, &grantedAccess, &accessYesNo) ){
    /*
     * Unable to perform access check.
     */

    rc = -1; goto done;
  }
  if( !accessYesNo ){
    rc = -1;
  }

done:

  if( hToken != NULL ){
    CloseHandle(hToken);
  }
  if( impersonated ){
    RevertToSelf();
    impersonated = FALSE;
  }
  if( pPrivSet!=NULL ){
    HeapFree(GetProcessHeap(), 0, pPrivSet);
  }
  if( pSd!=NULL ){
    HeapFree(GetProcessHeap(), 0, pSd);
  }
  return rc;
}

/*
** Wrapper around the chdir() system call.
*/
int win32_chdir(const wchar_t *zChDir, int bChroot){
  int rc = (int)!SetCurrentDirectoryW(zChDir);
  return rc;
}

/*
** Get the current working directory.
**
** On windows, the name is converted from unicode to UTF8 and all '\\'
** characters are converted to '/'.
*/
void win32_getcwd(char *zBuf, int nBuf){
  int i;
  char *zUtf8;
  wchar_t *zWide = fossil_malloc( sizeof(wchar_t)*nBuf );
  if( GetCurrentDirectoryW(nBuf, zWide)==0 ){
    fossil_fatal("cannot find current working directory.");
  }
  zUtf8 = fossil_path_to_utf8(zWide);
  fossil_free(zWide);
  for(i=0; zUtf8[i]; i++) if( zUtf8[i]=='\\' ) zUtf8[i] = '/';
  strncpy(zBuf, zUtf8, nBuf);
  fossil_path_free(zUtf8);
}
#endif /* _WIN32  -- This code is for win32 only */
