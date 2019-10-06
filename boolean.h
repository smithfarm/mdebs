/*
 * Borrowed from /usr/include/postgresql/c.h
 */

/*
 * bool
 *		Boolean value, either true or false.
 *
 */
#ifndef __cplusplus
#ifndef bool
typedef char bool;

#endif	 /* ndef bool */
#endif	 /* not C++ */
#ifndef true
#define true	((bool) 1)
#endif
#ifndef false
#define false	((bool) 0)
#endif
typedef bool *BoolPtr;

#ifndef TRUE
#define TRUE	1
#endif	 /* TRUE */

#ifndef FALSE
#define FALSE	0
#endif	 /* FALSE */

