
struct sound_driver {
	char *id;
	char *description;
	char **help;
	int (*init)(int, int);
        void (*deinit)();
	void (*play)(void *, int);
        void (*flush)();
        void (*pause)();
        void (*resume)();
        struct list_head *next;
};
