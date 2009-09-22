# 1 "lexer.c"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "lexer.c"
# 18 "lexer.c"
# 1 "/usr/include/stdio.h" 1 3 4
# 28 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 319 "/usr/include/features.h" 3 4
# 1 "/usr/include/sys/cdefs.h" 1 3 4
# 320 "/usr/include/features.h" 2 3 4
# 342 "/usr/include/features.h" 3 4
# 1 "/usr/include/gnu/stubs.h" 1 3 4
# 343 "/usr/include/features.h" 2 3 4
# 29 "/usr/include/stdio.h" 2 3 4





# 1 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 1 3 4
# 213 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 35 "/usr/include/stdio.h" 2 3 4

# 1 "/usr/include/bits/types.h" 1 3 4
# 28 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 29 "/usr/include/bits/types.h" 2 3 4


# 1 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 1 3 4
# 32 "/usr/include/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;







typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
# 129 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/typesizes.h" 1 3 4
# 130 "/usr/include/bits/types.h" 2 3 4






__extension__ typedef unsigned long int __dev_t;
__extension__ typedef unsigned int __uid_t;
__extension__ typedef unsigned int __gid_t;
__extension__ typedef unsigned long int __ino_t;
__extension__ typedef unsigned long int __ino64_t;
__extension__ typedef unsigned int __mode_t;
__extension__ typedef unsigned long int __nlink_t;
__extension__ typedef long int __off_t;
__extension__ typedef long int __off64_t;
__extension__ typedef int __pid_t;
__extension__ typedef struct { int __val[2]; } __fsid_t;
__extension__ typedef long int __clock_t;
__extension__ typedef unsigned long int __rlim_t;
__extension__ typedef unsigned long int __rlim64_t;
__extension__ typedef unsigned int __id_t;
__extension__ typedef long int __time_t;
__extension__ typedef unsigned int __useconds_t;
__extension__ typedef long int __suseconds_t;

__extension__ typedef int __daddr_t;
__extension__ typedef long int __swblk_t;
__extension__ typedef int __key_t;


__extension__ typedef int __clockid_t;


__extension__ typedef int __timer_t;


__extension__ typedef long int __blksize_t;




__extension__ typedef long int __blkcnt_t;
__extension__ typedef long int __blkcnt64_t;


__extension__ typedef unsigned long int __fsblkcnt_t;
__extension__ typedef unsigned long int __fsblkcnt64_t;


__extension__ typedef unsigned long int __fsfilcnt_t;
__extension__ typedef unsigned long int __fsfilcnt64_t;

__extension__ typedef long int __ssize_t;



typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;


__extension__ typedef long int __intptr_t;


__extension__ typedef unsigned int __socklen_t;
# 37 "/usr/include/stdio.h" 2 3 4









typedef struct _IO_FILE FILE;





# 62 "/usr/include/stdio.h" 3 4
typedef struct _IO_FILE __FILE;
# 72 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/libio.h" 1 3 4
# 32 "/usr/include/libio.h" 3 4
# 1 "/usr/include/_G_config.h" 1 3 4
# 14 "/usr/include/_G_config.h" 3 4
# 1 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 1 3 4
# 325 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 3 4
typedef int wchar_t;
# 354 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 3 4
typedef unsigned int wint_t;
# 15 "/usr/include/_G_config.h" 2 3 4
# 24 "/usr/include/_G_config.h" 3 4
# 1 "/usr/include/wchar.h" 1 3 4
# 48 "/usr/include/wchar.h" 3 4
# 1 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 1 3 4
# 49 "/usr/include/wchar.h" 2 3 4

# 1 "/usr/include/bits/wchar.h" 1 3 4
# 51 "/usr/include/wchar.h" 2 3 4
# 76 "/usr/include/wchar.h" 3 4
typedef struct
{
  int __count;
  union
  {
    wint_t __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 25 "/usr/include/_G_config.h" 2 3 4

typedef struct
{
  __off_t __pos;
  __mbstate_t __state;
} _G_fpos_t;
typedef struct
{
  __off64_t __pos;
  __mbstate_t __state;
} _G_fpos64_t;
# 44 "/usr/include/_G_config.h" 3 4
# 1 "/usr/include/gconv.h" 1 3 4
# 28 "/usr/include/gconv.h" 3 4
# 1 "/usr/include/wchar.h" 1 3 4
# 48 "/usr/include/wchar.h" 3 4
# 1 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 1 3 4
# 49 "/usr/include/wchar.h" 2 3 4
# 29 "/usr/include/gconv.h" 2 3 4


# 1 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 1 3 4
# 32 "/usr/include/gconv.h" 2 3 4





enum
{
  __GCONV_OK = 0,
  __GCONV_NOCONV,
  __GCONV_NODB,
  __GCONV_NOMEM,

  __GCONV_EMPTY_INPUT,
  __GCONV_FULL_OUTPUT,
  __GCONV_ILLEGAL_INPUT,
  __GCONV_INCOMPLETE_INPUT,

  __GCONV_ILLEGAL_DESCRIPTOR,
  __GCONV_INTERNAL_ERROR
};



enum
{
  __GCONV_IS_LAST = 0x0001,
  __GCONV_IGNORE_ERRORS = 0x0002
};



struct __gconv_step;
struct __gconv_step_data;
struct __gconv_loaded_object;
struct __gconv_trans_data;



typedef int (*__gconv_fct) (struct __gconv_step *, struct __gconv_step_data *,
       __const unsigned char **, __const unsigned char *,
       unsigned char **, size_t *, int, int);


typedef wint_t (*__gconv_btowc_fct) (struct __gconv_step *, unsigned char);


typedef int (*__gconv_init_fct) (struct __gconv_step *);
typedef void (*__gconv_end_fct) (struct __gconv_step *);



typedef int (*__gconv_trans_fct) (struct __gconv_step *,
      struct __gconv_step_data *, void *,
      __const unsigned char *,
      __const unsigned char **,
      __const unsigned char *, unsigned char **,
      size_t *);


typedef int (*__gconv_trans_context_fct) (void *, __const unsigned char *,
       __const unsigned char *,
       unsigned char *, unsigned char *);


typedef int (*__gconv_trans_query_fct) (__const char *, __const char ***,
     size_t *);


typedef int (*__gconv_trans_init_fct) (void **, const char *);
typedef void (*__gconv_trans_end_fct) (void *);

struct __gconv_trans_data
{

  __gconv_trans_fct __trans_fct;
  __gconv_trans_context_fct __trans_context_fct;
  __gconv_trans_end_fct __trans_end_fct;
  void *__data;
  struct __gconv_trans_data *__next;
};



struct __gconv_step
{
  struct __gconv_loaded_object *__shlib_handle;
  __const char *__modname;

  int __counter;

  char *__from_name;
  char *__to_name;

  __gconv_fct __fct;
  __gconv_btowc_fct __btowc_fct;
  __gconv_init_fct __init_fct;
  __gconv_end_fct __end_fct;



  int __min_needed_from;
  int __max_needed_from;
  int __min_needed_to;
  int __max_needed_to;


  int __stateful;

  void *__data;
};



struct __gconv_step_data
{
  unsigned char *__outbuf;
  unsigned char *__outbufend;



  int __flags;



  int __invocation_counter;



  int __internal_use;

  __mbstate_t *__statep;
  __mbstate_t __state;



  struct __gconv_trans_data *__trans;
};



typedef struct __gconv_info
{
  size_t __nsteps;
  struct __gconv_step *__steps;
  __extension__ struct __gconv_step_data __data [];
} *__gconv_t;
# 45 "/usr/include/_G_config.h" 2 3 4
typedef union
{
  struct __gconv_info __cd;
  struct
  {
    struct __gconv_info __cd;
    struct __gconv_step_data __data;
  } __combined;
} _G_iconv_t;

typedef int _G_int16_t __attribute__ ((__mode__ (__HI__)));
typedef int _G_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int _G_uint16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int _G_uint32_t __attribute__ ((__mode__ (__SI__)));
# 33 "/usr/include/libio.h" 2 3 4
# 53 "/usr/include/libio.h" 3 4
# 1 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stdarg.h" 1 3 4
# 43 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 54 "/usr/include/libio.h" 2 3 4
# 166 "/usr/include/libio.h" 3 4
struct _IO_jump_t; struct _IO_FILE;
# 176 "/usr/include/libio.h" 3 4
typedef void _IO_lock_t;





struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;



  int _pos;
# 199 "/usr/include/libio.h" 3 4
};


enum __codecvt_result
{
  __codecvt_ok,
  __codecvt_partial,
  __codecvt_error,
  __codecvt_noconv
};
# 267 "/usr/include/libio.h" 3 4
struct _IO_FILE {
  int _flags;




  char* _IO_read_ptr;
  char* _IO_read_end;
  char* _IO_read_base;
  char* _IO_write_base;
  char* _IO_write_ptr;
  char* _IO_write_end;
  char* _IO_buf_base;
  char* _IO_buf_end;

  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;



  int _flags2;

  __off_t _old_offset;



  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];



  _IO_lock_t *_lock;
# 315 "/usr/include/libio.h" 3 4
  __off64_t _offset;
# 324 "/usr/include/libio.h" 3 4
  void *__pad1;
  void *__pad2;
  void *__pad3;
  void *__pad4;
  size_t __pad5;

  int _mode;

  char _unused2[15 * sizeof (int) - 4 * sizeof (void *) - sizeof (size_t)];

};


typedef struct _IO_FILE _IO_FILE;


struct _IO_FILE_plus;

extern struct _IO_FILE_plus _IO_2_1_stdin_;
extern struct _IO_FILE_plus _IO_2_1_stdout_;
extern struct _IO_FILE_plus _IO_2_1_stderr_;
# 360 "/usr/include/libio.h" 3 4
typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);







typedef __ssize_t __io_write_fn (void *__cookie, __const char *__buf,
     size_t __n);







typedef int __io_seek_fn (void *__cookie, __off64_t *__pos, int __w);


typedef int __io_close_fn (void *__cookie);
# 412 "/usr/include/libio.h" 3 4
extern int __underflow (_IO_FILE *) __attribute__ ((__nothrow__));
extern int __uflow (_IO_FILE *) __attribute__ ((__nothrow__));
extern int __overflow (_IO_FILE *, int) __attribute__ ((__nothrow__));
extern wint_t __wunderflow (_IO_FILE *) __attribute__ ((__nothrow__));
extern wint_t __wuflow (_IO_FILE *) __attribute__ ((__nothrow__));
extern wint_t __woverflow (_IO_FILE *, wint_t) __attribute__ ((__nothrow__));
# 450 "/usr/include/libio.h" 3 4
extern int _IO_getc (_IO_FILE *__fp) __attribute__ ((__nothrow__));
extern int _IO_putc (int __c, _IO_FILE *__fp) __attribute__ ((__nothrow__));
extern int _IO_feof (_IO_FILE *__fp) __attribute__ ((__nothrow__));
extern int _IO_ferror (_IO_FILE *__fp) __attribute__ ((__nothrow__));

extern int _IO_peekc_locked (_IO_FILE *__fp) __attribute__ ((__nothrow__));





extern void _IO_flockfile (_IO_FILE *) __attribute__ ((__nothrow__));
extern void _IO_funlockfile (_IO_FILE *) __attribute__ ((__nothrow__));
extern int _IO_ftrylockfile (_IO_FILE *) __attribute__ ((__nothrow__));
# 480 "/usr/include/libio.h" 3 4
extern int _IO_vfscanf (_IO_FILE * __restrict, const char * __restrict,
   __gnuc_va_list, int *__restrict);
extern int _IO_vfprintf (_IO_FILE *__restrict, const char *__restrict,
    __gnuc_va_list);
extern __ssize_t _IO_padn (_IO_FILE *, int, __ssize_t) __attribute__ ((__nothrow__));
extern size_t _IO_sgetn (_IO_FILE *, void *, size_t) __attribute__ ((__nothrow__));

extern __off64_t _IO_seekoff (_IO_FILE *, __off64_t, int, int) __attribute__ ((__nothrow__));
extern __off64_t _IO_seekpos (_IO_FILE *, __off64_t, int) __attribute__ ((__nothrow__));

extern void _IO_free_backup_area (_IO_FILE *) __attribute__ ((__nothrow__));
# 73 "/usr/include/stdio.h" 2 3 4
# 86 "/usr/include/stdio.h" 3 4


typedef _G_fpos_t fpos_t;




# 138 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/stdio_lim.h" 1 3 4
# 139 "/usr/include/stdio.h" 2 3 4



extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;









extern int remove (__const char *__filename) __attribute__ ((__nothrow__));

extern int rename (__const char *__old, __const char *__new) __attribute__ ((__nothrow__));









extern FILE *tmpfile (void);
# 180 "/usr/include/stdio.h" 3 4
extern char *tmpnam (char *__s) __attribute__ ((__nothrow__));





extern char *tmpnam_r (char *__s) __attribute__ ((__nothrow__));
# 198 "/usr/include/stdio.h" 3 4
extern char *tempnam (__const char *__dir, __const char *__pfx)
     __attribute__ ((__nothrow__)) __attribute__ ((__malloc__));








extern int fclose (FILE *__stream);




extern int fflush (FILE *__stream);

# 223 "/usr/include/stdio.h" 3 4
extern int fflush_unlocked (FILE *__stream);
# 237 "/usr/include/stdio.h" 3 4






extern FILE *fopen (__const char *__restrict __filename,
      __const char *__restrict __modes);




extern FILE *freopen (__const char *__restrict __filename,
        __const char *__restrict __modes,
        FILE *__restrict __stream);
# 264 "/usr/include/stdio.h" 3 4

# 275 "/usr/include/stdio.h" 3 4
extern FILE *fdopen (int __fd, __const char *__modes) __attribute__ ((__nothrow__));
# 296 "/usr/include/stdio.h" 3 4



extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__));





extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__));








extern int fprintf (FILE *__restrict __stream,
      __const char *__restrict __format, ...);




extern int printf (__const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      __const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg);




extern int vprintf (__const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));





extern int snprintf (char *__restrict __s, size_t __maxlen,
       __const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        __const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));

# 390 "/usr/include/stdio.h" 3 4





extern int fscanf (FILE *__restrict __stream,
     __const char *__restrict __format, ...);




extern int scanf (__const char *__restrict __format, ...);

extern int sscanf (__const char *__restrict __s,
     __const char *__restrict __format, ...) __attribute__ ((__nothrow__));

# 432 "/usr/include/stdio.h" 3 4





extern int fgetc (FILE *__stream);
extern int getc (FILE *__stream);





extern int getchar (void);

# 456 "/usr/include/stdio.h" 3 4
extern int getc_unlocked (FILE *__stream);
extern int getchar_unlocked (void);
# 467 "/usr/include/stdio.h" 3 4
extern int fgetc_unlocked (FILE *__stream);











extern int fputc (int __c, FILE *__stream);
extern int putc (int __c, FILE *__stream);





extern int putchar (int __c);

# 500 "/usr/include/stdio.h" 3 4
extern int fputc_unlocked (int __c, FILE *__stream);







extern int putc_unlocked (int __c, FILE *__stream);
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream);


extern int putw (int __w, FILE *__stream);








extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream);






extern char *gets (char *__s);

# 580 "/usr/include/stdio.h" 3 4





extern int fputs (__const char *__restrict __s, FILE *__restrict __stream);





extern int puts (__const char *__s);






extern int ungetc (int __c, FILE *__stream);






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream);




extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s);

# 633 "/usr/include/stdio.h" 3 4
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream);
extern size_t fwrite_unlocked (__const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream);








extern int fseek (FILE *__stream, long int __off, int __whence);




extern long int ftell (FILE *__stream);




extern void rewind (FILE *__stream);

# 688 "/usr/include/stdio.h" 3 4






extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos);




extern int fsetpos (FILE *__stream, __const fpos_t *__pos);
# 711 "/usr/include/stdio.h" 3 4

# 720 "/usr/include/stdio.h" 3 4


extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__));




extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__));
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__));








extern void perror (__const char *__s);






# 1 "/usr/include/bits/sys_errlist.h" 1 3 4
# 27 "/usr/include/bits/sys_errlist.h" 3 4
extern int sys_nerr;
extern __const char *__const sys_errlist[];
# 750 "/usr/include/stdio.h" 2 3 4




extern int fileno (FILE *__stream) __attribute__ ((__nothrow__));




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__));
# 769 "/usr/include/stdio.h" 3 4
extern FILE *popen (__const char *__command, __const char *__modes);





extern int pclose (FILE *__stream);





extern char *ctermid (char *__s) __attribute__ ((__nothrow__));
# 809 "/usr/include/stdio.h" 3 4
extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__));


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__));
# 830 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/stdio.h" 1 3 4
# 33 "/usr/include/bits/stdio.h" 3 4
extern __inline int
vprintf (__const char *__restrict __fmt, __gnuc_va_list __arg)
{
  return vfprintf (stdout, __fmt, __arg);
}


extern __inline int
getchar (void)
{
  return _IO_getc (stdin);
}




extern __inline int
getc_unlocked (FILE *__fp)
{
  return (__builtin_expect ((__fp)->_IO_read_ptr >= (__fp)->_IO_read_end, 0) ? __uflow (__fp) : *(unsigned char *) (__fp)->_IO_read_ptr++);
}


extern __inline int
getchar_unlocked (void)
{
  return (__builtin_expect ((stdin)->_IO_read_ptr >= (stdin)->_IO_read_end, 0) ? __uflow (stdin) : *(unsigned char *) (stdin)->_IO_read_ptr++);
}




extern __inline int
putchar (int __c)
{
  return _IO_putc (__c, stdout);
}




extern __inline int
fputc_unlocked (int __c, FILE *__stream)
{
  return (__builtin_expect ((__stream)->_IO_write_ptr >= (__stream)->_IO_write_end, 0) ? __overflow (__stream, (unsigned char) (__c)) : (unsigned char) (*(__stream)->_IO_write_ptr++ = (__c)));
}





extern __inline int
putc_unlocked (int __c, FILE *__stream)
{
  return (__builtin_expect ((__stream)->_IO_write_ptr >= (__stream)->_IO_write_end, 0) ? __overflow (__stream, (unsigned char) (__c)) : (unsigned char) (*(__stream)->_IO_write_ptr++ = (__c)));
}


extern __inline int
putchar_unlocked (int __c)
{
  return (__builtin_expect ((stdout)->_IO_write_ptr >= (stdout)->_IO_write_end, 0) ? __overflow (stdout, (unsigned char) (__c)) : (unsigned char) (*(stdout)->_IO_write_ptr++ = (__c)));
}
# 111 "/usr/include/bits/stdio.h" 3 4
extern __inline int
__attribute__ ((__nothrow__)) feof_unlocked (FILE *__stream)
{
  return (((__stream)->_flags & 0x10) != 0);
}


extern __inline int
__attribute__ ((__nothrow__)) ferror_unlocked (FILE *__stream)
{
  return (((__stream)->_flags & 0x20) != 0);
}
# 831 "/usr/include/stdio.h" 2 3 4






# 19 "lexer.c" 2
# 33 "lexer.c"
int yyleng; extern char yytext[];
int yymorfg;
extern char *yysptr, yysbuf[];
int yytchar;

extern int yylineno;
struct yysvf {
 struct yywork *yystoff;
 struct yysvf *yyother;
 int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;
# 53 "lexer.c"
static char *ReservedWords[] = {
"ADD",
"SUB",
"MUL",
"DIV",
"NOT",
"AND",
"OR",
"XOR",
"LT",
"LTE",
"EQ",
"GT",
"GTE",
"NEG",
"SQRT",
"ABS",
"FLOOR",
"CEIL",
"ROUND",
"DUP",
"POP",
"EXCH",
"COPY",
"ROLL",
"INDEX",
"CLEAR",
"STO",
"RCL",
"GOTO",
"IFG",
"IFNG",
"EXIT",
"EXE",
"ABORT",
"PRINTSTACK",
"PRINTPROGRAM",
"PRINTIMAGE",
"PRINTFRAME",
"ECHO",
"OPEN",
"CLOSE",
"EQU",
"VAL",
"STREAMNAME",
"COMPONENT",
"PICTURERATE",
"FRAMESKIP",
"QUANTIZATION",
"SEARCHLIMIT",
"NTSC",
"CIF",
"QCIF",
""};
# 170 "lexer.c"
static char *EquLabels[] = {
"SQUANT",
"MQUANT",
"PTYPE",
"MTYPE",
"BD",
"FDBD",
"BDBD",
"IDBD",
"VAROR",
"FVAR",
"BVAR",
"IVAR",
"DVAR",
"RATE",
"BUFFERSIZE",
"BUFFERCONTENTS",
"QDFACT",
"QOFFS",
""};
# 211 "lexer.c"
int CommentDepth = 0;
int yyint=0;
int LexDebug=0;
# 224 "lexer.c"
struct id {
  char *name;
  int tokentype;
  int count;
  int value;
};

struct link_def {
struct id *lid;
struct link_def *next;
};

struct id *Cid=((void *)0);



extern void initparser();
extern void parser();
extern void Execute();

static int hashpjw();
static struct link_def * MakeLink();
static struct id * enter();
static char * getstr();
static void PrintProgram();
static void MakeProgram();
static void CompileProgram();
static int mylex();
# 260 "lexer.c"
yylex(){
int nstr; extern int yyprevious;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if((1)) return(0); break;
case 1:
{}
break;
case 2:
{Cid = enter(0,yytext,yyleng);
   if (LexDebug)
     {
       printf("%s : %s (%d)\n",
       yytext,
       ((Cid->tokentype) ? "RESERVED" : "IDENTIFIER"),
       Cid->count);
     }
   if (Cid->tokentype)
     {
       return(Cid->tokentype);
     }
   else
     {
       yyint = Cid->value;
       return(1003);
     }
        }
break;
case 3:
        {if (LexDebug)
      {
        printf("%s : %s\n", yytext, "REAL");
      }
    return(1005);
         }
break;
case 4:
{if (LexDebug)
      {
        printf("%s : %s\n", yytext, "INTEGER");
      }
    yyint = atoi(yytext);
    return(1000);}
break;
case 5:
{if (LexDebug)
      {
        printf("%s : %s\n", yytext, "(HEX)INTEGER");
      }
    yyint = strtol(yytext+2,((void *)0),16);
    return(1000);}
break;
case 6:
{if (LexDebug)
      {
        printf("%s : %s\n", yytext, "(HEX)INTEGER");
      }
    yyint = strtol(yytext,((void *)0),16);
    return(1000);}
break;
case 7:
{if (LexDebug)
      {
        printf("%s : %s\n", yytext, "(OCT)INTEGER");
      }
    yyint = strtol(yytext+2,((void *)0),8);
    return(1000);}
break;
case 8:
{if (LexDebug)
      {
        printf("%s : %s\n", yytext, "(OCT)INTEGER");
      }
    yyint = strtol(yytext,((void *)0),8);
    return(1000);}
break;
case 9:
{if (LexDebug)
      {
        printf("%s : %s\n", yytext, "(CHAR)INTEGER");
      }
    if (yyleng>4)
      {
        yyint = strtol(yytext+2,((void *)0),8);
      }
    else
      {
        if (*(yytext+1)=='\\')
          {
     switch(*(yytext+2))
       {
       case '0':
         yyint=0;
         break;
       case 'b':
         yyint = 0x8;
         break;
       case 'i':
         yyint = 0x9;
         break;
       case 'n':
         yyint = 0xa;
         break;
       case 'v':
         yyint = 0xb;
         break;
       case 'f':
         yyint = 0xc;
         break;
       case 'r':
         yyint = 0xd;
         break;
       default:
         yyint=(*yytext+2);
         break;
       }
          }
        else
          {
     yyint = *(yytext+1);
          }
      }
    return(1000);}
break;
case 10:
        {if (LexDebug)
      {
        printf("%s : %s\n", yytext, "LBRACKET");
      }
    return(1001);}
break;
case 11:
        {if (LexDebug)
      {
        printf("%s : %s\n", yytext, "RBRACKET");
      }
    return(1002);}
break;
case 12:
{if (LexDebug)
      {
        printf("%s : %s\n", yytext, "STRING");
      }
    return(1004);}
break;
case 13:
{CommentDepth++; yybgin = yysvec + 1 + 4;}
break;
case 14:
 {CommentDepth--;if(!CommentDepth) yybgin = yysvec + 1 + 2;}
break;
case 15:
      {


                              printf("Bad input char '%c' on line %d\n",
                yytext[0],
                yylineno);
         }
break;
case 16:
 {}
break;
case -1:
break;
default:
fprintf(stdout,"bad switch yylook %d",nstr);
} return(0); }







struct link_def *HashTable[211];
int DataLevel = 0;
double DataStack[1024];
double *DataPtr;

int NextVal=0;


double Memory[1024];

int LocalLevel=0;
int CommandLevel=0;
double *LocalStack;
int *CommandStack;

int CurrentLine=0;
int CurrentProgram=0;
double ProgramLocalStack[10][2000];
int ProgramCommandStack[10][2000];
int ProgramLevel[10];
int ProgramLocalLevel[10];

int PProgram=0;
int PLevel=0;
int PLLevel=0;
int *PCStack=((void *)0);
double *PLStack=((void *)0);

int LabelLevel=0;
struct id *LabelStack[1000];

int SourceLevel=0;
int SourceProgramStack[16];
int SourceLineStack[16];
# 480 "lexer.c"
void initparser()
{
  char i,**sptr;
  yybgin = yysvec + 1 + 2;

  for(i=1,sptr=ReservedWords;**sptr!='\0';i++,sptr++)
    {
      enter(i,*sptr,strlen(*sptr));
    }
  for(i=1,sptr=EquLabels;**sptr!='\0';i++,sptr++)
    {
      equname(i,*sptr);
    }
  for(i=0;i<10;i++)
    {
      ProgramLevel[i]=0;
    }
  DataLevel=0;
  DataPtr = DataStack;
}



# 1 "globals.h" 1
# 30 "globals.h"
# 1 "/usr/include/stdio.h" 1 3 4
# 31 "globals.h" 2
# 1 "mem.h" 1
# 33 "mem.h"
typedef unsigned char **BLOCK;
typedef struct memory_construct * (* Ifunc)();


struct memory_construct {
int len;
int width;
int height;
unsigned char *data;
};
# 32 "globals.h" 2
# 1 "system.h" 1
# 28 "system.h"
# 1 "/usr/include/sys/file.h" 1 3 4
# 25 "/usr/include/sys/file.h" 3 4
# 1 "/usr/include/fcntl.h" 1 3 4
# 29 "/usr/include/fcntl.h" 3 4




# 1 "/usr/include/bits/fcntl.h" 1 3 4
# 25 "/usr/include/bits/fcntl.h" 3 4
# 1 "/usr/include/sys/types.h" 1 3 4
# 29 "/usr/include/sys/types.h" 3 4






typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;




typedef __loff_t loff_t;



typedef __ino_t ino_t;
# 62 "/usr/include/sys/types.h" 3 4
typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;





typedef __off_t off_t;
# 100 "/usr/include/sys/types.h" 3 4
typedef __pid_t pid_t;




typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;
# 133 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/time.h" 1 3 4
# 74 "/usr/include/time.h" 3 4


typedef __time_t time_t;



# 92 "/usr/include/time.h" 3 4
typedef __clockid_t clockid_t;
# 104 "/usr/include/time.h" 3 4
typedef __timer_t timer_t;
# 134 "/usr/include/sys/types.h" 2 3 4
# 147 "/usr/include/sys/types.h" 3 4
# 1 "/usr/lib/gcc/x86_64-redhat-linux/3.4.6/include/stddef.h" 1 3 4
# 148 "/usr/include/sys/types.h" 2 3 4



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
# 191 "/usr/include/sys/types.h" 3 4
typedef int int8_t __attribute__ ((__mode__ (__QI__)));
typedef int int16_t __attribute__ ((__mode__ (__HI__)));
typedef int int32_t __attribute__ ((__mode__ (__SI__)));
typedef int int64_t __attribute__ ((__mode__ (__DI__)));


typedef unsigned int u_int8_t __attribute__ ((__mode__ (__QI__)));
typedef unsigned int u_int16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int u_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int u_int64_t __attribute__ ((__mode__ (__DI__)));

typedef int register_t __attribute__ ((__mode__ (__word__)));
# 213 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 37 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/endian.h" 1 3 4
# 38 "/usr/include/endian.h" 2 3 4
# 214 "/usr/include/sys/types.h" 2 3 4


# 1 "/usr/include/sys/select.h" 1 3 4
# 31 "/usr/include/sys/select.h" 3 4
# 1 "/usr/include/bits/select.h" 1 3 4
# 32 "/usr/include/sys/select.h" 2 3 4


# 1 "/usr/include/bits/sigset.h" 1 3 4
# 23 "/usr/include/bits/sigset.h" 3 4
typedef int __sig_atomic_t;




typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
  } __sigset_t;
# 35 "/usr/include/sys/select.h" 2 3 4



typedef __sigset_t sigset_t;





# 1 "/usr/include/time.h" 1 3 4
# 118 "/usr/include/time.h" 3 4
struct timespec
  {
    __time_t tv_sec;
    long int tv_nsec;
  };
# 45 "/usr/include/sys/select.h" 2 3 4

# 1 "/usr/include/bits/time.h" 1 3 4
# 69 "/usr/include/bits/time.h" 3 4
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
# 47 "/usr/include/sys/select.h" 2 3 4


typedef __suseconds_t suseconds_t;





typedef long int __fd_mask;
# 67 "/usr/include/sys/select.h" 3 4
typedef struct
  {






    __fd_mask __fds_bits[1024 / (8 * sizeof (__fd_mask))];


  } fd_set;






typedef __fd_mask fd_mask;
# 99 "/usr/include/sys/select.h" 3 4

# 109 "/usr/include/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 128 "/usr/include/sys/select.h" 3 4

# 217 "/usr/include/sys/types.h" 2 3 4


# 1 "/usr/include/sys/sysmacros.h" 1 3 4
# 29 "/usr/include/sys/sysmacros.h" 3 4
__extension__
extern __inline unsigned int gnu_dev_major (unsigned long long int __dev)
     __attribute__ ((__nothrow__));
__extension__
extern __inline unsigned int gnu_dev_minor (unsigned long long int __dev)
     __attribute__ ((__nothrow__));
__extension__
extern __inline unsigned long long int gnu_dev_makedev (unsigned int __major,
       unsigned int __minor)
     __attribute__ ((__nothrow__));


__extension__ extern __inline unsigned int
__attribute__ ((__nothrow__)) gnu_dev_major (unsigned long long int __dev)
{
  return ((__dev >> 8) & 0xfff) | ((unsigned int) (__dev >> 32) & ~0xfff);
}

__extension__ extern __inline unsigned int
__attribute__ ((__nothrow__)) gnu_dev_minor (unsigned long long int __dev)
{
  return (__dev & 0xff) | ((unsigned int) (__dev >> 12) & ~0xff);
}

__extension__ extern __inline unsigned long long int
__attribute__ ((__nothrow__)) gnu_dev_makedev (unsigned int __major, unsigned int __minor)
{
  return ((__minor & 0xff) | ((__major & 0xfff) << 8)
   | (((unsigned long long int) (__minor & ~0xff)) << 12)
   | (((unsigned long long int) (__major & ~0xfff)) << 32));
}
# 220 "/usr/include/sys/types.h" 2 3 4
# 231 "/usr/include/sys/types.h" 3 4
typedef __blkcnt_t blkcnt_t;



typedef __fsblkcnt_t fsblkcnt_t;



typedef __fsfilcnt_t fsfilcnt_t;
# 266 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/bits/pthreadtypes.h" 1 3 4
# 23 "/usr/include/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/bits/sched.h" 1 3 4
# 83 "/usr/include/bits/sched.h" 3 4
struct __sched_param
  {
    int __sched_priority;
  };
# 24 "/usr/include/bits/pthreadtypes.h" 2 3 4


struct _pthread_fastlock
{
  long int __status;
  int __spinlock;

};



typedef struct _pthread_descr_struct *_pthread_descr;





typedef struct __pthread_attr_s
{
  int __detachstate;
  int __schedpolicy;
  struct __sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void *__stackaddr;
  size_t __stacksize;
} pthread_attr_t;





__extension__ typedef long long __pthread_cond_align_t;




typedef struct
{
  struct _pthread_fastlock __c_lock;
  _pthread_descr __c_waiting;
  char __padding[48 - sizeof (struct _pthread_fastlock)
   - sizeof (_pthread_descr) - sizeof (__pthread_cond_align_t)];
  __pthread_cond_align_t __align;
} pthread_cond_t;



typedef struct
{
  int __dummy;
} pthread_condattr_t;


typedef unsigned int pthread_key_t;





typedef struct
{
  int __m_reserved;
  int __m_count;
  _pthread_descr __m_owner;
  int __m_kind;
  struct _pthread_fastlock __m_lock;
} pthread_mutex_t;



typedef struct
{
  int __mutexkind;
} pthread_mutexattr_t;



typedef int pthread_once_t;
# 150 "/usr/include/bits/pthreadtypes.h" 3 4
typedef unsigned long int pthread_t;
# 267 "/usr/include/sys/types.h" 2 3 4



# 26 "/usr/include/bits/fcntl.h" 2 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 27 "/usr/include/bits/fcntl.h" 2 3 4
# 152 "/usr/include/bits/fcntl.h" 3 4
struct flock
  {
    short int l_type;
    short int l_whence;

    __off_t l_start;
    __off_t l_len;




    __pid_t l_pid;
  };
# 197 "/usr/include/bits/fcntl.h" 3 4



extern ssize_t readahead (int __fd, __off64_t __offset, size_t __count)
    __attribute__ ((__nothrow__));


# 34 "/usr/include/fcntl.h" 2 3 4
# 63 "/usr/include/fcntl.h" 3 4
extern int fcntl (int __fd, int __cmd, ...);
# 72 "/usr/include/fcntl.h" 3 4
extern int open (__const char *__file, int __oflag, ...) __attribute__ ((__nonnull__ (1)));
# 91 "/usr/include/fcntl.h" 3 4
extern int creat (__const char *__file, __mode_t __mode) __attribute__ ((__nonnull__ (1)));
# 120 "/usr/include/fcntl.h" 3 4
extern int lockf (int __fd, int __cmd, __off_t __len);
# 174 "/usr/include/fcntl.h" 3 4

# 26 "/usr/include/sys/file.h" 2 3 4



# 51 "/usr/include/sys/file.h" 3 4
extern int flock (int __fd, int __operation) __attribute__ ((__nothrow__));



# 29 "system.h" 2



struct io_buffer_list {
int hpos;
int vpos;
int hor;
int ver;
int width;
int height;
int flag;
struct memory_construct *mem;
};
# 33 "globals.h" 2
# 1 "huffman.h" 1
# 29 "huffman.h"
struct Modified_Decoder_Huffman
{
  int NumberStates;
  int state[512];
};

struct Modified_Encoder_Huffman
{
  int n;
  int *Hlen;
  int *Hcode;
};

struct Modified_Encoder_Huffman *MakeEHUFF();
struct Modified_Decoder_Huffman *MakeDHUFF();
# 34 "globals.h" 2
# 127 "globals.h"
typedef int iFunc();
typedef void vFunc();
# 147 "globals.h"
struct Image_Definition {
char *StreamFileName;
int PartialFrame;
int MpegMode;
int Height;
int Width;
};

struct Frame_Definition {
int NumberComponents;
char ComponentFilePrefix[3][200];
char ComponentFileSuffix[3][200];
char ComponentFileName[3][200];
int PHeight[3];
int PWidth[3];
int Height[3];
int Width[3];
int hf[3];
int vf[3];
};

struct FStore_Definition {
int NumberComponents;
struct io_buffer_list *Iob[3];
};

struct Statistics_Definition {
double mean;
double mse;
double mrsnr;
double snr;
double psnr;
double entropy;
};

struct Rate_Control_Definition {
int position;
int size;
int baseq;
};

# 1 "prototypes.h" 1
# 31 "prototypes.h"
int main();
extern void MpegEncodeSequence();
extern void MpegDecodeSequence();
extern void MpegEncodeIPBDFrame();
extern void MpegDecodeIPBDFrame();
extern void PrintImage();
extern void PrintFrame();
extern void MakeImage();
extern void MakeFrame();
extern void MakeFGroup();
extern void LoadFGroup();
extern void MakeFStore();
extern void MakeStat();
extern void SetCCITT();
extern void CreateFrameSizes();
extern void Help();
extern void MakeFileNames();
extern void VerifyFiles();
extern int Integer2TimeCode();
extern int TimeCode2Integer();



extern void EncodeAC();
extern void CBPEncodeAC();
extern void DecodeAC();
extern void CBPDecodeAC();
extern int DecodeDC();
extern void EncodeDC();



extern void inithuff();
extern int Encode();
extern int Decode();
extern void PrintDhuff();
extern void PrintEhuff();
extern void PrintTable();



extern void MakeIob();
extern void SuperSubCompensate();
extern void SubCompensate();
extern void AddCompensate();
extern void Sub2Compensate();
extern void Add2Compensate();
extern void MakeMask();
extern void ClearFS();
extern void InitFS();
extern void ReadFS();
extern void InstallIob();
extern void InstallFSIob();
extern void WriteFS();
extern void MoveTo();
extern int Bpos();
extern void ReadBlock();
extern void WriteBlock();
extern void PrintIob();



extern void ChenDct();
extern void ChenIDct();



extern void initparser();
extern void parser();



extern void ByteAlign();
extern void WriteVEHeader();
extern void WriteVSHeader();
extern int ReadVSHeader();
extern void WriteGOPHeader();
extern void ReadGOPHeader();
extern void WritePictureHeader();
extern void ReadPictureHeader();
extern void WriteMBSHeader();
extern void ReadMBSHeader();
extern void ReadHeaderTrailer();
extern int ReadHeaderHeader();
extern int ClearToHeader();
extern void WriteMBHeader();
extern int ReadMBHeader();



extern void initme();
extern void HPFastBME();
extern void BruteMotionEstimation();
extern void InterpolativeBME();




extern void CopyMem();
extern ClearMem();
extern SetMem();
extern struct memory_construct *MakeMem();
extern void FreeMem();
extern struct memory_construct *LoadMem();
extern struct memory_construct *LoadPartialMem();
extern struct memory_construct *SaveMem();
extern struct memory_construct *SavePartialMem();



extern void Statistics();



extern void readalign();
extern void mropen();
extern void mrclose();
extern void mwopen();
extern void mwclose();
extern void zeroflush();
extern int mgetb();
extern void mputv();
extern int mgetv();
extern long mwtell();
extern long mrtell();
extern void mwseek();
extern void mrseek();
extern int seof();



extern void ReferenceDct();
extern void ReferenceIDct();
extern void TransposeMatrix();
extern void MPEGIntraQuantize();
extern void MPEGIntraIQuantize();
extern void MPEGNonIntraQuantize();
extern void MPEGNonIntraIQuantize();
extern void BoundIntegerMatrix();
extern void BoundQuantizeMatrix();
extern void BoundIQuantizeMatrix();
extern void ZigzagMatrix();
extern void IZigzagMatrix();
extern void PrintMatrix();
extern void ClearMatrix();
# 189 "globals.h" 2
# 504 "lexer.c" 2
# 1 "stream.h" 1
# 31 "stream.h"
extern int bit_set_mask[];
# 505 "lexer.c" 2
# 1 "/usr/include/math.h" 1 3 4
# 29 "/usr/include/math.h" 3 4




# 1 "/usr/include/bits/huge_val.h" 1 3 4
# 34 "/usr/include/math.h" 2 3 4
# 46 "/usr/include/math.h" 3 4
# 1 "/usr/include/bits/mathdef.h" 1 3 4
# 47 "/usr/include/math.h" 2 3 4
# 70 "/usr/include/math.h" 3 4
# 1 "/usr/include/bits/mathcalls.h" 1 3 4
# 53 "/usr/include/bits/mathcalls.h" 3 4


extern double acos (double __x) __attribute__ ((__nothrow__)); extern double __acos (double __x) __attribute__ ((__nothrow__));

extern double asin (double __x) __attribute__ ((__nothrow__)); extern double __asin (double __x) __attribute__ ((__nothrow__));

extern double atan (double __x) __attribute__ ((__nothrow__)); extern double __atan (double __x) __attribute__ ((__nothrow__));

extern double atan2 (double __y, double __x) __attribute__ ((__nothrow__)); extern double __atan2 (double __y, double __x) __attribute__ ((__nothrow__));


extern double cos (double __x) __attribute__ ((__nothrow__)); extern double __cos (double __x) __attribute__ ((__nothrow__));

extern double sin (double __x) __attribute__ ((__nothrow__)); extern double __sin (double __x) __attribute__ ((__nothrow__));

extern double tan (double __x) __attribute__ ((__nothrow__)); extern double __tan (double __x) __attribute__ ((__nothrow__));




extern double cosh (double __x) __attribute__ ((__nothrow__)); extern double __cosh (double __x) __attribute__ ((__nothrow__));

extern double sinh (double __x) __attribute__ ((__nothrow__)); extern double __sinh (double __x) __attribute__ ((__nothrow__));

extern double tanh (double __x) __attribute__ ((__nothrow__)); extern double __tanh (double __x) __attribute__ ((__nothrow__));

# 87 "/usr/include/bits/mathcalls.h" 3 4


extern double acosh (double __x) __attribute__ ((__nothrow__)); extern double __acosh (double __x) __attribute__ ((__nothrow__));

extern double asinh (double __x) __attribute__ ((__nothrow__)); extern double __asinh (double __x) __attribute__ ((__nothrow__));

extern double atanh (double __x) __attribute__ ((__nothrow__)); extern double __atanh (double __x) __attribute__ ((__nothrow__));







extern double exp (double __x) __attribute__ ((__nothrow__)); extern double __exp (double __x) __attribute__ ((__nothrow__));


extern double frexp (double __x, int *__exponent) __attribute__ ((__nothrow__)); extern double __frexp (double __x, int *__exponent) __attribute__ ((__nothrow__));


extern double ldexp (double __x, int __exponent) __attribute__ ((__nothrow__)); extern double __ldexp (double __x, int __exponent) __attribute__ ((__nothrow__));


extern double log (double __x) __attribute__ ((__nothrow__)); extern double __log (double __x) __attribute__ ((__nothrow__));


extern double log10 (double __x) __attribute__ ((__nothrow__)); extern double __log10 (double __x) __attribute__ ((__nothrow__));


extern double modf (double __x, double *__iptr) __attribute__ ((__nothrow__)); extern double __modf (double __x, double *__iptr) __attribute__ ((__nothrow__));

# 127 "/usr/include/bits/mathcalls.h" 3 4


extern double expm1 (double __x) __attribute__ ((__nothrow__)); extern double __expm1 (double __x) __attribute__ ((__nothrow__));


extern double log1p (double __x) __attribute__ ((__nothrow__)); extern double __log1p (double __x) __attribute__ ((__nothrow__));


extern double logb (double __x) __attribute__ ((__nothrow__)); extern double __logb (double __x) __attribute__ ((__nothrow__));

# 152 "/usr/include/bits/mathcalls.h" 3 4


extern double pow (double __x, double __y) __attribute__ ((__nothrow__)); extern double __pow (double __x, double __y) __attribute__ ((__nothrow__));


extern double sqrt (double __x) __attribute__ ((__nothrow__)); extern double __sqrt (double __x) __attribute__ ((__nothrow__));





extern double hypot (double __x, double __y) __attribute__ ((__nothrow__)); extern double __hypot (double __x, double __y) __attribute__ ((__nothrow__));






extern double cbrt (double __x) __attribute__ ((__nothrow__)); extern double __cbrt (double __x) __attribute__ ((__nothrow__));








extern double ceil (double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern double __ceil (double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern double fabs (double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern double __fabs (double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern double floor (double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern double __floor (double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern double fmod (double __x, double __y) __attribute__ ((__nothrow__)); extern double __fmod (double __x, double __y) __attribute__ ((__nothrow__));




extern int __isinf (double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern int __finite (double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));





extern int isinf (double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern int finite (double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern double drem (double __x, double __y) __attribute__ ((__nothrow__)); extern double __drem (double __x, double __y) __attribute__ ((__nothrow__));



extern double significand (double __x) __attribute__ ((__nothrow__)); extern double __significand (double __x) __attribute__ ((__nothrow__));





extern double copysign (double __x, double __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern double __copysign (double __x, double __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__));

# 231 "/usr/include/bits/mathcalls.h" 3 4
extern int __isnan (double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));



extern int isnan (double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern double j0 (double) __attribute__ ((__nothrow__)); extern double __j0 (double) __attribute__ ((__nothrow__));
extern double j1 (double) __attribute__ ((__nothrow__)); extern double __j1 (double) __attribute__ ((__nothrow__));
extern double jn (int, double) __attribute__ ((__nothrow__)); extern double __jn (int, double) __attribute__ ((__nothrow__));
extern double y0 (double) __attribute__ ((__nothrow__)); extern double __y0 (double) __attribute__ ((__nothrow__));
extern double y1 (double) __attribute__ ((__nothrow__)); extern double __y1 (double) __attribute__ ((__nothrow__));
extern double yn (int, double) __attribute__ ((__nothrow__)); extern double __yn (int, double) __attribute__ ((__nothrow__));






extern double erf (double) __attribute__ ((__nothrow__)); extern double __erf (double) __attribute__ ((__nothrow__));
extern double erfc (double) __attribute__ ((__nothrow__)); extern double __erfc (double) __attribute__ ((__nothrow__));
extern double lgamma (double) __attribute__ ((__nothrow__)); extern double __lgamma (double) __attribute__ ((__nothrow__));

# 265 "/usr/include/bits/mathcalls.h" 3 4
extern double gamma (double) __attribute__ ((__nothrow__)); extern double __gamma (double) __attribute__ ((__nothrow__));






extern double lgamma_r (double, int *__signgamp) __attribute__ ((__nothrow__)); extern double __lgamma_r (double, int *__signgamp) __attribute__ ((__nothrow__));







extern double rint (double __x) __attribute__ ((__nothrow__)); extern double __rint (double __x) __attribute__ ((__nothrow__));


extern double nextafter (double __x, double __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern double __nextafter (double __x, double __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__));





extern double remainder (double __x, double __y) __attribute__ ((__nothrow__)); extern double __remainder (double __x, double __y) __attribute__ ((__nothrow__));



extern double scalbn (double __x, int __n) __attribute__ ((__nothrow__)); extern double __scalbn (double __x, int __n) __attribute__ ((__nothrow__));



extern int ilogb (double __x) __attribute__ ((__nothrow__)); extern int __ilogb (double __x) __attribute__ ((__nothrow__));
# 359 "/usr/include/bits/mathcalls.h" 3 4





extern double scalb (double __x, double __n) __attribute__ ((__nothrow__)); extern double __scalb (double __x, double __n) __attribute__ ((__nothrow__));
# 71 "/usr/include/math.h" 2 3 4
# 93 "/usr/include/math.h" 3 4
# 1 "/usr/include/bits/mathcalls.h" 1 3 4
# 53 "/usr/include/bits/mathcalls.h" 3 4


extern float acosf (float __x) __attribute__ ((__nothrow__)); extern float __acosf (float __x) __attribute__ ((__nothrow__));

extern float asinf (float __x) __attribute__ ((__nothrow__)); extern float __asinf (float __x) __attribute__ ((__nothrow__));

extern float atanf (float __x) __attribute__ ((__nothrow__)); extern float __atanf (float __x) __attribute__ ((__nothrow__));

extern float atan2f (float __y, float __x) __attribute__ ((__nothrow__)); extern float __atan2f (float __y, float __x) __attribute__ ((__nothrow__));


extern float cosf (float __x) __attribute__ ((__nothrow__)); extern float __cosf (float __x) __attribute__ ((__nothrow__));

extern float sinf (float __x) __attribute__ ((__nothrow__)); extern float __sinf (float __x) __attribute__ ((__nothrow__));

extern float tanf (float __x) __attribute__ ((__nothrow__)); extern float __tanf (float __x) __attribute__ ((__nothrow__));




extern float coshf (float __x) __attribute__ ((__nothrow__)); extern float __coshf (float __x) __attribute__ ((__nothrow__));

extern float sinhf (float __x) __attribute__ ((__nothrow__)); extern float __sinhf (float __x) __attribute__ ((__nothrow__));

extern float tanhf (float __x) __attribute__ ((__nothrow__)); extern float __tanhf (float __x) __attribute__ ((__nothrow__));

# 87 "/usr/include/bits/mathcalls.h" 3 4


extern float acoshf (float __x) __attribute__ ((__nothrow__)); extern float __acoshf (float __x) __attribute__ ((__nothrow__));

extern float asinhf (float __x) __attribute__ ((__nothrow__)); extern float __asinhf (float __x) __attribute__ ((__nothrow__));

extern float atanhf (float __x) __attribute__ ((__nothrow__)); extern float __atanhf (float __x) __attribute__ ((__nothrow__));







extern float expf (float __x) __attribute__ ((__nothrow__)); extern float __expf (float __x) __attribute__ ((__nothrow__));


extern float frexpf (float __x, int *__exponent) __attribute__ ((__nothrow__)); extern float __frexpf (float __x, int *__exponent) __attribute__ ((__nothrow__));


extern float ldexpf (float __x, int __exponent) __attribute__ ((__nothrow__)); extern float __ldexpf (float __x, int __exponent) __attribute__ ((__nothrow__));


extern float logf (float __x) __attribute__ ((__nothrow__)); extern float __logf (float __x) __attribute__ ((__nothrow__));


extern float log10f (float __x) __attribute__ ((__nothrow__)); extern float __log10f (float __x) __attribute__ ((__nothrow__));


extern float modff (float __x, float *__iptr) __attribute__ ((__nothrow__)); extern float __modff (float __x, float *__iptr) __attribute__ ((__nothrow__));

# 127 "/usr/include/bits/mathcalls.h" 3 4


extern float expm1f (float __x) __attribute__ ((__nothrow__)); extern float __expm1f (float __x) __attribute__ ((__nothrow__));


extern float log1pf (float __x) __attribute__ ((__nothrow__)); extern float __log1pf (float __x) __attribute__ ((__nothrow__));


extern float logbf (float __x) __attribute__ ((__nothrow__)); extern float __logbf (float __x) __attribute__ ((__nothrow__));

# 152 "/usr/include/bits/mathcalls.h" 3 4


extern float powf (float __x, float __y) __attribute__ ((__nothrow__)); extern float __powf (float __x, float __y) __attribute__ ((__nothrow__));


extern float sqrtf (float __x) __attribute__ ((__nothrow__)); extern float __sqrtf (float __x) __attribute__ ((__nothrow__));





extern float hypotf (float __x, float __y) __attribute__ ((__nothrow__)); extern float __hypotf (float __x, float __y) __attribute__ ((__nothrow__));






extern float cbrtf (float __x) __attribute__ ((__nothrow__)); extern float __cbrtf (float __x) __attribute__ ((__nothrow__));








extern float ceilf (float __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern float __ceilf (float __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern float fabsf (float __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern float __fabsf (float __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern float floorf (float __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern float __floorf (float __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern float fmodf (float __x, float __y) __attribute__ ((__nothrow__)); extern float __fmodf (float __x, float __y) __attribute__ ((__nothrow__));




extern int __isinff (float __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern int __finitef (float __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));





extern int isinff (float __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern int finitef (float __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern float dremf (float __x, float __y) __attribute__ ((__nothrow__)); extern float __dremf (float __x, float __y) __attribute__ ((__nothrow__));



extern float significandf (float __x) __attribute__ ((__nothrow__)); extern float __significandf (float __x) __attribute__ ((__nothrow__));





extern float copysignf (float __x, float __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern float __copysignf (float __x, float __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__));

# 231 "/usr/include/bits/mathcalls.h" 3 4
extern int __isnanf (float __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));



extern int isnanf (float __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern float j0f (float) __attribute__ ((__nothrow__)); extern float __j0f (float) __attribute__ ((__nothrow__));
extern float j1f (float) __attribute__ ((__nothrow__)); extern float __j1f (float) __attribute__ ((__nothrow__));
extern float jnf (int, float) __attribute__ ((__nothrow__)); extern float __jnf (int, float) __attribute__ ((__nothrow__));
extern float y0f (float) __attribute__ ((__nothrow__)); extern float __y0f (float) __attribute__ ((__nothrow__));
extern float y1f (float) __attribute__ ((__nothrow__)); extern float __y1f (float) __attribute__ ((__nothrow__));
extern float ynf (int, float) __attribute__ ((__nothrow__)); extern float __ynf (int, float) __attribute__ ((__nothrow__));






extern float erff (float) __attribute__ ((__nothrow__)); extern float __erff (float) __attribute__ ((__nothrow__));
extern float erfcf (float) __attribute__ ((__nothrow__)); extern float __erfcf (float) __attribute__ ((__nothrow__));
extern float lgammaf (float) __attribute__ ((__nothrow__)); extern float __lgammaf (float) __attribute__ ((__nothrow__));

# 265 "/usr/include/bits/mathcalls.h" 3 4
extern float gammaf (float) __attribute__ ((__nothrow__)); extern float __gammaf (float) __attribute__ ((__nothrow__));






extern float lgammaf_r (float, int *__signgamp) __attribute__ ((__nothrow__)); extern float __lgammaf_r (float, int *__signgamp) __attribute__ ((__nothrow__));







extern float rintf (float __x) __attribute__ ((__nothrow__)); extern float __rintf (float __x) __attribute__ ((__nothrow__));


extern float nextafterf (float __x, float __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern float __nextafterf (float __x, float __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__));





extern float remainderf (float __x, float __y) __attribute__ ((__nothrow__)); extern float __remainderf (float __x, float __y) __attribute__ ((__nothrow__));



extern float scalbnf (float __x, int __n) __attribute__ ((__nothrow__)); extern float __scalbnf (float __x, int __n) __attribute__ ((__nothrow__));



extern int ilogbf (float __x) __attribute__ ((__nothrow__)); extern int __ilogbf (float __x) __attribute__ ((__nothrow__));
# 359 "/usr/include/bits/mathcalls.h" 3 4





extern float scalbf (float __x, float __n) __attribute__ ((__nothrow__)); extern float __scalbf (float __x, float __n) __attribute__ ((__nothrow__));
# 94 "/usr/include/math.h" 2 3 4
# 114 "/usr/include/math.h" 3 4
# 1 "/usr/include/bits/mathcalls.h" 1 3 4
# 53 "/usr/include/bits/mathcalls.h" 3 4


extern long double acosl (long double __x) __attribute__ ((__nothrow__)); extern long double __acosl (long double __x) __attribute__ ((__nothrow__));

extern long double asinl (long double __x) __attribute__ ((__nothrow__)); extern long double __asinl (long double __x) __attribute__ ((__nothrow__));

extern long double atanl (long double __x) __attribute__ ((__nothrow__)); extern long double __atanl (long double __x) __attribute__ ((__nothrow__));

extern long double atan2l (long double __y, long double __x) __attribute__ ((__nothrow__)); extern long double __atan2l (long double __y, long double __x) __attribute__ ((__nothrow__));


extern long double cosl (long double __x) __attribute__ ((__nothrow__)); extern long double __cosl (long double __x) __attribute__ ((__nothrow__));

extern long double sinl (long double __x) __attribute__ ((__nothrow__)); extern long double __sinl (long double __x) __attribute__ ((__nothrow__));

extern long double tanl (long double __x) __attribute__ ((__nothrow__)); extern long double __tanl (long double __x) __attribute__ ((__nothrow__));




extern long double coshl (long double __x) __attribute__ ((__nothrow__)); extern long double __coshl (long double __x) __attribute__ ((__nothrow__));

extern long double sinhl (long double __x) __attribute__ ((__nothrow__)); extern long double __sinhl (long double __x) __attribute__ ((__nothrow__));

extern long double tanhl (long double __x) __attribute__ ((__nothrow__)); extern long double __tanhl (long double __x) __attribute__ ((__nothrow__));

# 87 "/usr/include/bits/mathcalls.h" 3 4


extern long double acoshl (long double __x) __attribute__ ((__nothrow__)); extern long double __acoshl (long double __x) __attribute__ ((__nothrow__));

extern long double asinhl (long double __x) __attribute__ ((__nothrow__)); extern long double __asinhl (long double __x) __attribute__ ((__nothrow__));

extern long double atanhl (long double __x) __attribute__ ((__nothrow__)); extern long double __atanhl (long double __x) __attribute__ ((__nothrow__));







extern long double expl (long double __x) __attribute__ ((__nothrow__)); extern long double __expl (long double __x) __attribute__ ((__nothrow__));


extern long double frexpl (long double __x, int *__exponent) __attribute__ ((__nothrow__)); extern long double __frexpl (long double __x, int *__exponent) __attribute__ ((__nothrow__));


extern long double ldexpl (long double __x, int __exponent) __attribute__ ((__nothrow__)); extern long double __ldexpl (long double __x, int __exponent) __attribute__ ((__nothrow__));


extern long double logl (long double __x) __attribute__ ((__nothrow__)); extern long double __logl (long double __x) __attribute__ ((__nothrow__));


extern long double log10l (long double __x) __attribute__ ((__nothrow__)); extern long double __log10l (long double __x) __attribute__ ((__nothrow__));


extern long double modfl (long double __x, long double *__iptr) __attribute__ ((__nothrow__)); extern long double __modfl (long double __x, long double *__iptr) __attribute__ ((__nothrow__));

# 127 "/usr/include/bits/mathcalls.h" 3 4


extern long double expm1l (long double __x) __attribute__ ((__nothrow__)); extern long double __expm1l (long double __x) __attribute__ ((__nothrow__));


extern long double log1pl (long double __x) __attribute__ ((__nothrow__)); extern long double __log1pl (long double __x) __attribute__ ((__nothrow__));


extern long double logbl (long double __x) __attribute__ ((__nothrow__)); extern long double __logbl (long double __x) __attribute__ ((__nothrow__));

# 152 "/usr/include/bits/mathcalls.h" 3 4


extern long double powl (long double __x, long double __y) __attribute__ ((__nothrow__)); extern long double __powl (long double __x, long double __y) __attribute__ ((__nothrow__));


extern long double sqrtl (long double __x) __attribute__ ((__nothrow__)); extern long double __sqrtl (long double __x) __attribute__ ((__nothrow__));





extern long double hypotl (long double __x, long double __y) __attribute__ ((__nothrow__)); extern long double __hypotl (long double __x, long double __y) __attribute__ ((__nothrow__));






extern long double cbrtl (long double __x) __attribute__ ((__nothrow__)); extern long double __cbrtl (long double __x) __attribute__ ((__nothrow__));








extern long double ceill (long double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern long double __ceill (long double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern long double fabsl (long double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern long double __fabsl (long double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern long double floorl (long double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern long double __floorl (long double __x) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern long double fmodl (long double __x, long double __y) __attribute__ ((__nothrow__)); extern long double __fmodl (long double __x, long double __y) __attribute__ ((__nothrow__));




extern int __isinfl (long double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern int __finitel (long double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));





extern int isinfl (long double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern int finitel (long double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern long double dreml (long double __x, long double __y) __attribute__ ((__nothrow__)); extern long double __dreml (long double __x, long double __y) __attribute__ ((__nothrow__));



extern long double significandl (long double __x) __attribute__ ((__nothrow__)); extern long double __significandl (long double __x) __attribute__ ((__nothrow__));





extern long double copysignl (long double __x, long double __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern long double __copysignl (long double __x, long double __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__));

# 231 "/usr/include/bits/mathcalls.h" 3 4
extern int __isnanl (long double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));



extern int isnanl (long double __value) __attribute__ ((__nothrow__)) __attribute__ ((__const__));


extern long double j0l (long double) __attribute__ ((__nothrow__)); extern long double __j0l (long double) __attribute__ ((__nothrow__));
extern long double j1l (long double) __attribute__ ((__nothrow__)); extern long double __j1l (long double) __attribute__ ((__nothrow__));
extern long double jnl (int, long double) __attribute__ ((__nothrow__)); extern long double __jnl (int, long double) __attribute__ ((__nothrow__));
extern long double y0l (long double) __attribute__ ((__nothrow__)); extern long double __y0l (long double) __attribute__ ((__nothrow__));
extern long double y1l (long double) __attribute__ ((__nothrow__)); extern long double __y1l (long double) __attribute__ ((__nothrow__));
extern long double ynl (int, long double) __attribute__ ((__nothrow__)); extern long double __ynl (int, long double) __attribute__ ((__nothrow__));






extern long double erfl (long double) __attribute__ ((__nothrow__)); extern long double __erfl (long double) __attribute__ ((__nothrow__));
extern long double erfcl (long double) __attribute__ ((__nothrow__)); extern long double __erfcl (long double) __attribute__ ((__nothrow__));
extern long double lgammal (long double) __attribute__ ((__nothrow__)); extern long double __lgammal (long double) __attribute__ ((__nothrow__));

# 265 "/usr/include/bits/mathcalls.h" 3 4
extern long double gammal (long double) __attribute__ ((__nothrow__)); extern long double __gammal (long double) __attribute__ ((__nothrow__));






extern long double lgammal_r (long double, int *__signgamp) __attribute__ ((__nothrow__)); extern long double __lgammal_r (long double, int *__signgamp) __attribute__ ((__nothrow__));







extern long double rintl (long double __x) __attribute__ ((__nothrow__)); extern long double __rintl (long double __x) __attribute__ ((__nothrow__));


extern long double nextafterl (long double __x, long double __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__)); extern long double __nextafterl (long double __x, long double __y) __attribute__ ((__nothrow__)) __attribute__ ((__const__));





extern long double remainderl (long double __x, long double __y) __attribute__ ((__nothrow__)); extern long double __remainderl (long double __x, long double __y) __attribute__ ((__nothrow__));



extern long double scalbnl (long double __x, int __n) __attribute__ ((__nothrow__)); extern long double __scalbnl (long double __x, int __n) __attribute__ ((__nothrow__));



extern int ilogbl (long double __x) __attribute__ ((__nothrow__)); extern int __ilogbl (long double __x) __attribute__ ((__nothrow__));
# 359 "/usr/include/bits/mathcalls.h" 3 4





extern long double scalbl (long double __x, long double __n) __attribute__ ((__nothrow__)); extern long double __scalbl (long double __x, long double __n) __attribute__ ((__nothrow__));
# 115 "/usr/include/math.h" 2 3 4
# 130 "/usr/include/math.h" 3 4
extern int signgam;
# 257 "/usr/include/math.h" 3 4
typedef enum
{
  _IEEE_ = -1,
  _SVID_,
  _XOPEN_,
  _POSIX_,
  _ISOC_
} _LIB_VERSION_TYPE;




extern _LIB_VERSION_TYPE _LIB_VERSION;
# 282 "/usr/include/math.h" 3 4
struct exception

  {
    int type;
    char *name;
    double arg1;
    double arg2;
    double retval;
  };




extern int matherr (struct exception *__exc);
# 382 "/usr/include/math.h" 3 4
# 1 "/usr/include/bits/mathinline.h" 1 3 4
# 383 "/usr/include/math.h" 2 3 4
# 438 "/usr/include/math.h" 3 4

# 506 "lexer.c" 2



extern struct Frame_Definition *CFrame;
extern struct Image_Definition *CImage;
extern int ErrorValue;
extern int Prate;
extern int FrameSkip;
extern int SearchLimit;
extern int ImageType;
extern int InitialQuant;







static int hashpjw(s)
     char *s;
{
  static char RoutineName[]= "hashpjw";
  char *p;
  unsigned int h=0,g;

  for(p=s;*p!='\0';p++)
    {
      h = (h << 4) + *p;
      if (g = h&0xf0000000)
 {
   h = h ^(g >> 24);
   h = h ^ g;
 }
    }
  return(h % 211);
}
# 551 "lexer.c"
static struct link_def *MakeLink(tokentype,str,len)
     int tokentype;
     char *str;
     int len;
{
  static char RoutineName[]= "MakeLink";
  struct link_def *temp;

  if (!(temp = ((struct link_def *) malloc(sizeof(struct link_def)))))
    {
      printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,561);
      printf("Cannot make a LINK.\n");
      exit(12);
    }
  if (!(temp->lid = ((struct id *) malloc(sizeof(struct id)))))
    {
      printf("Cannot make an id.\n");
      exit(12);
    }
  temp->next = ((void *)0);
  if (!(temp->lid->name =(char *)calloc(len+1,sizeof(char))))
    {
      printf("Cannot make a string space for the link.\n");
      exit(12);
    }
  strcpy(temp->lid->name,str);
  temp->lid->tokentype = tokentype;
  temp->lid->count = 1;
  temp->lid->value = -1;
  return(temp);
}







static struct id *enter(tokentype,str,len)
     int tokentype;
     char *str;
     int len;
{
  static char RoutineName[]= "enter";
  int hashnum;
  struct link_def *temp,*current;
  char *ptr;

  for(ptr=str;*ptr!='\0';ptr++)
    {
      if ((*ptr>='a') && (*ptr<='z'))
 {
   *ptr = *ptr - ('a'-'A');
 }
    }
  hashnum = hashpjw(str);
  for(temp=((void *)0),current=HashTable[hashnum];
      current!= ((void *)0);
      current=current->next)
    {
      if (strcmp(str,current->lid->name) == 0)
 {
   temp=current;
   break;
 }
    }
  if (temp)
    {
      temp->lid->count++;
      return(temp->lid);
    }
  else
    {
      temp = MakeLink(tokentype,str,len);
      {if(!HashTable[hashnum]){HashTable[hashnum]=temp;}else{temp->next=HashTable[hashnum];HashTable[hashnum]=temp;}};
      return(temp->lid);
    }
}

equname(number,name)
     int number;
     char *name;
{
  struct id *temp;
  temp = enter(0,name,strlen(name));
  temp->value=number;
}
# 647 "lexer.c"
static char *getstr()
{
  static char RoutineName[]= "getstr";
  char *tmp,*ptr,*bptr;
  int i,accum,flag;
  if (mylex() != 1004)
    {
      printf("String expected.\n");
      if (!(tmp=(char *) malloc(sizeof(char))))
 {
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,657);
   printf("Cannot allocate for null string.\n");
   exit(12);
 }
      *tmp='\0';
      return(tmp);
    }
  if (!(tmp=(char *)calloc(strlen(yytext)+1,sizeof(char))))
    {
      printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,666);
      printf("Cannot allocate %d string space.\n",yyleng);
      exit(12);
    }
  for(bptr=yytext+1,ptr=tmp;*bptr!='"';bptr++,ptr++)
    {
      if (*bptr=='\\')
 {
   bptr++;
   for(flag=0,accum=0,i=0;i<3;i++)
     {
       if ((*bptr>='0')&&(*bptr<='7'))
  {
    accum = (accum<<3)+(*bptr-'0');
    bptr++;
    flag=1;
  }
       else {break;}
     }
   if (flag) {bptr--;*ptr=accum;}
   else
     {
       switch(*(bptr))
  {
  case '0':
    *ptr = 0;
    break;
  case 'b':
    *ptr = 0x8;
    break;
  case 'i':
    *ptr = 0x9;
    break;
  case 'n':
    *ptr = 0xa;
    break;
  case 'v':
    *ptr = 0xb;
    break;
  case 'f':
    *ptr = 0xc;
    break;
  case 'r':
    *ptr = 0xd;
    break;
  default:
    *ptr=(*bptr);
  }
     }
 }
      else {*ptr = (*bptr);}
    }
  *ptr='\0';
  return(tmp);
}
# 814 "lexer.c"
void parser()
{
  static char RoutineName[]= "parser";
  int i,dest,value,token,ntoken,arrayflag;
  double accum;
  int hold;
  char *sptr;

  while(token=mylex())
    {
      arrayflag=0;
      switch(token)
  {
 case 1000:
    *(DataPtr++) = (double) ((double) yyint); DataLevel++;;
    break;
  case 1005:
    *(DataPtr++) = (double) (atof(yytext)); DataLevel++;;
    break;

  case 1:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); *(--DataPtr) += accum; DataPtr++; DataLevel--;;
   break;
  case 2:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); *(--DataPtr) -= accum; DataPtr++; DataLevel--;;
    break;
 case 3:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); *(--DataPtr) *= accum; DataPtr++; DataLevel--;;
    break;
  case 4:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); *(--DataPtr) /= accum; DataPtr++; DataLevel--;;
    break;
  case 5:
   accum = *(--DataPtr);
    *(DataPtr++) = (accum ? 0.0 : 1.0);
    break;
  case 6:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); DataPtr--; if (*(DataPtr) && (accum)) *(DataPtr++) = 1.0; else *(DataPtr++) = 0.0; DataLevel--;;
    break;
  case 7:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); DataPtr--; if (*(DataPtr) || (accum)) *(DataPtr++) = 1.0; else *(DataPtr++) = 0.0; DataLevel--;;
    break;
  case 8:
   if (DataLevel<2)
     {
       printf("Not enough operands on stack.\n");
       break;
     }
   accum = *(--DataPtr); DataPtr--;
   if ((*(DataPtr) && !(accum))||
       (!(*(DataPtr)) && (accum))) *(DataPtr++) = 1.0;
          else *(DataPtr++) = 0.0;
   DataLevel--;
    break;
  case 9:
   if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); DataPtr--; if (*(DataPtr) < (accum)) *(DataPtr++) = 1.0; else *(DataPtr++) = 0.0; DataLevel--;;
    break;
  case 10:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); DataPtr--; if (*(DataPtr) <= (accum)) *(DataPtr++) = 1.0; else *(DataPtr++) = 0.0; DataLevel--;;
    break;
 case 11:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); DataPtr--; if (*(DataPtr) == (accum)) *(DataPtr++) = 1.0; else *(DataPtr++) = 0.0; DataLevel--;;
    break;
  case 12:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); DataPtr--; if (*(DataPtr) > (accum)) *(DataPtr++) = 1.0; else *(DataPtr++) = 0.0; DataLevel--;;
    break;
  case 13:
    if (DataLevel<2) { printf("Not enough operands on stack.\n"); break; } accum = *(--DataPtr); DataPtr--; if (*(DataPtr) >= (accum)) *(DataPtr++) = 1.0; else *(DataPtr++) = 0.0; DataLevel--;;
    break;

  case 14:
    accum = *(--DataPtr);
    *(DataPtr++) = -(accum);
   break;
  case 15:
    accum = *(--DataPtr);
   *(DataPtr++) = sqrt(accum);
    break;
  case 16:
    accum = *(--DataPtr);
   *(DataPtr++) = fabs(accum);
    break;
  case 17:
    accum = *(--DataPtr);
   *(DataPtr++) = floor(accum);
    break;
  case 18:
    accum = *(--DataPtr);
   *(DataPtr++) = ceil(accum);
    break;
  case 19:
    accum = *(--DataPtr);
   *(DataPtr++) = ((accum<0)?ceil(accum-0.5):floor(accum+0.5));
    break;

 case 20:
    *(DataPtr) = DataPtr[-1];
   DataPtr++;
    DataLevel++;
    break;
  case 21:
   if (DataLevel)
      {
        DataLevel--;
        DataPtr--;
      }
    else {printf("Not enough stack elements.\n");}
    break;
  case 22:
    *DataPtr = DataPtr[-1];
    DataPtr[-1] = DataPtr[-2];
    DataPtr[-2] = *DataPtr;
    break;
  case 23:
   (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,928); printf("Integer expected.\n"); break;};
    if (DataLevel<yyint)
      {
        printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,931);
        printf("Not enough elements\n");
        break;
      }
    for(i=0;i<yyint;i++)
      {
        *(DataPtr) = DataPtr[-yyint];
        DataPtr++;
        DataLevel++;
      }
    break;
  case 24:
   (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,943); printf("Integer expected.\n"); break;};
   dest=yyint;
    (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,945); printf("Integer expected.\n"); break;};
   value=yyint;
   value = value % dest;
    if (value<0) {value+= dest;}
    for(i=0;i<value;i++)
     {DataPtr[i] = DataPtr[i-value];}
    for(i=0;i<dest-value;i++)
      {DataPtr[-i-1] = DataPtr[-value-i-1];}
    for(i=0;i<value;i++)
      {DataPtr[i-dest] = DataPtr[i];}
    break;
  case 25:
   (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,957); printf("Integer expected.\n"); break;};
   if (yyint > DataLevel)
      {
       printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,960);
        printf("Index out of bounds\n");
        break;
      }
    *DataPtr = DataPtr[-yyint];
    DataPtr++;
    DataLevel++;
    break;
  case 26:
    DataLevel=0;
   DataPtr=DataStack;
    break;

  case 27:
    if (!DataLevel)
      {
        printf("Not enough stack elements.\n");
      }
    ntoken = mylex();
    if ((ntoken!=1003)&&(ntoken!=1000))
      {
       printf("Integer or label expected.\n");
        break;
      }
    Memory[yyint]= *(--DataPtr);
    DataLevel--;
   break;
  case 28:
    ntoken = mylex();
    if ((ntoken!=1003)&&(ntoken!=1000))
      {
       printf("Integer or label expected.\n");
        break;
      }
    *(DataPtr++) = (double) (Memory[yyint]); DataLevel++;;
    break;

 case 29:
 case 30:
 case 31:
 case 32:
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1001);
   printf("Program commands not available on top-level.\n");
   break;

  case 33:
    ntoken = mylex();
    if ((ntoken!=1003)&&(ntoken!=1000))
     {
       printf("Integer or label expected.\n");
       break;
     }
   SourceProgramStack[SourceLevel] = CurrentProgram; SourceLineStack[SourceLevel] = CurrentLine; SourceLevel++; CurrentProgram = yyint; CurrentLine = 0; CommandStack = ProgramCommandStack[CurrentProgram]; LocalStack = ProgramLocalStack[CurrentProgram]; LocalLevel = ProgramLocalLevel[CurrentProgram]; CommandLevel = ProgramLevel[CurrentProgram];;
   break;
 case 34:
   (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1015); printf("Integer expected.\n"); break;};
   exit(yyint);
   break;

  case 35:
    for(i=0;i<DataLevel;i++)
      {
        printf("%d: %f\n",i,DataStack[i]);
      }
    break;
  case 36:
    ntoken = mylex();
    if ((ntoken!=1003)&&(ntoken!=1000))
      {
        printf("Integer or label expected.\n");
        break;
     }
    PProgram=(yyint); PLStack = ProgramLocalStack[(yyint)]; PCStack = ProgramCommandStack[(yyint)]; PLLevel = ProgramLocalLevel[(yyint)]; PLevel = ProgramLevel[(yyint)];;
    PrintProgram();
    break;
 case 37:
   PrintImage();
   break;
 case 38:
   PrintFrame();
   break;

 case 39:
   printf("%s\n",getstr());
   break;
  case 40:
    ntoken = mylex();
    if ((ntoken!=1003)&&(ntoken!=1000))
      {
        printf("Integer or label expected.\n");
        break;
      }
    hold = yyint;
   PProgram=(hold); PLStack = ProgramLocalStack[(hold)]; PCStack = ProgramCommandStack[(hold)]; PLLevel = ProgramLocalLevel[(hold)]; PLevel = ProgramLevel[(hold)];;
    PLevel=0;
    MakeProgram();
    CompileProgram();
    ProgramLevel[hold]=PLevel;
    ProgramLocalLevel[hold]=PLLevel;
   break;
 case 41:
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1061);
   printf("Close not available on top level.\n");
   break;

  case 42:
   if (!DataLevel)
      {
        printf("Not enough stack elements.\n");
      }
    ntoken = mylex();
    if ((ntoken!=1003))
      {
        printf("Label expected.\n");
       break;
     }
    Cid->value = (int) *(--DataPtr);
    DataLevel--;
    break;
 case 43:
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1080);
   printf("VAL is not a valid id on top level.\n");
   break;

 case 44:
   CImage->StreamFileName=getstr();
   break;
 case 45:
   ntoken=mylex();
   if (ntoken==1001) { arrayflag=1; ntoken=mylex(); } if (ntoken!=1000) { printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1089); printf("Expected integer.\n"); break; } while(1) {;
   dest = yyint;
   ntoken=mylex();
   if (ntoken!=1001)
     {
       printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1094);
       printf("Left bracket expected.\n");
       break;
     }
   sptr=getstr();
   strcpy(CFrame->ComponentFilePrefix[dest],sptr);
   sptr=getstr();
   strcpy(CFrame->ComponentFileSuffix[dest],sptr);
   ntoken=mylex();
   if (ntoken!=1002)
     {
       printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1105);
       printf("Right bracket expected.\n");
       break;
     }
   if (arrayflag) { if ((ntoken=mylex())==1002) break; else if (ntoken!=1000) { printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1109); printf("Expected integer or right bracket.\n"); break; } } else break; };
   break;
        case 46:
   (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1112); printf("Integer expected.\n"); break;};
   Prate = yyint;
   break;
 case 47:
   (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1116); printf("Integer expected.\n"); break;};

   break;
 case 48:
   (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1120); printf("Integer expected.\n"); break;};
   InitialQuant = yyint;
   break;
 case 49:
   (ntoken)=mylex(); if ((ntoken)!=1000) {printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1124); printf("Integer expected.\n"); break;};


   break;
 case 50:
   ImageType=0;
   break;
 case 51:
   ImageType=1;
   break;
 case 52:
   ImageType=2;
   break;
 default:
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1138);
   printf("Illegal token type encountered: %d\n",token);
   break;
 }
    }
}







static void PrintProgram()
{
  static char RoutineName[]= "PrintProgram";
  int i;

  for(i=0;i<PLevel;i++)
    {
      switch(PCStack[i])
 {
 case 1:
 case 2:
 case 3:
 case 4:

 case 5:
 case 6:
 case 7:
 case 8:
 case 9:
 case 10:
 case 11:
 case 12:
 case 13:

 case 14:
 case 15:
 case 16:
 case 17:
 case 18:
 case 19:

 case 20:
 case 21:
 case 22:
 case 26:
 case 32:
 case 35:
 case 37:
 case 38:
   printf("%d: %s\n",
   i,
   ReservedWords[PCStack[i]-1]);
   break;
 case 23:
 case 25:
 case 27:
 case 28:
 case 33:
 case 34:
 case 36:
   printf("%d: %s %d\n",
   i,
   ReservedWords[PCStack[i]-1],
   PCStack[i+1]);
   i++;
   break;
 case 24:
   printf("%d: %s %d %d\n",
   i,
   ReservedWords[PCStack[i]-1],
   PCStack[i+1],
   PCStack[i+2]);
   i+=2;
   break;
 case 29:
 case 30:
 case 31:
   printf("%d: %s %d\n",
   i,
   ReservedWords[PCStack[i]-1],
   PCStack[i+1]);
   i++;
   break;
 case 43:
   printf("%d: %s %f\n",
   i,
   ReservedWords[PCStack[i]-1],
   PLStack[PCStack[i+1]]);
   i++;
   break;
 case 39:
 case 40:
 case 41:
 case 42:
 case 44:
 case 45:
 case 46:
 case 47:
 case 48:
 case 49:
 case 50:
 case 51:
 case 52:
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1244);
   printf("Top-level token occurring in program: %s.\n",
   ReservedWords[PCStack[i]-1]);
   break;
 default:
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1249);
   printf("Bad token type %d\n",PCStack[i]);
   break;
 }
    }
}







static void MakeProgram()
{
  static char RoutineName[]= "MakeProgram";
  int ntoken;

  while((ntoken=mylex())!= 41)
    {
      switch(ntoken)
 {
 case 0:
   exit(-1);
   break;
 case 1:
 case 2:
 case 3:
 case 4:

 case 5:
 case 6:
 case 7:
 case 8:
 case 9:
 case 10:
 case 11:
 case 12:
 case 13:

 case 14:
 case 15:
 case 16:
 case 17:
 case 18:
 case 19:

 case 20:
 case 21:
 case 22:
 case 26:

 case 32:
 case 35:
 case 37:
 case 38:
   PCStack[PLevel++] = ntoken;
   break;
 case 23:
 case 25:
 case 27:
 case 28:
 case 33:
 case 34:
 case 36:
   PCStack[PLevel++] = ntoken;
   ntoken = mylex();
   if ((ntoken==1000)||(ntoken==1003))
     {
       PCStack[PLevel++] = yyint;
     }
   else
     {
       PCStack[PLevel++] = 0;
       printf("Integer expected.\n");
     }
   break;
 case 24:
   PCStack[PLevel++] = ntoken;
   ntoken = mylex();
   if ((ntoken==1000)||(ntoken==1003))
     {
       PCStack[PLevel++] = yyint;
     }
   else
     {
       PCStack[PLevel++] = 0;
       printf("Integer expected.\n");
     }
   ntoken = mylex();
   if ((ntoken==1000)||(ntoken==1003))
     {
       PCStack[PLevel++] = yyint;
     }
   else
     {
       PCStack[PLevel++] = 0;
       printf("Integer expected.\n");
     }
   break;
 case 29:
 case 30:
 case 31:
   PCStack[PLevel++] = ntoken;
   ntoken = mylex();
   if (ntoken==1003)
     {
       LabelStack[LabelLevel] = Cid;
       PCStack[PLevel++] = LabelLevel++;
     }
   else
     {
       printf("Id expected.\n");
     }
   break;
 case 43:
   PCStack[PLevel++] = ntoken;
   PLStack[PLLevel]=(double) *(--DataPtr);
   DataLevel--;
   PCStack[PLevel++] = PLLevel++;
   break;
 case 1000:
   PCStack[PLevel++] = 43;
   PLStack[PLLevel]=(double) yyint;
   PCStack[PLevel++] = PLLevel++;
   break;
 case 1005:
   PCStack[PLevel++] = 43;
   PLStack[PLLevel] = atof(yytext);
   PCStack[PLevel++] = PLLevel++;
   break;
 case 1003:
   if (Cid->value>=0)
     {
       printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1383);
       printf("Attempt to redefine label.\n");
       break;
     }
   Cid->value = PLevel;
   break;
 default:
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1390);
   printf("Token type %d not allowed in programs.\n",ntoken);
   break;
 }
    }
}







static void CompileProgram()
{
  static char RoutineName[]= "CompileProgram";
  int i;

  for(i=0;i<PLevel;i++)
    {
      switch(PCStack[i])
 {
 case 1:
 case 2:
 case 3:
 case 4:

 case 5:
 case 6:
 case 7:
 case 8:
 case 9:
 case 10:
 case 11:
 case 12:
 case 13:

 case 14:
 case 15:
 case 16:
 case 17:
 case 18:
 case 19:

 case 20:
 case 21:
 case 22:
 case 26:

 case 32:
 case 35:
 case 37:
 case 38:
   break;
 case 23:
 case 25:
 case 27:
 case 28:
 case 33:
 case 34:
 case 43:
 case 36:
   i++;
   break;
 case 24:
   i+=2;
   break;
 case 29:
 case 30:
 case 31:
   i++;
   if (!LabelStack[PCStack[i]]->value)
     {
       printf("Bad reference to label!\n");
       break;
     }
   PCStack[i] = LabelStack[PCStack[i]]->value;
   break;
 default:
   printf("F>%s:R>%s:L>%d: ", "lexer.c",RoutineName,1469);
   printf("Invalid program compilation token: %d.\n",PCStack[i]);
   break;
 }
    }
}
# 1483 "lexer.c"
static int mylex()
{
  static char RoutineName[]= "mylex";
  int token;

  while(1)
    {
      if (!SourceLevel)
 {
   return(yylex());
 }
      token = CommandStack[CurrentLine++];




      if (NextVal)
 {
   NextVal--;
   yyint = token;
   return(1000);
 }
      switch(token)
 {
 case 0:
   printf("Abnormal break at: %d\n",CurrentLine);
   SourceLevel--; CurrentProgram = SourceProgramStack[SourceLevel]; CurrentLine = SourceLineStack[SourceLevel]; CommandStack = ProgramCommandStack[CurrentProgram]; LocalStack = ProgramLocalStack[CurrentProgram]; LocalLevel = ProgramLocalLevel[CurrentProgram]; CommandLevel = ProgramLevel[CurrentProgram];;
   break;
 case 43:
   *(DataPtr++) = (double) (LocalStack[CommandStack[CurrentLine++]]); DataLevel++;;
   break;
 case 29:
   CurrentLine = CommandStack[CurrentLine];
   break;
 case 30:
   DataLevel--;
   if (*(--DataPtr))
     {
       CurrentLine = CommandStack[CurrentLine];
     }
   else CurrentLine++;
   break;
 case 31:
   DataLevel--;
   if (!(*(--DataPtr)))
     {
       CurrentLine = CommandStack[CurrentLine];
     }
   else CurrentLine++;
   break;
 case 32:
   SourceLevel--; CurrentProgram = SourceProgramStack[SourceLevel]; CurrentLine = SourceLineStack[SourceLevel]; CommandStack = ProgramCommandStack[CurrentProgram]; LocalStack = ProgramLocalStack[CurrentProgram]; LocalLevel = ProgramLocalLevel[CurrentProgram]; CommandLevel = ProgramLevel[CurrentProgram];;
   break;
 case 23:
 case 25:
 case 27:
 case 28:
 case 33:
 case 34:
 case 36:
   NextVal = 1;
   return(token);
   break;
 case 24:
   NextVal = 2;
   return(token);
   break;
 default:
   return(token);
   break;
 }
    }
}
# 1564 "lexer.c"
void Execute(pnum)
     int pnum;
{
  static char RoutineName[]= "Execute";

  if (ProgramLevel[pnum])
    {
      SourceProgramStack[SourceLevel] = CurrentProgram; SourceLineStack[SourceLevel] = CurrentLine; SourceLevel++; CurrentProgram = pnum; CurrentLine = 0; CommandStack = ProgramCommandStack[CurrentProgram]; LocalStack = ProgramLocalStack[CurrentProgram]; LocalLevel = ProgramLocalLevel[CurrentProgram]; CommandLevel = ProgramLevel[CurrentProgram];;
      parser();
    }
}




int yyvstop[] ={
0,

15,
0,

1,
15,
0,

1,
0,

15,
0,

15,
0,

15,
0,

15,
0,

15,
0,

4,
15,
0,

4,
15,
0,

4,
15,
0,

2,
15,
0,

2,
15,
0,

10,
15,
0,

11,
15,
0,

16,
0,

16,
0,

16,
0,

12,
0,

4,
0,

4,
0,

4,
0,

3,
0,

13,
0,

3,
0,

4,
0,

4,
0,

8,
0,

6,
0,

8,
0,

8,
0,

2,
0,

2,
0,

2,
0,

2,
6,
0,

14,
0,

12,
0,

9,
0,

3,
0,

3,
0,

7,
0,

5,
0,

3,
0,

3,
0,

3,
0,

3,
0,
0};

struct yywork { unsigned char verify, advance; } yycrank[] ={
0,0, 0,0, 3,7, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 3,8, 3,9,
0,0, 8,9, 8,9, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 3,10,
8,9, 27,51, 0,0, 0,0,
3,11, 28,52, 0,0, 0,0,
3,12, 14,36, 0,0, 3,13,
3,14, 3,15, 3,16, 3,16,
3,16, 3,16, 3,16, 3,16,
3,16, 3,17, 6,23, 23,50,
0,0, 0,0, 0,0, 6,24,
0,0, 0,0, 3,18, 3,18,
0,0, 0,0, 3,18, 4,11,
3,19, 3,19, 35,54, 0,0,
0,0, 0,0, 4,13, 4,14,
3,19, 4,16, 4,16, 4,16,
4,16, 4,16, 4,16, 4,16,
5,22, 3,19, 55,64, 0,0,
3,20, 3,7, 3,21, 3,7,
5,22, 5,22, 0,0, 10,25,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 35,54, 10,25,
10,25, 13,35, 13,35, 13,35,
13,35, 13,35, 13,35, 13,35,
13,35, 13,35, 13,35, 0,0,
0,0, 5,22, 55,64, 4,20,
0,0, 4,21, 0,0, 0,0,
0,0, 5,23, 5,22, 0,0,
10,26, 0,0, 5,24, 5,22,
0,0, 0,0, 0,0, 0,0,
11,28, 10,25, 0,0, 5,22,
0,0, 0,0, 10,25, 0,0,
11,28, 11,28, 0,0, 0,0,
5,22, 5,22, 10,25, 0,0,
5,22, 0,0, 5,22, 5,22,
0,0, 0,0, 0,0, 10,25,
10,25, 0,0, 5,22, 10,25,
0,0, 10,25, 10,25, 0,0,
0,0, 11,28, 0,0, 5,22,
0,0, 10,25, 0,0, 5,22,
0,0, 5,22, 11,28, 0,0,
0,0, 0,0, 10,25, 11,28,
0,0, 0,0, 10,27, 0,0,
10,25, 0,0, 0,0, 11,28,
43,59, 43,59, 43,59, 43,59,
43,59, 43,59, 43,59, 43,59,
11,28, 11,28, 0,0, 0,0,
11,28, 0,0, 11,28, 11,28,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 11,28, 0,0,
29,28, 0,0, 0,0, 0,0,
0,0, 12,30, 0,0, 11,28,
0,0, 0,0, 0,0, 11,29,
0,0, 11,28, 12,31, 12,32,
12,32, 12,32, 12,32, 12,32,
12,32, 12,32, 12,33, 12,33,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 12,34,
12,34, 12,34, 12,34, 12,34,
12,34, 0,0, 29,53, 29,53,
29,53, 29,53, 29,53, 29,53,
29,53, 29,53, 0,0, 0,0,
0,0, 0,0, 15,37, 0,0,
15,38, 15,38, 15,38, 15,38,
15,38, 15,38, 15,38, 15,38,
15,39, 15,39, 0,0, 12,34,
12,34, 12,34, 12,34, 12,34,
12,34, 15,34, 15,40, 15,40,
15,34, 15,41, 15,34, 0,0,
15,42, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 15,43,
29,28, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
15,44, 61,28, 61,28, 61,28,
61,28, 61,28, 61,28, 61,28,
61,28, 15,34, 15,40, 15,40,
15,34, 15,41, 15,34, 0,0,
15,42, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 15,43,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 16,37,
15,44, 16,38, 16,38, 16,38,
16,38, 16,38, 16,38, 16,38,
16,38, 16,39, 16,39, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 16,34, 16,40,
16,40, 16,34, 16,41, 16,34,
0,0, 16,42, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
16,45, 34,34, 34,34, 34,34,
34,34, 34,34, 34,34, 34,34,
34,34, 34,34, 34,34, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 16,34, 16,40,
16,40, 16,34, 16,41, 16,34,
0,0, 16,42, 0,0, 0,0,
0,0, 0,0, 0,0, 17,37,
16,45, 17,39, 17,39, 17,39,
17,39, 17,39, 17,39, 17,39,
17,39, 17,39, 17,39, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 17,34, 17,34,
17,34, 17,34, 17,41, 17,34,
0,0, 17,42, 40,34, 40,34,
40,34, 40,34, 40,34, 40,34,
40,34, 40,34, 40,34, 40,34,
57,67, 57,67, 57,67, 57,67,
57,67, 57,67, 57,67, 57,67,
57,67, 57,67, 0,0, 0,0,
0,0, 0,0, 17,34, 17,34,
17,34, 17,34, 17,41, 17,34,
0,0, 17,42, 18,46, 18,46,
18,46, 18,46, 18,46, 18,46,
18,46, 18,46, 18,46, 18,46,
18,47, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 18,46,
18,46, 18,46, 18,46, 18,46,
18,46, 18,48, 18,49, 18,48,
18,48, 18,48, 18,48, 18,48,
18,48, 18,48, 18,48, 18,48,
18,48, 18,48, 18,48, 18,48,
18,48, 18,48, 18,48, 18,48,
18,48, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 18,46,
18,46, 18,46, 18,46, 18,46,
18,46, 18,48, 18,49, 18,48,
18,48, 18,48, 18,48, 18,48,
18,48, 18,48, 18,48, 18,48,
18,48, 18,48, 18,48, 18,48,
18,48, 18,48, 18,48, 18,48,
18,48, 19,48, 19,48, 19,48,
19,48, 19,48, 19,48, 19,48,
19,48, 19,48, 19,48, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 19,48, 19,48,
19,48, 19,48, 19,48, 19,48,
0,0, 19,48, 0,0, 0,0,
37,55, 37,55, 37,55, 37,55,
37,55, 37,55, 37,55, 37,55,
37,55, 37,55, 58,58, 58,58,
58,58, 58,58, 58,58, 58,58,
58,58, 58,58, 58,58, 58,58,
0,0, 37,56, 19,48, 19,48,
19,48, 19,48, 19,48, 19,48,
0,0, 19,48, 31,32, 31,32,
31,32, 31,32, 31,32, 31,32,
31,32, 31,32, 31,33, 31,33,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 31,34,
31,40, 31,40, 31,34, 31,34,
31,34, 37,56, 31,42, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 31,43, 53,52, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 31,44, 53,61,
53,61, 53,61, 53,61, 53,61,
53,61, 53,61, 53,61, 31,34,
31,40, 31,40, 31,34, 31,34,
31,34, 0,0, 31,42, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 31,43, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 31,44, 32,32,
32,32, 32,32, 32,32, 32,32,
32,32, 32,32, 32,32, 32,33,
32,33, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
32,34, 32,40, 32,40, 32,34,
32,34, 32,34, 0,0, 32,42,
0,0, 0,0, 41,57, 0,0,
41,57, 0,0, 32,45, 41,58,
41,58, 41,58, 41,58, 41,58,
41,58, 41,58, 41,58, 41,58,
41,58, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
32,34, 32,40, 32,40, 32,34,
32,34, 32,34, 0,0, 32,42,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 32,45, 33,33,
33,33, 33,33, 33,33, 33,33,
33,33, 33,33, 33,33, 33,33,
33,33, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
33,34, 33,34, 33,34, 33,34,
33,34, 33,34, 0,0, 33,42,
44,60, 44,60, 44,60, 44,60,
44,60, 44,60, 44,60, 44,60,
44,60, 44,60, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
0,0, 44,60, 44,60, 44,60,
44,60, 44,60, 44,60, 0,0,
33,34, 33,34, 33,34, 33,34,
33,34, 33,34, 0,0, 33,42,
0,0, 0,0, 0,0, 0,0,
0,0, 0,0, 48,48, 48,48,
48,48, 48,48, 48,48, 48,48,
48,48, 48,48, 48,48, 48,48,
0,0, 44,60, 44,60, 44,60,
44,60, 44,60, 44,60, 48,48,
48,48, 48,48, 48,48, 48,48,
48,48, 54,62, 48,48, 54,62,
0,0, 0,0, 54,63, 54,63,
54,63, 54,63, 54,63, 54,63,
54,63, 54,63, 54,63, 54,63,
62,63, 62,63, 62,63, 62,63,
62,63, 62,63, 62,63, 62,63,
62,63, 62,63, 0,0, 48,48,
48,48, 48,48, 48,48, 48,48,
48,48, 0,0, 48,48, 49,48,
49,48, 49,48, 49,48, 49,48,
49,48, 49,48, 49,48, 49,48,
49,48, 0,0, 0,0, 0,0,
0,0, 0,0, 0,0, 0,0,
49,48, 49,48, 49,48, 49,48,
49,48, 49,48, 56,65, 49,48,
56,65, 0,0, 0,0, 56,66,
56,66, 56,66, 56,66, 56,66,
56,66, 56,66, 56,66, 56,66,
56,66, 65,66, 65,66, 65,66,
65,66, 65,66, 65,66, 65,66,
65,66, 65,66, 65,66, 0,0,
49,48, 49,48, 49,48, 49,48,
49,48, 49,48, 64,68, 49,48,
64,68, 0,0, 0,0, 64,69,
64,69, 64,69, 64,69, 64,69,
64,69, 64,69, 64,69, 64,69,
64,69, 68,69, 68,69, 68,69,
68,69, 68,69, 68,69, 68,69,
68,69, 68,69, 68,69, 0,0,
0,0};
struct yysvf yysvec[] ={
0, 0, 0,
yycrank+0, 0, 0,
yycrank+0, 0, 0,
yycrank+-1, 0, 0,
yycrank+-32, yysvec+3, 0,
yycrank+-87, 0, 0,
yycrank+-16, yysvec+5, 0,
yycrank+0, 0, yyvstop+1,
yycrank+4, 0, yyvstop+3,
yycrank+0, yysvec+8, yyvstop+6,
yycrank+-98, 0, yyvstop+8,
yycrank+-139, 0, yyvstop+10,
yycrank+186, 0, yyvstop+12,
yycrank+61, 0, yyvstop+14,
yycrank+3, 0, yyvstop+16,
yycrank+224, 0, yyvstop+18,
yycrank+297, 0, yyvstop+21,
yycrank+361, 0, yyvstop+24,
yycrank+418, 0, yyvstop+27,
yycrank+493, yysvec+18, yyvstop+30,
yycrank+0, 0, yyvstop+33,
yycrank+0, 0, yyvstop+36,
yycrank+0, 0, yyvstop+39,
yycrank+12, 0, yyvstop+41,
yycrank+0, yysvec+14, yyvstop+43,
yycrank+0, yysvec+10, 0,
yycrank+0, 0, yyvstop+45,
yycrank+-3, yysvec+10, 0,
yycrank+2, 0, 0,
yycrank+210, 0, 0,
yycrank+0, yysvec+11, 0,
yycrank+550, 0, yyvstop+47,
yycrank+623, 0, yyvstop+49,
yycrank+687, 0, yyvstop+51,
yycrank+329, yysvec+33, 0,
yycrank+5, yysvec+13, yyvstop+53,
yycrank+0, 0, yyvstop+55,
yycrank+520, 0, yyvstop+57,
yycrank+0, yysvec+16, yyvstop+59,
yycrank+0, yysvec+17, yyvstop+61,
yycrank+386, yysvec+33, yyvstop+63,
yycrank+655, yysvec+33, 0,
yycrank+0, 0, yyvstop+65,
yycrank+148, 0, yyvstop+67,
yycrank+712, 0, 0,
yycrank+0, 0, yyvstop+69,
yycrank+0, yysvec+18, yyvstop+71,
yycrank+0, 0, yyvstop+73,
yycrank+750, yysvec+18, yyvstop+75,
yycrank+807, yysvec+18, yyvstop+77,
yycrank+0, 0, yyvstop+80,
yycrank+0, yysvec+10, yyvstop+82,
yycrank+0, 0, yyvstop+84,
yycrank+591, 0, 0,
yycrank+778, 0, 0,
yycrank+21, yysvec+37, yyvstop+86,
yycrank+835, 0, 0,
yycrank+396, 0, 0,
yycrank+530, yysvec+33, yyvstop+88,
yycrank+0, yysvec+43, yyvstop+90,
yycrank+0, yysvec+44, yyvstop+92,
yycrank+265, yysvec+53, 0,
yycrank+788, 0, 0,
yycrank+0, yysvec+62, yyvstop+94,
yycrank+867, 0, 0,
yycrank+845, 0, 0,
yycrank+0, yysvec+65, yyvstop+96,
yycrank+0, yysvec+57, yyvstop+98,
yycrank+877, 0, 0,
yycrank+0, yysvec+68, yyvstop+100,
0, 0, 0};
struct yywork *yytop = yycrank+934;
struct yysvf *yybgin = yysvec+1;
char yymatch[] ={
00 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,011 ,012 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
011 ,01 ,'"' ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,'+' ,01 ,'+' ,01 ,01 ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'8' ,'8' ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,'A' ,'B' ,'B' ,'A' ,'E' ,'A' ,'G' ,
'H' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'O' ,
'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,
'X' ,'G' ,'G' ,01 ,0134,01 ,'^' ,01 ,
01 ,'A' ,'B' ,'B' ,'A' ,'E' ,'A' ,'G' ,
'H' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'O' ,
'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,
'X' ,'G' ,'G' ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
01 ,01 ,01 ,01 ,01 ,01 ,01 ,01 ,
0};
char yyextra[] ={
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};


int yylineno =1;


char yytext[200];
struct yysvf *yylstate [200], **yylsp, **yyolsp;
char yysbuf[200];
char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = 10;
yylook(){
 register struct yysvf *yystate, **lsp;
 register struct yywork *yyt;
 struct yysvf *yyz;
 int yych;
 struct yywork *yyr;



 char *yylastch;




 if (!yymorfg)
  yylastch = yytext;
 else {
  yymorfg=0;
  yylastch = yytext+yyleng;
  }
 for(;;){
  lsp = yylstate;
  yyestate = yystate = yybgin;
  if (yyprevious==10) yystate++;
  for (;;){



   yyt = yystate->yystoff;
   if(yyt == yycrank){
    yyz = yystate->yyother;
    if(yyz == 0)break;
    if(yyz->yystoff == yycrank)break;
    }
   *yylastch++ = yych = (((yytchar=yysptr>yysbuf?((*--yysptr)&0377):_IO_getc (yyin))==10?(yylineno++,yytchar):yytchar)==(-1)?0:yytchar);
  tryagain:







   yyr = yyt;
   if ( (int)yyt > (int)yycrank){
    yyt = yyr + yych;
    if (yyt <= yytop && yyt->verify+yysvec == yystate){
     if(yyt->advance+yysvec == yysvec)
      {{yytchar= (*--yylastch);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;};break;}
     *lsp++ = yystate = yyt->advance+yysvec;
     goto contin;
     }
    }

   else if((int)yyt < (int)yycrank) {
    yyt = yyr = yycrank+(yycrank-yyt);



    yyt = yyt + yych;
    if(yyt <= yytop && yyt->verify+yysvec == yystate){
     if(yyt->advance+yysvec == yysvec)
      {{yytchar= (*--yylastch);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;};break;}
     *lsp++ = yystate = yyt->advance+yysvec;
     goto contin;
     }
    yyt = yyr + yymatch[yych];







    if(yyt <= yytop && yyt->verify+yysvec == yystate){
     if(yyt->advance+yysvec == yysvec)
      {{yytchar= (*--yylastch);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;};break;}
     *lsp++ = yystate = yyt->advance+yysvec;
     goto contin;
     }
    }
   if ((yystate = yystate->yyother) && (yyt= yystate->yystoff) != yycrank){



    goto tryagain;
    }

   else
    {{yytchar= (*--yylastch);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;};break;}
  contin:







   ;
   }







  while (lsp-- > yylstate){
   *yylastch-- = 0;
   if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
    yyolsp = lsp;
    if(yyextra[*yyfnd]){
     while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
      lsp--;
      {yytchar= (*yylastch--);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;};
      }
     }
    yyprevious = *yylastch;
    yylsp = lsp;
    yyleng = yylastch-yytext+1;
    yytext[yyleng] = 0;







    return(*yyfnd++);
    }
   {yytchar= (*yylastch);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;};
   }
  if (yytext[0] == 0 )
   {
   yysptr=yysbuf;
   return(0);
   }
  yyprevious = yytext[0] = (((yytchar=yysptr>yysbuf?((*--yysptr)&0377):_IO_getc (yyin))==10?(yylineno++,yytchar):yytchar)==(-1)?0:yytchar);
  if (yyprevious>0)
   _IO_putc (yyprevious, yyout);
  yylastch=yytext;



  }
 }
yyback(p, m)
 int *p;
{
if (p==0) return(0);
while (*p)
 {
 if (*p++ == m)
  return(1);
 }
return(0);
}

yyinput(){
 return((((yytchar=yysptr>yysbuf?((*--yysptr)&0377):_IO_getc (yyin))==10?(yylineno++,yytchar):yytchar)==(-1)?0:yytchar));
 }
yyoutput(c)
  int c; {
 _IO_putc (c, yyout);
 }
yyunput(c)
   int c; {
 {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;};
 }
