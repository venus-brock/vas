#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "vas.h"

float a4;
int env_count;
int midi_channel;
int mod_count;
int part_count;
char *preset_dir;

struct envelope *env;
struct modulator *mod;
struct partial *part;

double get_mod_phase(struct modulator m, double t);
void vas_exit(int sig);

struct note{
    double delta;
    bool held;
    int id;
    double rtime;
    double time;
};

static struct note notes[MAX_POLY];

static double delta_time;
static jack_client_t *client;
static bool gui_started = false;
static jack_port_t *midi_in;
static jack_port_t *out_port_l, *out_port_r;
static uint32_t rate;

static double get_env_gain(struct envelope e, int n);
static int process(jack_nframes_t num_frames, void *arg);
static void vas_init(void);

int main(void){
    vas_init();
    
    gui_started = true;
    vas_gui_start(); // never returns
}

int process(jack_nframes_t num_frames, void *arg){
    jack_default_audio_sample_t *out_l, *out_r;
    void *port_buf = jack_port_get_buffer(midi_in, num_frames);
    jack_midi_event_t ev;
    jack_nframes_t evdex = 0;
    jack_nframes_t numev = jack_midi_get_event_count(port_buf);

    out_l = (jack_default_audio_sample_t*)jack_port_get_buffer(out_port_l,
        num_frames);
    out_r = (jack_default_audio_sample_t*)jack_port_get_buffer(out_port_r,
        num_frames);

    if(numev) jack_midi_event_get(&ev, port_buf, evdex);

    for(jack_nframes_t frame = 0; frame < num_frames; frame++){
        if((ev.time <= frame) && (evdex < numev)){
            if(ev.buffer[0] == 0x90 + midi_channel){
                for(int i = 0; i < MAX_POLY; i++){
                    if(notes[i].id != -1) continue;
                    notes[i].id = ev.buffer[1];
                    notes[i].delta = pow(2.0, ((double)notes[i].id - 69) / 12);
                    notes[i].delta *= (double)2.0 * M_PI * a4 / rate;
                    notes[i].held = true;
                    notes[i].time = 0.0;
                    break;
                }
            }
            if(ev.buffer[0] == 0x80 + midi_channel){
                for(int i = 0; i < MAX_POLY; i++){
                    if(!(notes[i].held && (notes[i].id == ev.buffer[1])))
                        continue;
                    for(int j = 0; j < env_count; j++)
                        env[j].release_gain[i] = get_env_gain(env[j], i);
                    notes[i].held = false;
                    notes[i].rtime = 0.0;
                    break;
                }
            }
            evdex++;
            if(evdex < numev)
                jack_midi_event_get(&ev, port_buf, evdex);
        }
        double sample = 0.0;
        for(int i = 0; i < MAX_POLY; i++){
            if(notes[i].id == -1) continue;
            bool off = !notes[i].held;
            for(int j = 0; j < part_count; j++){
                if(part[j].gain == 0.0) continue;
                double gain = part[j].gain;
                double phase = notes[i].delta * part[j].ratio / delta_time;
                if(part[j].env > 0 && part[j].env <= env_count){
                    gain *= get_env_gain(env[part[j].env - 1], i);
                    if(notes[i].rtime < env[part[j].env - 1].release)
                        off = false;
                }
                if(part[j].mod > 0 && part[j].mod <= mod_count)
                    phase *= get_mod_phase(mod[part[j].mod - 1],
                        notes[i].time);
                else phase *= notes[i].time;
                sample += sin(phase) * gain;
            }
            notes[i].time += delta_time;
            notes[i].rtime += delta_time;
            if(off) notes[i].id = -1;
        }
        out_r[frame] = out_l[frame] = sample * 0.05;
    }
    return 0;
}

double get_env_gain(struct envelope e, int n){
    double result;
    double t = notes[n].time;
    if(notes[n].held){
        if(t <= e.attack && e.attack != 0.0) result = t / e.attack;
        else if((t -= e.attack) <= e.decay && e.decay != 0.0)
            result = (e.decay - t * (1 - e.sustain)) / e.decay;
        else result = e.sustain;
    } else
        result = e.release_gain[n] * (1 - notes[n].rtime / e.release);
    return result;
}

double get_mod_phase(struct modulator m, double t){
    int dir;
    double offset = m.offset;
    double result;
    if(t <= m.time) offset = 0.0;
    if(m.mode & VAS_MOD_REPEAT && t > m.time){
        dir = (int)(t / m.time) % 2;
        offset *= (int)(t / m.time);
        t = fmod(t, m.time);
    }
    if(t <= m.time && m.time != 0.0){
        if(m.mode & VAS_MOD_SIN){
            result = -2 * m.factor * m.time *
                cos(t * M_PI / ((double)2.0 * m.time)) / M_PI - m.factor * t +
                t + m.soffset + offset;
        }
        else{
            result = pow(t, 2.0) * m.factor / (2.0 * m.time);
            if(dir)
                result = result + t - t * m.factor + offset;
            else
                result = -result + t + offset;
        }
    } else result = t - m.time + offset;
    return result;
}

void vas_exit(int sig){
    if(gui_started) gui_cleanup();
    jack_client_close(client);
    fprintf(stderr, "\nVAS: Exiting...\n");
    exit(sig);
}

void vas_init(void){
    char *client_name = "VAS";

    client = jack_client_open(client_name, JackNullOption, NULL, NULL);
    if(!client){
        fprintf(stderr, "VAS: Failed opening JACK client.\n");
        exit(1);
    }

    a4 = 440.0;
    rate = jack_get_sample_rate(client);
    delta_time = 1.0 / (double)rate;
    env_count = 0;
    mod_count = 0;
    part_count = 0;
    midi_channel = 0;

    env = (struct envelope*)malloc(BLOCK_SIZE * sizeof(struct envelope));
    mod = (struct modulator*)malloc(BLOCK_SIZE * sizeof(struct modulator));
    part = (struct partial*)malloc(BLOCK_SIZE * sizeof(struct partial));
    if(!(env && mod && part)){
        fprintf(stderr, "VAS: Memory allocation failed.\n");
        abort();
    }
    memset(env, 0, BLOCK_SIZE * sizeof(struct envelope));
    memset(mod, 0, BLOCK_SIZE * sizeof(struct modulator));
    memset(part, 0, BLOCK_SIZE * sizeof(struct partial));

    for(int i = 0; i < MAX_POLY; i++) notes[i].id = -1;

    preset_dir = getenv("HOME");
    if(!preset_dir)
        fprintf(stderr, "VAS: HOME variable unset; unable to load presets.\n");
    else{
        strcat(preset_dir, "/.config/vas/presets/");
        load_preset("vas_default");
    }

    jack_set_process_callback(client, process, NULL);

    midi_in = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE,
        JackPortIsInput, 0);
    out_port_l = jack_port_register(client, "out_left",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    out_port_r = jack_port_register(client, "out_right",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if(jack_activate(client)){
        fprintf(stderr, "VAS: Failed activating JACK client.\n");
        vas_exit(1);
    }

    signal(SIGQUIT, vas_exit);
    signal(SIGTERM, vas_exit);
    signal(SIGHUP, vas_exit);
    signal(SIGINT, vas_exit);
    return;
}
