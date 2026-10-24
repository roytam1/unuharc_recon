/* UHARC 0.6 table stream cipher, default password (C89, no globals). */
#ifndef UHCRYPT06_H
#define UHCRYPT06_H

typedef struct uhcrypt06 uhcrypt06_t;

/* Fresh cipher state (copies the master S-box, resets the stream
 * position). Returns 0 on allocation failure. */
uhcrypt06_t *uhcrypt06_init(void);

/* In-place symmetric transform of len bytes (encrypt == decrypt). */
void uhcrypt06_update(uhcrypt06_t *c, unsigned char *data, unsigned len);

/* Releases the state (NULL-safe). */
void uhcrypt06_free(uhcrypt06_t *c);

#endif
