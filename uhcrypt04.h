/* UHARC 0.4 table stream cipher, default password (C89, no globals). */
#ifndef UHCRYPT04_H
#define UHCRYPT04_H

typedef struct uhcrypt04 uhcrypt04_t;

/* Fresh cipher state (copies the master S-box, resets the stream
 * position). Returns 0 on allocation failure. */
uhcrypt04_t *uhcrypt04_init(void);

/* In-place symmetric transform of len bytes (encrypt == decrypt). */
void uhcrypt04_update(uhcrypt04_t *c, unsigned char *data, unsigned len);

/* Releases the state (NULL-safe). */
void uhcrypt04_free(uhcrypt04_t *c);

#endif
