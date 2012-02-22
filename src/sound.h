#include <xmp.h>

struct sound_driver {
	char *id;
	char *description;
	char **help;
	int (*init)(int *, int *);
        void (*deinit)(void);
	void (*play)(void *, int);
        void (*flush)(void);
        void (*pause)(void);
        void (*resume)(void);
        struct list_head *next;
};
