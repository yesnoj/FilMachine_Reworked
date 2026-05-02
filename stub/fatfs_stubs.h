/**
 * fatfs_stubs.h — FatFS API redirected to standard C file I/O
 * 
 * Maps FatFS file operations to local files in the sd/ directory,
 * simulating the SD card that would be present on the real hardware.
 * 
 * Firmware path:  f_open("/FilMachine.cfg", ...)
 * Simulator maps: sd/FilMachine.cfg
 */

#ifndef FATFS_STUBS_H
#define FATFS_STUBS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ─────────────────────────────────────────────
 * FatFS result codes
 * ───────────────────────────────────────────── */
typedef enum {
    FR_OK = 0,
    FR_DISK_ERR,
    FR_INT_ERR,
    FR_NOT_READY,
    FR_NO_FILE,
    FR_NO_PATH,
    FR_INVALID_NAME,
    FR_DENIED,
    FR_EXIST,
    FR_INVALID_OBJECT,
    FR_WRITE_PROTECTED,
    FR_INVALID_DRIVE,
    FR_NOT_ENABLED,
    FR_NO_FILESYSTEM,
    FR_MKFS_ABORTED,
    FR_TIMEOUT,
    FR_LOCKED,
    FR_NOT_ENOUGH_CORE,
    FR_TOO_MANY_OPEN_FILES,
    FR_INVALID_PARAMETER
} FRESULT;

/* ─────────────────────────────────────────────
 * FatFS open mode flags
 * ───────────────────────────────────────────── */
#define FA_READ          0x01
#define FA_WRITE         0x02
#define FA_CREATE_NEW    0x04
#define FA_CREATE_ALWAYS 0x08
#define FA_OPEN_ALWAYS   0x10
#define FA_OPEN_EXISTING 0x00

/* ─────────────────────────────────────────────
 * FIL — File object (wraps standard FILE*)
 * ───────────────────────────────────────────── */
typedef struct {
    FILE *fp;
    long  file_size;
} FIL;

/* FILINFO for f_stat */
typedef struct {
    long fsize;
} FILINFO;

/* ─────────────────────────────────────────────
 * SD Card simulation directory
 * 
 * Files are stored in ./sd/ relative to the
 * simulator executable's working directory.
 * This mirrors the real SD card mount point.
 * ───────────────────────────────────────────── */
#define SIM_SD_DIR "sd"

/* Ensure sd/ directory exists */
static inline void fatfs_ensure_dir(void) {
    struct stat st = {0};
    if (stat(SIM_SD_DIR, &st) == -1) {
        mkdir(SIM_SD_DIR, 0755);
        printf("[SIM] Created SD card directory: %s/\n", SIM_SD_DIR);
    }
}

/* Build a local path: strip leading / and prepend sd/ */
static inline const char* fatfs_local_path(const char *path, char *buf, int bufsize) {
    fatfs_ensure_dir();
    /* Strip leading slashes */
    const char *clean = path;
    while (*clean == '/') clean++;
    /* If empty after stripping, use the path as-is */
    if (*clean == '\0') clean = path;
    snprintf(buf, bufsize, "%s/%s", SIM_SD_DIR, clean);
    return buf;
}

/* ─────────────────────────────────────────────
 * FatFS function implementations
 * ───────────────────────────────────────────── */
static inline FRESULT f_open(FIL *fp, const char *path, uint8_t mode) {
    char local[256];
    fatfs_local_path(path, local, sizeof(local));
    
    const char *fmode = "rb";
    if (mode & FA_WRITE) {
        if (mode & FA_CREATE_ALWAYS) fmode = "wb";
        else if (mode & FA_CREATE_NEW) fmode = "wb";
        else fmode = "r+b";
    }
    
    fp->fp = fopen(local, fmode);
    if (!fp->fp) {
        printf("[SIM] f_open FAILED: %s (mode: %s)\n", local, fmode);
        return FR_NO_FILE;
    }
    
    /* Get file size */
    fseek(fp->fp, 0, SEEK_END);
    fp->file_size = ftell(fp->fp);
    fseek(fp->fp, 0, SEEK_SET);
    
    printf("[SIM] f_open OK: %s (%ld bytes, mode: %s)\n", local, fp->file_size, fmode);
    return FR_OK;
}

static inline FRESULT f_close(FIL *fp) {
    if (fp && fp->fp) {
        fclose(fp->fp);
        fp->fp = NULL;
    }
    return FR_OK;
}

static inline FRESULT f_read(FIL *fp, void *buf, unsigned int btr, unsigned int *br) {
    if (!fp || !fp->fp) return FR_INVALID_OBJECT;
    *br = (unsigned int)fread(buf, 1, btr, fp->fp);
    return FR_OK;
}

static inline FRESULT f_write(FIL *fp, const void *buf, unsigned int btw, unsigned int *bw) {
    if (!fp || !fp->fp) return FR_INVALID_OBJECT;
    *bw = (unsigned int)fwrite(buf, 1, btw, fp->fp);
    return FR_OK;
}

static inline FRESULT f_stat(const char *path, FILINFO *fno) {
    char local[256];
    fatfs_local_path(path, local, sizeof(local));
    
    struct stat st;
    if (stat(local, &st) != 0) return FR_NO_FILE;
    if (fno) fno->fsize = st.st_size;
    return FR_OK;
}

static inline FRESULT f_unlink(const char *path) {
    char local[256];
    fatfs_local_path(path, local, sizeof(local));
    printf("[SIM] f_unlink: %s\n", local);
    if (unlink(local) != 0) return FR_NO_FILE;
    return FR_OK;
}

static inline FRESULT f_rename(const char *path_old, const char *path_new) {
    char local_old[256], local_new[256];
    fatfs_local_path(path_old, local_old, sizeof(local_old));
    fatfs_local_path(path_new, local_new, sizeof(local_new));
    printf("[SIM] f_rename: %s -> %s\n", local_old, local_new);
    if (rename(local_old, local_new) != 0) return FR_DISK_ERR;
    return FR_OK;
}

static inline long f_size(FIL *fp) {
    return fp ? fp->file_size : 0;
}

#endif /* FATFS_STUBS_H */
