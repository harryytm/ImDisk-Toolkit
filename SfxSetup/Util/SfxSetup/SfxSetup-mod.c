#include "../../7z.h"
#include "../../7zAlloc.h"
#include "../../7zCrc.h"
#include "../../7zFile.h"
#include "../../CpuArch.h"
#include <stdio.h>
#include <intrin.h>

#define kBufferSize (1 << 15)
#define kSignatureSearchLimit (1 << 20)

static Bool FindSignature(CSzFile *stream, UInt64 *resPos)
{
  Byte buf[kBufferSize];
  size_t numPrevBytes = 0;
  *resPos = 0;
  for (;;)
  {
    size_t processed, pos;
    if (*resPos > kSignatureSearchLimit)
      return False;
    processed = kBufferSize - numPrevBytes;
    if (File_Read(stream, buf + numPrevBytes, &processed) != 0)
      return False;
    processed += numPrevBytes;
    if (processed < k7zStartHeaderSize || (processed == k7zStartHeaderSize && numPrevBytes != 0))
      return False;
    processed -= k7zStartHeaderSize;
    for (pos = 0; pos <= processed; pos++)
    {
      for (; pos <= processed && buf[pos] != '7'; pos++);
      if (pos > processed)
        break;
      if (memcmp(buf + pos, k7zSignature, k7zSignatureSize) == 0)
        if (CrcCalc(buf + pos + 12, 20) == GetUi32(buf + pos + 8))
        {
          *resPos += pos;
          return True;
        }
    }
    *resPos += processed;
    numPrevBytes = k7zStartHeaderSize;
    memmove(buf, buf + processed, k7zStartHeaderSize);
  }
}

static Bool DoesFileOrDirExist(const WCHAR *path)
{
  WIN32_FIND_DATAW fd;
  HANDLE handle = FindFirstFileW(path, &fd);
  if (handle == INVALID_HANDLE_VALUE)
    return False;
  FindClose(handle);
  return True;
}

static void RemoveDirWithSubItems(WCHAR *path)
{
  WIN32_FIND_DATAW fd;
  HANDLE handle;
  WCHAR *filename_ptr = path + wcslen(path);

  *(ULONG*)filename_ptr = '*';
  if ((handle = FindFirstFileW(path, &fd)) == INVALID_HANDLE_VALUE) return;

  do
  {
	if (fd.cFileName[0] == '.' && (!fd.cFileName[1] || (fd.cFileName[1] == '.' && !fd.cFileName[2]))) continue;
    wcscpy(filename_ptr, fd.cFileName);
    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
      wcscat(path, WSTRING_PATH_SEPARATOR);
      RemoveDirWithSubItems(path);
    }
    else
    {
      SetFileAttributesW(path, 0);
      DeleteFileW(path);
    }
  } while (FindNextFileW(handle, &fd));

  FindClose(handle);
  *filename_ptr = 0;
  RemoveDirectoryW(path);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
  CFileInStream archiveStream;
  CLookToRead lookStream;
  CSzArEx db;
  SRes res = SZ_OK;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;
  WCHAR sfxPath[MAX_PATH + 2];
  WCHAR path[MAX_PATH * 3 + 2];
  WCHAR workCurDir[MAX_PATH + 20];
  size_t pathLen;
  const wchar_t *cmdLineParams;
  char *errorMessage = "Error";
  DWORD exitCode = 0;

  CrcGenerateTable();

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;
  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  FileInStream_CreateVTable(&archiveStream);
  LookToRead_CreateVTable(&lookStream, False);

  sfxPath[0] = 0;
  GetModuleFileNameW(NULL, sfxPath, MAX_PATH);
  cmdLineParams = GetCommandLineW();
  {
    Bool quoteMode = False;
    for (;; cmdLineParams++)
    {
      wchar_t c = *cmdLineParams;
      if (c == L'\"')
        quoteMode = !quoteMode;
      else if (c == 0 || (c == L' ' && !quoteMode))
        break;
    }
  }

  {
    _snwprintf(path + GetTempPathW(MAX_PATH + 1, path), 19, L"7z%I64X", _rdtsc());
    if (!CreateDirectoryW(path, NULL))
	{
	  pathLen = GetCurrentDirectoryW(MAX_PATH, path);
	  _snwprintf(path + pathLen - (path[pathLen - 1] == '\\'), 20, L"\\7z%I64X", _rdtsc());
	  if (!CreateDirectoryW(path, NULL))
      {
	    errorMessage = "Can't create temp folder";
	    goto disp_error;
      }
	}
    wcscpy(workCurDir, path);
    wcscat(path, WSTRING_PATH_SEPARATOR);
    pathLen = wcslen(path);
  }

  if (InFile_OpenW(&archiveStream.file, sfxPath) != 0)
  {
    errorMessage = "Can't open input file";
    res = SZ_ERROR_FAIL;
  }
  else
  {
    UInt64 pos;
    if (!FindSignature(&archiveStream.file, &pos))
      res = SZ_ERROR_FAIL;
    else if (File_Seek(&archiveStream.file, (Int64 *)&pos, SZ_SEEK_SET) != 0)
      res = SZ_ERROR_FAIL;
    if (res != 0)
      errorMessage = "Can't find 7z archive";
  }

  if (res == SZ_OK)
  {
    lookStream.realStream = &archiveStream.s;
    LookToRead_Init(&lookStream);
  }

  SzArEx_Init(&db);
  if (res == SZ_OK)
    res = SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp);

  if (res == SZ_OK)
  {
    UInt32 executeFileIndex = -1;
    UInt32 i;
    UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
    Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
    size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */

    for (i = 0; i < db.NumFiles; i++)
    {
      size_t offset = 0;
      size_t outSizeProcessed = 0;
      WCHAR *temp;

      if (SzArEx_GetFileNameUtf16(&db, i, NULL) >= MAX_PATH)
      {
        res = SZ_ERROR_FAIL;
        break;
      }

      temp = path + pathLen;

      SzArEx_GetFileNameUtf16(&db, i, temp);
      {
        res = SzArEx_Extract(&db, &lookStream.s, i,
          &blockIndex, &outBuffer, &outBufferSize,
          &offset, &outSizeProcessed,
          &allocImp, &allocTempImp);
        if (res != SZ_OK)
          break;
      }
      {
        CSzFile outFile;
        size_t processedSize;
        size_t j;
        size_t nameStartPos = 0;
        for (j = 0; temp[j] != 0; j++)
          if (temp[j] == '/')
          {
            temp[j] = 0;
            CreateDirectoryW(path, NULL);
            temp[j] = CHAR_PATH_SEPARATOR;
            nameStartPos = j + 1;
          }

        if (SzArEx_IsDir(&db, i))
        {
          CreateDirectoryW(path, NULL);
          continue;
        }
        else
        {
          if (!_wcsicmp(temp + nameStartPos, L"config.exe"))
            executeFileIndex = i;

          if (DoesFileOrDirExist(path))
          {
            errorMessage = "Duplicate file";
            res = SZ_ERROR_FAIL;
            break;
          }
          if (OutFile_OpenW(&outFile, path))
          {
            errorMessage = "Can't open output file";
            res = SZ_ERROR_FAIL;
            break;
          }
        }

        processedSize = outSizeProcessed;
        if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0 || processedSize != outSizeProcessed)
        {
          errorMessage = "Can't write output file";
          res = SZ_ERROR_FAIL;
        }

        if (SzBitWithVals_Check(&db.MTime, i))
        {
          const CNtfsFileTime *t = db.MTime.Vals + i;
          FILETIME mTime;
          mTime.dwLowDateTime = t->Low;
          mTime.dwHighDateTime = t->High;
          SetFileTime(outFile.handle, NULL, NULL, &mTime);
        }

        {
          SRes res2 = File_Close(&outFile);
          if (res != SZ_OK)
            break;
          if (res2 != SZ_OK)
          {
            res = res2;
            break;
          }
        }
        if (SzBitWithVals_Check(&db.Attribs, i))
          SetFileAttributesW(path, db.Attribs.Vals[i]);
      }
    }

    if (res == SZ_OK)
    {
      if (executeFileIndex == -1)
      {
        errorMessage = "No file to execute";
        res = SZ_ERROR_FAIL;
      }
      else
      {
        WCHAR *temp = path + pathLen;
        UInt32 j;
        SzArEx_GetFileNameUtf16(&db, executeFileIndex, temp);
        for (j = 0; temp[j] != 0; j++)
          if (temp[j] == '/')
            temp[j] = CHAR_PATH_SEPARATOR;
      }
    }
    IAlloc_Free(&allocImp, outBuffer);
  }
  SzArEx_Free(&db, &allocImp);

  File_Close(&archiveStream.file);

  if (res == SZ_OK)
  {
    SHELLEXECUTEINFO ei = {sizeof(SHELLEXECUTEINFO)};
    ei.lpFile = path;
    ei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
    ei.lpParameters = cmdLineParams;
    ei.lpDirectory = workCurDir;
    ei.nShow = SW_SHOWNORMAL;
    if (ShellExecuteEx(&ei))
    {
	  if (ei.hProcess != NULL)
	  {
	    WaitForSingleObject(ei.hProcess, INFINITE);
	    if (!GetExitCodeProcess(ei.hProcess, &exitCode))
	      exitCode = 1;
        CloseHandle(ei.hProcess);
      }
    }
    else
      res = SZ_ERROR_FAIL;
  }

  path[pathLen] = L'\0';
  RemoveDirWithSubItems(path);

  if (res == SZ_OK)
    ExitProcess(exitCode);

  if (res == SZ_ERROR_UNSUPPORTED)
    errorMessage = "Decoder doesn't support this archive";
  else if (res == SZ_ERROR_MEM)
    errorMessage = "Can't allocate required memory";
  else if (res == SZ_ERROR_CRC)
    errorMessage = "CRC error";

disp_error:
  MessageBoxA(0, errorMessage, "7-Zip Error", MB_ICONERROR);
  ExitProcess(1);
}
