/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * blowfish.h - encryption and decryption, public domain
 *
 * ------------------------------------------------------------------------- */
/* 28 April 97 */

#ifndef BLOWFISH_H
#define BLOWFISH_H

#define MAXKEYBYTES 56	/* 448 bits */
#define bf_N 16
#define noErr 0
#define DATAERROR -1
#define KEYBYTES 8

#define UBYTE_08bits  u_char
#define UWORD_16bits  u_short

#if SIZEOF_INT==4
#  define UWORD_32bits  u_int
#else
#  if SIZEOF_LONG==4
#  define UWORD_32bits  u_long
#  endif
#endif

/* choose a byte order for your hardware */
#ifdef WORDS_BIGENDIAN
/* ABCD - big endian - motorola */
union aword {
	UWORD_32bits word;
	UBYTE_08bits byte [4];
	struct {
		u_int byte0:8;
		u_int byte1:8;
		u_int byte2:8;
		u_int byte3:8;
	} w;
};
#endif  /* WORDS_BIGENDIAN */

#ifndef WORDS_BIGENDIAN
/* DCBA - little endian - intel */
union aword {
	UWORD_32bits word;
	UBYTE_08bits byte [4];
	struct {
		u_int byte3:8;
		u_int byte2:8;
		u_int byte1:8;
		u_int byte0:8;
	} w;
};
#endif  /* !WORDS_BIGENDIAN */

/* Prototypes */
void init_blowfish();
u_long blowfish_sizeof();
void blowfish_encipher(UWORD_32bits *xl, UWORD_32bits *xr);
void blowfish_decipher(UWORD_32bits *xl, UWORD_32bits *xr);
void debug_blowfish();
void blowfish_init(UBYTE_08bits *key, char keybytes);

int base64dec(char c);
void encrypt_pass(char *text, char *new);
char *encrypt_string(char *key, char *str);
char *decrypt_string(char *key, char *str);

#endif /* BLOWFISH_H */
