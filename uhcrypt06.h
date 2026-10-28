/* UHARC 0.6 table stream cipher, default password (C89, no globals). */
#ifndef UHCRYPT06_H
#define UHCRYPT06_H

typedef struct uhcrypt06 uhcrypt06_t;

/* Fresh cipher state (copies the master S-box, resets the stream
 * position). Returns 0 on allocation failure. */
uhcrypt06_t *uhcrypt06_init(void);

/* Same with an explicit password (e.g. from -pw). A NULL or empty
 * password falls back to the default password. */
uhcrypt06_t *uhcrypt06_init_pw(const char *pwd);

/* In-place symmetric transform of len bytes (encrypt == decrypt). */
void uhcrypt06_update(uhcrypt06_t *c, unsigned char *data, unsigned len);

/* Releases the state (NULL-safe). */
void uhcrypt06_free(uhcrypt06_t *c);

#endif
