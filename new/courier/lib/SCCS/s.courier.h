h50799
s 00004/00000/00063
d D 1.2 83/02/23 14:04:39 cooper 2 1
c added LongUnspecified as predefined type
e
s 00063/00000/00000
d D 1.1 83/02/23 13:58:00 cooper 1 0
c date and time created 83/02/23 13:58:00 by cooper
e
u
U
t
T
I 1
/*
 * %M% %I% %G%
 */
#include <stdio.h>

#if DEBUG
extern int CourierDebuggingFlag;
#endif

/*
 * Predefined Courier types.
 */
typedef char		Boolean;
typedef unsigned short	Cardinal;
typedef unsigned long	LongCardinal;
typedef short		Integer;
typedef long		LongInteger;
typedef char		*String;
typedef unsigned short	Unspecified;
I 2
typedef unsigned long	LongUnspecified;
E 2

#define MoveByte(a, b, flag)	(flag ? *(char *)(b) = *(char *)(a), 1 : 1)

/*
 * Low-level byte moving, with byte-swapping.
 * Use these definitions for VAX and other low-enders.
 */
#if vax
#define MoveShort(a, b, flag)	(flag ? *(char *)(b) = *((char *)(a)+1), *((char *)(b)+1) = *(char *)(a), 1 : 1)
#define MoveLong(a, b, flag)	(flag ? *(char *)(b) = *((char *)(a)+3), *((char *)(b)+1) = *((char *)(a)+2), *((char *)(b)+2) = *((char *)(a)+1), *((char *)(b)+3) = *(char *)(a), 2 : 2)
#endif

/*
 * Low-level byte moving, without byte-swapping.
 * Use these definitions for SUN and other high-enders.
 */
#if sun
#define MoveShort(a, b, flag)	(flag ? *(short *)(b) = *(short *)(a), 1 : 1)
#define MoveLong(a, b, flag)	(flag ? *(long *)(b) = *(long *)(a), 2 : 2)
#endif

#define PackBoolean(p, buf, flag)		MoveByte(p, buf, flag)
#define UnpackBoolean(p, buf)			MoveByte(buf, p, 1)

#define PackCardinal(p, buf, flag)		MoveShort(p, buf, flag)
#define UnpackCardinal(p, buf)			MoveShort(buf, p, 1)

#define PackLongCardinal(p, buf, flag)		MoveLong(p, buf, flag)
#define UnpackLongCardinal(p, buf)		MoveLong(buf, p, 1)

#define PackInteger(p, buf, flag)		MoveShort(p, buf, flag)
#define UnpackInteger(p, buf)			MoveShort(buf, p, 1)

#define PackLongInteger(p, buf, flag)		MoveLong(p, buf, flag)
#define UnpackLongInteger(p, buf)		MoveLong(buf, p, 1)

#define PackUnspecified(p, buf, flag)		MoveShort(p, buf, flag)
#define UnpackUnspecified(p, buf)		MoveShort(buf, p, 1)
I 2

#define PackLongUnspecified(p, buf, flag)	MoveLong(p, buf, flag)
#define UnpackLongUnspecified(p, buf)		MoveLong(buf, p, 1)
E 2

/*
 * External declarations.
 */
extern Unspecified *Allocate();
extern Unspecified *ReceiveCallMessage(), *ReceiveReturnMessage();
E 1
