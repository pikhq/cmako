#ifndef CONSTANTS_H
#define CONSTANTS_H

#define OP_CONST	0
#define OP_CALL		1
#define OP_JUMP		2
#define OP_JUMPZ	3
#define OP_JUMPIF	4

#define OP_LOAD		10
#define OP_STOR		11
#define OP_RETURN	12
#define OP_DROP		13
#define OP_SWAP		14
#define OP_DUP		15
#define OP_OVER		16
#define OP_STR		17
#define OP_RTS		18

#define OP_ADD		19
#define OP_SUB		20
#define OP_MUL		21
#define OP_DIV		22
#define OP_MOD		23
#define OP_AND		24
#define OP_OR		25
#define OP_XOR		26
#define OP_NOT		27
#define OP_SGT		28
#define OP_SLT		29
#define OP_SYNC		30
#define OP_NEXT		31

#define OP_LOOKUP	32
#define OP_EXIT		33
#define OP_HCF		34

#define PC		0
#define DP		1
#define RP		2

#define GP		3
#define GT		4
#define SP		5
#define ST		6
#define SX		7
#define SY		8
#define GS		9
#define CL		10
#define RN		11
#define KY		12

#define CO		13
#define AU		14
#define KB		15

#define RESERVED_HEADER	16

#define H_MIRROR_MASK	0x10000
#define V_MIRROR_MASK	0x20000
#define GRID_Z_MASK	0x40000000

#define KEY_UP		0x01
#define KEY_RT		0x02
#define KEY_DN		0x04
#define KEY_LF		0x08
#define KEY_A		0x10
#define KEY_B		0x20
#define KEY_MASK	(KEY_UP | KEY_RT | KEY_DN | KEY_LF | KEY_A | KEY_B)

#endif
