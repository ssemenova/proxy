#ifndef __utils_h__
#define __utils_h__

#define CR "\r"
#define LF "\n"
#define CRLF CR LF

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)
#define EVEN(a)   (a % 2 == 0)
#define ODD(a)    (! EVEN(a))

#define str(s) #s
#define xstr(s) str(s)

#define die(...) do {                                        \
        fprintf(stderr, __VA_ARGS__);                        \
        exit(1);                                             \
    } while(0)

#define err(...) do {                                        \
        fprintf(stderr, __VA_ARGS__);                        \
        goto err;                                            \
    } while(0)

#define pdie(msg) do {                                       \
        perror(msg);                                         \
        exit(1);                                             \
    } while(0)

#define perr(msg) do {                                       \
        perror(msg);                                         \
        goto err;                                            \
    } while(0)

#endif /* __utils_h__ */
