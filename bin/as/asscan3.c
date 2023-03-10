/*
 *	Copyright (c) 1982 Regents of the University of California
 */
#ifndef lint
static char sccsid[] = "@(#)asscan3.c 4.3 2/14/82";
#endif not lint

#include "asscanl.h"

readonly short type[] = {

/*fill up the input buffer*/	NEEDSBUF,	
/*hit the hard end of file*/	SCANEOF,	
	/* '^@' 0x00 0000 */	SP,
	/* '^A' 0x01 0001 */	BADCHAR,
	/* '^B' 0x02 0002 */	BADCHAR,
	/* '^C' 0x03 0003 */	BADCHAR,
	/* '^D' 0x04 0004 */	BADCHAR,
	/* '^E' 0x05 0005 */	BADCHAR,
	/* '^F' 0x06 0006 */	BADCHAR,
	/* '^G' 0x07 0007 */	BADCHAR,
	/* '\b' 0x08 0010 */	BADCHAR,
	/* '\t' 0x09 0011 */	SP,
	/* '\n' 0x0a 0012 */	NL,
	/* '^K' 0x0b 0013 */	BADCHAR,
	/* '\f' 0x0c 0014 */	BADCHAR,
	/* '\r' 0x0d 0015 */	SP,
	/* '^N' 0x0e 0016 */	BADCHAR,
	/* '^O' 0x0f 0017 */	BADCHAR,
	/* '^P' 0x10 0020 */	BADCHAR,
	/* '^Q' 0x11 0021 */	BADCHAR,
	/* '^R' 0x12 0022 */	BADCHAR,
	/* '^S' 0x13 0023 */	BADCHAR,
	/* '^T' 0x14 0024 */	BADCHAR,
	/* '^U' 0x15 0025 */	BADCHAR,
	/* '^V' 0x16 0026 */	BADCHAR,
	/* '^W' 0x17 0027 */	BADCHAR,
	/* '^X' 0x18 0030 */	BADCHAR,
	/* '^Y' 0x19 0031 */	BADCHAR,
	/* '^Z' 0x1a 0032 */	BADCHAR,
	/* '^[' 0x1b 0033 */	BADCHAR,
	/* '^\' 0x1c 0034 */	BADCHAR,
	/* '^]' 0x1d 0035 */	BADCHAR,
	/* '^^' 0x1e 0036 */	BADCHAR,
	/* '^_' 0x1f 0037 */	BADCHAR,
	/* ' ' 0x20 0040 */	SP,
	/* '!' 0x21 0041 */	ORNOT,
	/* '"' 0x22 0042 */	DQ,
	/* '#' 0x23 0043 */	SH,
	/* '$' 0x24 0044 */	LITOP,
	/* '%' 0x25 0045 */	REGOP,
	/* '&' 0x26 0046 */	AND,
	/* ''' 0x27 0047 */	SQ,
	/* '(' 0x28 0050 */	LP,
	/* ')' 0x29 0051 */	RP,
	/* '*' 0x2a 0052 */	MUL,
	/* '+' 0x2b 0053 */	PLUS,
	/* ',' 0x2c 0054 */	CM,
	/* '-' 0x2d 0055 */	MINUS,
	/* '.' 0x2e 0056 */	ALPH,
	/* '/' 0x2f 0057 */	DIV,
	/* '0' 0x30 0060 */	DIG,
	/* '1' 0x31 0061 */	DIG,
	/* '2' 0x32 0062 */	DIG,
	/* '3' 0x33 0063 */	DIG,
	/* '4' 0x34 0064 */	DIG,
	/* '5' 0x35 0065 */	DIG,
	/* '6' 0x36 0066 */	DIG,
	/* '7' 0x37 0067 */	DIG,
	/* '8' 0x38 0070 */	DIG,
	/* '9' 0x39 0071 */	DIG,
	/* ':' 0x3a 0072 */	COLON,
	/* ';' 0x3b 0073 */	SEMI,
	/* '<' 0x3c 0074 */	LSH,
	/* '=' 0x3d 0075 */	BADCHAR,
	/* '>' 0x3e 0076 */	RSH,
	/* '?' 0x3f 0077 */	BADCHAR,
	/* '@' 0x40 0100 */	BADCHAR,
	/* 'A' 0x41 0101 */	ALPH,
	/* 'B' 0x42 0102 */	ALPH,
	/* 'C' 0x43 0103 */	ALPH,
	/* 'D' 0x44 0104 */	ALPH,
	/* 'E' 0x45 0105 */	ALPH,
	/* 'F' 0x46 0106 */	ALPH,
	/* 'G' 0x47 0107 */	ALPH,
	/* 'H' 0x48 0110 */	ALPH,
	/* 'I' 0x49 0111 */	ALPH,
	/* 'J' 0x4a 0112 */	ALPH,
	/* 'K' 0x4b 0113 */	ALPH,
	/* 'L' 0x4c 0114 */	ALPH,
	/* 'M' 0x4d 0115 */	ALPH,
	/* 'N' 0x4e 0116 */	ALPH,
	/* 'O' 0x4f 0117 */	ALPH,
	/* 'P' 0x50 0120 */	ALPH,
	/* 'Q' 0x51 0121 */	ALPH,
	/* 'R' 0x52 0122 */	ALPH,
	/* 'S' 0x53 0123 */	ALPH,
	/* 'T' 0x54 0124 */	ALPH,
	/* 'U' 0x55 0125 */	ALPH,
	/* 'V' 0x56 0126 */	ALPH,
	/* 'W' 0x57 0127 */	ALPH,
	/* 'X' 0x58 0130 */	ALPH,
	/* 'Y' 0x59 0131 */	ALPH,
	/* 'Z' 0x5a 0132 */	ALPH,
	/* '[' 0x5b 0133 */	LB,
	/* '\\' 0x5c 0134 */	BADCHAR,
	/* ']' 0x5d 0135 */	RB,
	/* '^' 0x5e 0136 */	XOR,
	/* '_' 0x5f 0137 */	ALPH,
	/* '`' 0x60 0140 */	SIZEQUOTE,
	/* 'a' 0x61 0141 */	ALPH,
	/* 'b' 0x62 0142 */	ALPH,
	/* 'c' 0x63 0143 */	ALPH,
	/* 'd' 0x64 0144 */	ALPH,
	/* 'e' 0x65 0145 */	ALPH,
	/* 'f' 0x66 0146 */	ALPH,
	/* 'g' 0x67 0147 */	ALPH,
	/* 'h' 0x68 0150 */	ALPH,
	/* 'i' 0x69 0151 */	ALPH,
	/* 'j' 0x6a 0152 */	ALPH,
	/* 'k' 0x6b 0153 */	ALPH,
	/* 'l' 0x6c 0154 */	ALPH,
	/* 'm' 0x6d 0155 */	ALPH,
	/* 'n' 0x6e 0156 */	ALPH,
	/* 'o' 0x6f 0157 */	ALPH,
	/* 'p' 0x70 0160 */	ALPH,
	/* 'q' 0x71 0161 */	ALPH,
	/* 'r' 0x72 0162 */	ALPH,
	/* 's' 0x73 0163 */	ALPH,
	/* 't' 0x74 0164 */	ALPH,
	/* 'u' 0x75 0165 */	ALPH,
	/* 'v' 0x76 0166 */	ALPH,
	/* 'w' 0x77 0167 */	ALPH,
	/* 'x' 0x78 0170 */	ALPH,
	/* 'y' 0x79 0171 */	ALPH,
	/* 'z' 0x7a 0172 */	ALPH,
	/* '{' 0x7b 0173 */	BADCHAR,
	/* '|' 0x7c 0174 */	IOR,
	/* '}' 0x7d 0175 */	BADCHAR,
	/* '~' 0x7e 0176 */	TILDE,
	/* '^[' 0x7f 0177 */	BADCHAR,
0
};

readonly short charsets[] = {
	/* '^@' 0x00 0000 */	0,
	/* '^A' 0x01 0001 */	0,
	/* '^B' 0x02 0002 */	0,
	/* '^C' 0x03 0003 */	0,
	/* '^D' 0x04 0004 */	0,
	/* '^E' 0x05 0005 */	0,
	/* '^F' 0x06 0006 */	0,
	/* '^G' 0x07 0007 */	0,
	/* '\b' 0x08 0010 */	0,
	/* '\t' 0x09 0011 */	SPACE,
	/* '\n' 0x0a 0012 */	STRESCAPE,
	/* '^K' 0x0b 0013 */	0,
	/* '\f' 0x0c 0014 */	0,
	/* '\r' 0x0d 0015 */	0,
	/* '^N' 0x0e 0016 */	0,
	/* '^O' 0x0f 0017 */	0,
	/* '^P' 0x10 0020 */	0,
	/* '^Q' 0x11 0021 */	0,
	/* '^R' 0x12 0022 */	0,
	/* '^S' 0x13 0023 */	0,
	/* '^T' 0x14 0024 */	0,
	/* '^U' 0x15 0025 */	0,
	/* '^V' 0x16 0026 */	0,
	/* '^W' 0x17 0027 */	0,
	/* '^X' 0x18 0030 */	0,
	/* '^Y' 0x19 0031 */	0,
	/* '^Z' 0x1a 0032 */	0,
	/* '^[' 0x1b 0033 */	0,
	/* '^\' 0x1c 0034 */	0,
	/* '^]' 0x1d 0035 */	0,
	/* '^^' 0x1e 0036 */	0,
	/* '^_' 0x1f 0037 */	0,
	/* ' ' 0x20 0040 */	SPACE,
	/* '!' 0x21 0041 */	0,
	/* '"' 0x22 0042 */	STRESCAPE,
	/* '#' 0x23 0043 */	0,
	/* '$' 0x24 0044 */	ALPHA,
	/* '%' 0x25 0045 */	0,
	/* '&' 0x26 0046 */	0,
	/* ''' 0x27 0047 */	0,
	/* '(' 0x28 0050 */	0,
	/* ')' 0x29 0051 */	0,
	/* '*' 0x2a 0052 */	0,
	/* '+' 0x2b 0053 */	SIGN,
	/* ',' 0x2c 0054 */	0,
	/* '-' 0x2d 0055 */	SIGN,
	/* '.' 0x2e 0056 */	POINT+ALPHA,
	/* '/' 0x2f 0057 */	0,
	/* '0' 0x30 0060 */	DIGIT+REGDIGIT+OCTDIGIT,
	/* '1' 0x31 0061 */	DIGIT+REGDIGIT+OCTDIGIT,
	/* '2' 0x32 0062 */	DIGIT+REGDIGIT+OCTDIGIT,
	/* '3' 0x33 0063 */	DIGIT+REGDIGIT+OCTDIGIT,
	/* '4' 0x34 0064 */	DIGIT+REGDIGIT+OCTDIGIT,
	/* '5' 0x35 0065 */	DIGIT+REGDIGIT+OCTDIGIT,
	/* '6' 0x36 0066 */	DIGIT+OCTDIGIT,
	/* '7' 0x37 0067 */	DIGIT+OCTDIGIT,
	/* '8' 0x38 0070 */	DIGIT,
	/* '9' 0x39 0071 */	DIGIT,
	/* ':' 0x3a 0072 */	0,
	/* ';' 0x3b 0073 */	0,
	/* '<' 0x3c 0074 */	0,
	/* '=' 0x3d 0075 */	0,
	/* '>' 0x3e 0076 */	0,
	/* '?' 0x3f 0077 */	0,
	/* '@' 0x40 0100 */	0,
	/* 'A' 0x41 0101 */	ALPHA+HEXUDIGIT,
	/* 'B' 0x42 0102 */	ALPHA+HEXUDIGIT+SZSPECBEGIN,
	/* 'C' 0x43 0103 */	ALPHA+HEXUDIGIT,
	/* 'D' 0x44 0104 */	ALPHA+HEXUDIGIT+FLOATEXP+FLOATFLAG,
	/* 'E' 0x45 0105 */	ALPHA+HEXUDIGIT+FLOATEXP,
	/* 'F' 0x46 0106 */	ALPHA+HEXUDIGIT+FLOATFLAG+FLOATEXP,
	/* 'G' 0x47 0107 */	ALPHA+FLOATFLAG+FLOATEXP,
	/* 'H' 0x48 0110 */	ALPHA+FLOATFLAG+FLOATEXP,
	/* 'I' 0x49 0111 */	ALPHA,
	/* 'J' 0x4a 0112 */	ALPHA,
	/* 'K' 0x4b 0113 */	ALPHA,
	/* 'L' 0x4c 0114 */	ALPHA+SZSPECBEGIN,
	/* 'M' 0x4d 0115 */	ALPHA,
	/* 'N' 0x4e 0116 */	ALPHA,
	/* 'O' 0x4f 0117 */	ALPHA,
	/* 'P' 0x50 0120 */	ALPHA,
	/* 'Q' 0x51 0121 */	ALPHA,
	/* 'R' 0x52 0122 */	ALPHA,
	/* 'S' 0x53 0123 */	ALPHA,
	/* 'T' 0x54 0124 */	ALPHA,
	/* 'U' 0x55 0125 */	ALPHA,
	/* 'V' 0x56 0126 */	ALPHA,
	/* 'W' 0x57 0127 */	ALPHA+SZSPECBEGIN,
	/* 'X' 0x58 0130 */	ALPHA+HEXFLAG,
	/* 'Y' 0x59 0131 */	ALPHA,
	/* 'Z' 0x5a 0132 */	ALPHA,
	/* '[' 0x5b 0133 */	0,
	/* '\\' 0x5c 0134 */	STRESCAPE,
	/* ']' 0x5d 0135 */	0,
	/* '^' 0x5e 0136 */	0,
	/* '_' 0x5f 0137 */	ALPHA,
	/* '`' 0x60 0140 */	0,
	/* 'a' 0x61 0141 */	ALPHA+HEXLDIGIT,
	/* 'b' 0x62 0142 */	ALPHA+HEXLDIGIT+BSESCAPE+SZSPECBEGIN,
	/* 'c' 0x63 0143 */	ALPHA+HEXLDIGIT,
	/* 'd' 0x64 0144 */	ALPHA+HEXLDIGIT+FLOATEXP+FLOATFLAG,
	/* 'e' 0x65 0145 */	ALPHA+HEXLDIGIT+FLOATEXP,
	/* 'f' 0x66 0146 */	ALPHA+HEXLDIGIT+BSESCAPE+FLOATEXP+FLOATFLAG,
	/* 'g' 0x67 0147 */	ALPHA+FLOATEXP+FLOATFLAG,
	/* 'h' 0x68 0150 */	ALPHA+FLOATEXP+FLOATFLAG,
	/* 'i' 0x69 0151 */	ALPHA,
	/* 'j' 0x6a 0152 */	ALPHA,
	/* 'k' 0x6b 0153 */	ALPHA,
	/* 'l' 0x6c 0154 */	ALPHA+SZSPECBEGIN,
	/* 'm' 0x6d 0155 */	ALPHA,
	/* 'n' 0x6e 0156 */	ALPHA+BSESCAPE,
	/* 'o' 0x6f 0157 */	ALPHA,
	/* 'p' 0x70 0160 */	ALPHA,
	/* 'q' 0x71 0161 */	ALPHA,
	/* 'r' 0x72 0162 */	ALPHA+BSESCAPE,
	/* 's' 0x73 0163 */	ALPHA,
	/* 't' 0x74 0164 */	ALPHA+BSESCAPE,
	/* 'u' 0x75 0165 */	ALPHA,
	/* 'v' 0x76 0166 */	ALPHA,
	/* 'w' 0x77 0167 */	ALPHA+SZSPECBEGIN,
	/* 'x' 0x78 0170 */	ALPHA+HEXFLAG,
	/* 'y' 0x79 0171 */	ALPHA,
	/* 'z' 0x7a 0172 */	ALPHA,
	/* '{' 0x7b 0173 */	0,
	/* '|' 0x7c 0174 */	0,
	/* '}' 0x7d 0175 */	0,
	/* '~' 0x7e 0176 */	0,
	/* '^[' 0x7f 0177 */	0,
0
};
