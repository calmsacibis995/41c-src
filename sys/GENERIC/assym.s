#ifdef LOCORE
#define	P_LINK 0
#define	P_RLINK 4
#define	P_ADDR 8
#define	P_PRI 13
#define	P_STAT 15
#define	P_WCHAN 84
#define	SSLEEP 1
#define	SRUN 3
#define	UBA_BRRVR 48
#define	UH_UBA 0
#define	UH_VEC 8
#define	UH_SIZE 52
#define	RP_FLAG 12
#define	V_SWTCH 0
#define	V_TRAP 4
#define	V_SYSCALL 8
#define	V_INTR 12
#define	V_PDMA 16
#define	UPAGES 8
#define	CLSIZE 2
#define	SYSPTSIZE 4480
#define	USRPTSIZE 1024
#define	MSGBUFPTECNT 8
#define	NMBCLUSTERS 256
#else
asm(".set	U_ARG,384");
asm(".set	U_QSAVE,148701184");
#endif
