#ifndef VAS_H
#define VAS_H

#include <stdbool.h>

#define BLOCK_SIZE 8
#define LINE_LEN 80
#define MAX_POLY 16

#define WIN_HEIGHT 1000
#define WIN_WIDTH 800

#define VAS_MOD_REPEAT 1
#define VAS_MOD_SIN 2

struct partial{
    float ratio;
    char ratio_s[LINE_LEN];
    char ratio_ns[LINE_LEN];
    float gain;
    int env;
    char env_s[LINE_LEN];
    char env_ns[LINE_LEN];
    int mod;
    char mod_s[LINE_LEN];
    char mod_ns[LINE_LEN];
};

struct envelope{
    float attack;
    float decay;
    float sustain;
    float release;
    float release_gain[MAX_POLY];
};

struct modulator{
    float factor;
    float time;
    int mode;
    double offset;
    double soffset;
};

double get_mod_phase(struct modulator m, double t);
void gui_cleanup();
void load_preset(char *preset_name);
void save_preset(char *preset_name);
void vas_gui_start();
void vas_exit(int sig);

#endif
