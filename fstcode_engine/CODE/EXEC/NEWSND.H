/* FST sounds */

extern int engine_noise_on, noise_on;

extern void setup_sound(void);
extern void exit_sound(void);
extern void stop_sounds(void);
extern void update_sounds(void);

extern void cannon_noise(void);
extern void fire_noise(int length);
extern void gear_sound(void);
extern void explo1_sound(void);
extern void explo2_sound(void);
extern void explo3_sound(void);
extern void landing_sound(void);
extern void maw_sound(void);
extern void ping_sound(void);
extern void clunk_sound(void);
extern void swlock_sound(void);
extern void near_bullet_sound(void);
extern void eject_sound(void);
extern void flare_sound(void);
extern void chaff_sound(void);
extern void klaxon_sound(void);
extern void die_sound(void);
extern void start_sound(void);
extern void lowwarn_sound(void);
extern void glidewarn_sound(void);
extern void switch_sound(void);

extern void play_sound_sample(int n);
