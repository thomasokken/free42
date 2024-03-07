extern "C" {
  #include <string.h>
  #include <dmcp.h>
}


#define INI_BUFFERSIZE  128       /* maximum line length, maximum path length */
#define INI_ANSIONLY
#define INI_READONLY
#define INI_NOBROWSE

#if defined FF_USE_STRFUNC && FF_USE_STRFUNC == 2 && !defined INI_LINETERM
  #define INI_LINETERM  "\n"
#endif


#define INI_FILETYPE    FIL
#define ini_openread(filename,file)   (f_open((file), (filename), FA_READ+FA_OPEN_EXISTING) == FR_OK)
#define ini_close(file)               (f_close(file) == FR_OK)

#define INI_FILEPOS                   DWORD
#define ini_tell(file,pos)            (*(pos) = f_tell((file)))
#define ini_seek(file,pos)            (f_lseek((file), *(pos)) == FR_OK)

static int ini_read(char *buffer, size_t size, INI_FILETYPE *fp) {
  uint rd = 0;
  return f_read(fp, buffer, size, &rd) == FR_OK;
}
