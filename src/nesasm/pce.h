
/* MML.C */
int mml_start(unsigned char *buffer);
int mml_stop(unsigned char *buffer);
int mml_parse(unsigned char *buffer, int bufsize, char *ptr);

/* PCX.C */
#define MAX_PCX_ARGS			8
#define NUM_PCX_PALETTE_DIMS	3
#define NUM_PCX_PALETTE_ENTRIES	256
#define MAX_PCX_NAME			127
#define PCX_HEADER_SIZE			128
#define PCX_PLANE_BUFFER_SIZE	128
#define NUM_PCX_PLANES			4
#define MAX_PCX_TILES			256
#define PCX_TILE_HASH_SIZE		256
#define PCX_TILE_HASH_MOD(v)	((v)&0xFF)
#define MAX_PCX_WIDTH			1024
#define MAX_PCX_HEIGHT			768
#define MIN_PCX_WIDTH			16
#define MIN_PCX_HEIGHT			16

extern int pcx_w, pcx_h;	/* PCX dimensions */
extern int pcx_nb_colors;	/* number of colors (16/256) in the PCX */
extern unsigned int   pcx_arg[MAX_PCX_ARGS];	/* PCX args array */
extern unsigned char *pcx_buf;	/* pointer to the PCX buffer */
extern unsigned char  pcx_pal[NUM_PCX_PALETTE_ENTRIES][NUM_PCX_PALETTE_DIMS];	/* palette */
