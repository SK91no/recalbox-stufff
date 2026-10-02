typedef struct {
    int hfreq;
    float vfreq;
    unsigned int width;
    unsigned int height;
    int interlaced;
} video_mode;

video_mode get_current_mode();