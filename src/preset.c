#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vas.h"

void load_preset(char *preset_name);
void save_preset(char *preset_name);

extern int env_count;
extern int mod_count;
extern int part_count;
extern char *preset_dir;

extern struct envelope *env;
extern struct modulator *mod;
extern struct partial *part;

void load_preset(char *preset_name){
    // Reads the preset file in two passes; the first of which performs some
    // rudimentary error checking, and if all is good, the second pass loads it
    // into memory.

    // "goto considered harmful" mfs when they see this function:

    char buf[LINE_LEN];
    char *loc = (char*)malloc(strlen(preset_name) + LINE_LEN);
    int n;
    FILE *preset;
    char *tok;

    if(!loc){
        fprintf(stderr, "VAS: Something has gone horribly wrong.\n");
        abort();
    }

    strcpy(loc, preset_dir);
    strcat(loc, preset_name);
    strcat(loc, ".vas");

    // FIRST PASS

    preset = fopen(loc, "r");
    if(!preset){
        fprintf(stderr, "VAS: Failed loading preset \"%s\".\n", loc);
        return;
    }

    fgets(buf, LINE_LEN, preset);
    if((n = atoi(buf)) < 0) goto format_err;
    fgets(buf, LINE_LEN, preset);
    if(strcmp(buf, "ratio,gain,envelope,modulator\n"))
        goto format_err;

    for(int i = 0; i < n; i++){
        fgets(buf, LINE_LEN, preset);
        if(!(tok = strtok(buf, ","))) goto format_err;
        for(int j = 0; j < 3; j++){
            if(!(tok = strtok(NULL, ","))) goto format_err;
        }
    }

    fgets(buf, LINE_LEN, preset);
    if((n = atoi(buf)) < 0) goto format_err;
    fgets(buf, LINE_LEN, preset);
    if(strcmp(buf, "attack,decay,sustain,release\n")) goto format_err;

    for(int i = 0; i < n; i++){
        fgets(buf, LINE_LEN, preset);
        if(!(tok = strtok(buf, ","))) goto format_err;
        for(int j = 0; j < 3; j++){
            if(!(tok = strtok(NULL, ","))) goto format_err;
        }
    }

    fgets(buf, LINE_LEN, preset);
    if((n = atoi(buf)) < 0) goto format_err;
    fgets(buf, LINE_LEN, preset);
    if(strcmp(buf, "factor,time,mode\n")) goto format_err;

    for(int i = 0; i < n; i++){
        fgets(buf, LINE_LEN, preset);
        if(!(tok = strtok(buf, ","))) goto format_err;
        for(int j = 0; j < 2; j++){
            if(!(tok = strtok(NULL, ","))) goto format_err;
        }
    }

    fclose(preset);

    // SECOND PASS

    preset = fopen(loc, "r");
    if(!preset){
        fprintf(stderr, "VAS: Failed opening preset file "
            "\"%s\" for second pass.\n", loc);
        return;
    }

    // PARTIALS

    fgets(buf, LINE_LEN, preset);
    part_count = atoi(buf);

    if(part_count > 0){
        struct partial *tmp;
        tmp = (struct partial*)realloc(part, BLOCK_SIZE *
            (part_count + BLOCK_SIZE - 1) / BLOCK_SIZE *
            sizeof(struct partial));
        if(!tmp) goto alloc_err;
        part = tmp;
    }

    fgets(buf, LINE_LEN, preset); // header

    for(int i = 0; i < part_count; i++){
        fgets(buf, LINE_LEN, preset);
        part[i].ratio = atof(strtok(buf, ","));
        snprintf(part[i].ratio_s, LINE_LEN, "%f", part[i].ratio);
        snprintf(part[i].ratio_ns, LINE_LEN, "%f", part[i].ratio);
        part[i].gain = atof(strtok(NULL, ","));
        part[i].env = atoi(strtok(NULL, ","));
        snprintf(part[i].env_s, LINE_LEN, "%d", part[i].env);
        snprintf(part[i].env_ns, LINE_LEN, "%d", part[i].env);
        part[i].mod = atoi(strtok(NULL, ","));
        snprintf(part[i].mod_s, LINE_LEN, "%d", part[i].mod);
        snprintf(part[i].mod_ns, LINE_LEN, "%d", part[i].mod);
    }

    // ENVELOPES

    fgets(buf, LINE_LEN, preset);
    env_count = atoi(buf);

    if(env_count > 0){
        struct envelope *tmp;
        tmp = (struct envelope*)realloc(env, BLOCK_SIZE *
            (env_count + BLOCK_SIZE - 1) / BLOCK_SIZE *
            sizeof(struct envelope));
        if(!tmp) goto alloc_err;
        env = tmp;
    }

    fgets(buf, LINE_LEN, preset); // header

    for(int i = 0; i < env_count; i++){
        fgets(buf, LINE_LEN, preset);
        env[i].attack = atof(strtok(buf, ","));
        env[i].decay = atof(strtok(NULL, ","));
        env[i].sustain = atof(strtok(NULL, ","));
        env[i].release = atof(strtok(NULL, ","));
    }

    // MODULATORS

    fgets(buf, LINE_LEN, preset);
    mod_count = atoi(buf);

    if(mod_count > 0){
        struct modulator *tmp;
        tmp = (struct modulator*)realloc(mod, BLOCK_SIZE *
            (mod_count + BLOCK_SIZE - 1) / BLOCK_SIZE *
            sizeof(struct modulator));
        if(!tmp) goto alloc_err;
        mod = tmp;
    }

    fgets(buf, LINE_LEN, preset); // header

    for(int i = 0; i < mod_count; i++){
        fgets(buf, LINE_LEN, preset);
        mod[i].factor = atof(strtok(buf, ","));
        mod[i].time = atof(strtok(NULL, ","));
        mod[i].mode = atoi(strtok(NULL, ","));
        mod[i].soffset = (double)2.0 * mod[i].factor * mod[i].time / M_PI;
        mod[i].offset = get_mod_phase(mod[i], mod[i].time);
    }

    fclose(preset);
    free(loc);
    return;

alloc_err:
    fprintf(stderr, "VAS: Memory reallocation failed.\n");
    abort();

format_err:
    fprintf(stderr, "VAS: Preset \"%s\" is incorrectly formatted.\n", loc);
    free(loc);
    fclose(preset);
    return;
}

void save_preset(char *preset_name){
    char *loc = (char*)malloc(strlen(preset_name) + LINE_LEN);
    FILE *preset;

    if(!loc){
        fprintf(stderr, "VAS: Something has gone horribly wrong.\n");
        abort();
    }

    strcpy(loc, preset_dir);
    strcat(loc, preset_name);
    strcat(loc, ".vas");
    preset = fopen(loc, "w");

    if(!preset){
        fprintf(stderr, "VAS: Failed to open \"%s\", maybe "
            "you don't have write permissions?\n", loc);
        free(loc);
        return;
    }

    fprintf(preset, "%d\n", part_count);
    fprintf(preset, "ratio,gain,envelope,modulator\n");
    for(int i = 0; i < part_count; i++)
        fprintf(preset, "%f,%f,%d,%d\n", part[i].ratio, part[i].gain,
            part[i].env, part[i].mod);
    fprintf(preset, "%d\n", env_count);
    fprintf(preset, "attack,decay,sustain,release\n");
    for(int i = 0; i < env_count; i++)
        fprintf(preset, "%f,%f,%f,%f\n", env[i].attack, env[i].decay,
            env[i].sustain, env[i].release);
    fprintf(preset, "%d\n", mod_count);
    fprintf(preset, "factor,time,mode\n");
    for(int i = 0; i < mod_count; i++)
        fprintf(preset, "%f,%f,%d\n", mod[i].factor, mod[i].time, mod[i].mode);
    fclose(preset);
    free(loc);
    return;
}
