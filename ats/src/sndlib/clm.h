#ifndef CLM_H
#define CLM_H

#define MUS_VERSION 2
#define MUS_REVISION 31
#define MUS_DATE "27-June-03"

/* 
 * 27-June:    added mus_samples_to_seconds and mus_seconds_to_samples.
 * 9-June:     added mus_mix_with_reader_and_writer.
 * 27-May:     bugfix: interpolating all-pass ("zall-pass") had an extra delay.
 * 25-Apr:     mus_spectrum and mus_convolution now return Float*.
 * 9-Apr:      removed MUS_HANNING_WINDOW (use MUS_HANN_WINDOW).
 * 3-Mar:      mus_delay_line_p for tap error checking.
 * 27-Feb:     mus_length for env -> original duration in samples.
 * 21-Feb:     mus_set_cosines added, mus_cosines moved to hop slot.
 *             mus_[set_]x1/x2/y1/y2.
 * 10-Feb:     mus_file_name moved into the mus_input|output structs.
 *             folded mus_input|output into mus_any.
 *             moved mus_frame|mixer declarations into clm.c.
 *             all mus_input|output|frame|mixer pointers->mus_any.
 *             all method void pointers->mus_any.
 * 7-Feb:      split strings out of clm2xen.c into clm-strings.h.
 * 3-Feb:      mus_offset for envs, mus_width for square_wave et al.
 *             new core class fields(10) for various methods.
 * 7-Jan-03:   mus_src with very large sr_change segfault bugfix.
 * --------
 * 17-Dec:     mus_env_offset|initial_power for Snd exp env optimizations.
 * 13-Sep:     mus_frandom and mus_irandom(for Snd optimizer).
 * 19-Aug:     changed internal phase-vocoder array accessor names
 * 13-Aug:     added set!(*-ref) for frame, locsig, mixer, locsig-reverb.
 * 29-Jul:     added various *_1 cases for the optimizer.
 * 15-Jul:     mus_continue_sample2file.
 * 10-Jul:     mus_file_name.
 * 7-Jun:      fftw support added(mus_fftw).
 * 31-May:     changed mus_any_class.
 * 3-May:      many int->off_t changes for large files.
 * 8-Apr:      off-by-1 env bug(Lisp/C are now identical), env_interp of exp env beyond end bugfix.
 * 1-Apr:      sine-summation n=0 bugfix.
 * 27-Mar:     negative degree locsig bugfix.
 * 18-Mar:     mus_move_locsig.
 * 15-Mar:     n-chan locsig(and reverb scalers), 'type' arg to mus_make_locsig.
 * 6-Mar:      mus_scaler in asymmetric-fm now refers to the "r" parameter, "a" in sine-summation.
 * 5-Mar:      dumb typo in asymmetric-fm generator fixed.
 * 19-Feb:     buffer reallocation redundant free bugfix.
 * 25-Jan-02:  mus_increment of env returns base.
 * --------
 * 10-Dec:     add outctr calls, phase-vocoder bugfixes, thanks to Scott Wilson.
 * 21-Oct:     fill in some set-data methods.
 * 1-Sep:      mus_polar2rectangular.
 * 6-July:     scm -> xen.
 * 26-May:     mus_rand_seed.
 * 22-May:     locsig reverb distance calc was upside down.
 * 18-May:     mus_describe and mus_inspect returned string should not be freed any more.
   (previous version was not usable in gdb and was unneeded due to mus_snprintf)
 * 7-May:      filled in some leftover equal_p methods.
 * 1-Apr:      mus_make_file2sample_with_comment and mus_length for file->sample/sample->file.
 *             mus_file_buffer_size.
 * 26-Mar:     extended_type field added to mus_any_class for more robust type checking.
 * 16-Mar:     mus_phase of env -> current_value.
 * 28-Feb:     added mus_position(currently only for envs).
 * 8-Feb:      clm2scm.h.
 * 24-Jan:     mus-bank in clm2scm.
 * 5-Jan:      clm2scm gens are applicable.
 * 4-Jan:      mus_bank.
 * 2-Jan-01:   mus_run method.
 * --------
 * 28-Dec:     mus_clear_filter_state and other minor tweaks for Snd.
 * 28-Nov:     Dolph-Chebyshev window(under HAVE_GSL flag -- needs complex trig support).
 * 8-Nov:      mus_clear_sinc_tables.
 * 12-Oct:     mus_formant_bank takes one input(can't remember why I had an array here)
 * 27-Sep:     mus_array_interp bugfix(imitates mus.lisp now).
 * 18-Sep:     clm now assumes it's used as a part of sndlib.
 * 11-Sep:     generalized set! to generic functions in clm2scm.c.
 * 31-Aug:     changed formant field setters(thanks to Anders Vinjar).
 * 10-Aug:     removed built-in setf support(clm2scm.c).
 * 31-Jul:     mus_granulate tries to protect against illegal length and ramp values.
 * 24-Jul:     mus_make_fir_coeffs.
 * 20-Jul:     sum_of_sines, atan2 to rectangular->polar, phase_vocoder gen.
 * 22-June:    made mus_bessi0 local again.
 * 1-June:     bugfixes for linuxppc 2000.
 * 19-May:     mus_apply.
 * 8-May:      added "const" and XEN_PROCEDURE_CAST(for c++), made mus_bessi0 global.
 * 24-Apr:     changed formant radius to match lisp version(it's now 1-old_radius)
 * 20-Apr:     mus_convolve_files
 * 7-Apr:      src width bug fixed
 * 31-Mar:     finally implemented set-location for envs.
 * 14-Feb:     buffer-full?.
 * 1-Feb:      removed mus_phasepartials2waveshape.
 * 3-Jan-00:   format and type args added to make_sample2file, 
 *             mus_file_close. 
 *             removed make_file_input and make_file_output.
 * --------
 * 29-Dec:     various bugfixes especially in envelope handlers.
 * 19-Nov:     mus_oscil_bank and mus_formant_bank.
 * 5-Nov:      mus_sin exported.
 * 4-Oct:(scm) make-env arg order changed to reflect mus.lisp.
 * 29-Sep:     implemented mus-increment and mus-frequency for granulate(as in mus.lisp).
 *             clm's fft renamed mus-fft to avoid collision with snd's version.
 *             added max_size arg to make_granulate(to reflect mus.lisp).
 * 25-Sep-99:  added width arg to make_src -- forgot this somehow in first pass.
 *             decided to make mus_inspect return char* like mus_describe.
 */

/* isn't mus_env_interp backwards? */

/* compile time switches:
 *
 * WITH_SINE_TABLE(default 1)
 *   if 1, use table lookup for sine, else math library's sin
 *
 * HAVE_GSL(default 0)
 *   if 1, use libgsl for some math functions
 * 
 * Float can be defined to be double or float(default)
 *
 * HAVE_FFTW
 *  if 1, include mus_fftw which calls the fftw real fft
 */

#ifndef TRUE
  #define TRUE 1
#endif
#ifndef FALSE
  #define FALSE 0
#endif

#include "sndlib.h"

#if(!defined(M_PI))
  #define M_PI 3.14159265358979323846264338327
  #define M_PI_2 (M_PI / 2.0)
#endif

typedef struct {
  struct mus__any_class *core;
} mus_any;

typedef struct mus__any_class {
  int type;
  char *name;
  int (*release)(mus_any *ptr);
  char* (*describe)(mus_any *ptr);
  char* (*inspect)(mus_any *ptr);
  int (*equalp)(mus_any *gen1, mus_any *gen2);
  Float* (*data)(mus_any *ptr);
  Float* (*set_data)(mus_any *ptr, Float *new_data);
  int (*length)(mus_any *ptr);
  int (*set_length)(mus_any *ptr, int new_length);
  Float (*frequency)(mus_any *ptr);
  Float (*set_frequency)(mus_any *ptr, Float new_freq);
  Float (*phase)(mus_any *ptr); 
  Float (*set_phase)(mus_any *ptr, Float new_phase);
  Float (*scaler)(mus_any *ptr);
  Float (*set_scaler)(mus_any *ptr, Float val);
  Float (*increment)(mus_any *ptr);
  Float (*set_increment)(mus_any *ptr, Float val);
  Float (*run)(mus_any *gen, Float arg1, Float arg2);
  int extended_type;
  void* (*environ)(mus_any *gen);
  int (*channels)(mus_any *ptr);
  Float (*offset)(mus_any *ptr);
  Float (*set_offset)(mus_any *ptr, Float val);
  Float (*width)(mus_any *ptr);
  Float (*set_width)(mus_any *ptr, Float val);
  Float (*b2)(mus_any *ptr);
  Float (*set_b2)(mus_any *ptr, Float val);
  int (*hop)(mus_any *ptr);
  int (*set_hop)(mus_any *ptr, int new_length);
  int (*ramp)(mus_any *ptr);
  int (*set_ramp)(mus_any *ptr, int new_length);
  Float (*read_sample)(mus_any *ptr, off_t samp, int chan);
  Float (*write_sample)(mus_any *ptr, off_t samp, int chan, Float data);
  char* (*file_name)(mus_any *ptr);
  int (*end)(mus_any *ptr);
  off_t (*location)(mus_any *ptr);
  off_t (*set_location)(mus_any *ptr, off_t loc);
  int (*channel)(mus_any *ptr);
} mus_any_class;

enum {MUS_LINEAR, MUS_SINUSOIDAL};

enum {MUS_RECTANGULAR_WINDOW, MUS_HANN_WINDOW, MUS_WELCH_WINDOW, MUS_PARZEN_WINDOW, MUS_BARTLETT_WINDOW,
      MUS_HAMMING_WINDOW, MUS_BLACKMAN2_WINDOW, MUS_BLACKMAN3_WINDOW, MUS_BLACKMAN4_WINDOW,
      MUS_EXPONENTIAL_WINDOW, MUS_RIEMANN_WINDOW, MUS_KAISER_WINDOW, MUS_CAUCHY_WINDOW, MUS_POISSON_WINDOW,
      MUS_GAUSSIAN_WINDOW, MUS_TUKEY_WINDOW, MUS_DOLPH_CHEBYSHEV_WINDOW};

#define MUS_FFT_WINDOW_OK(Window) (((Window) >= MUS_RECTANGULAR_WINDOW) &&((Window) <= MUS_DOLPH_CHEBYSHEV_WINDOW))

#define MUS_RUN(GEN, ARG_1, ARG_2) ((*((GEN->core)->run))(GEN, ARG_1, ARG_2))
#define MUS_RUN_P(GEN) ((GEN->core)->run)


#ifdef __cplusplus
extern "C" {
#endif

void init_mus_module(void);

int mus_make_class_tag(void);
Float mus_radians2hz(Float rads);
Float mus_hz2radians(Float hz);
Float mus_degrees2radians(Float degree);
Float mus_radians2degrees(Float rads);
Float mus_db2linear(Float x);
Float mus_linear2db(Float x);
Float mus_srate(void);
Float mus_set_srate(Float val);
off_t mus_seconds_to_samples(Float secs);
Float mus_samples_to_seconds(off_t samps);
int mus_array_print_length(void);
int mus_set_array_print_length(int val);
Float mus_sin(Float phase);
Float mus_sum_of_sines(Float *amps, Float *phases, int size);

Float mus_ring_modulate(Float s1, Float s2);
Float mus_amplitude_modulate(Float s1, Float s2, Float s3);
Float mus_contrast_enhancement(Float sig, Float index);
Float mus_dot_product(Float *data1, Float *data2, int size);
void mus_clear_array(Float *arr, int size);
Float mus_polynomial(Float *coeffs, Float x, int ncoeffs);
void mus_multiply_arrays(Float *data, Float *window, int len);
void mus_rectangular2polar(Float *rl, Float *im, int size);
void mus_polar2rectangular(Float *rl, Float *im, int size);
Float mus_array_interp(Float *wave, Float phase, int size);

int mus_free(mus_any *ptr);
char *mus_describe(mus_any *gen);
char *mus_inspect(mus_any *gen);
int mus_equalp(mus_any *g1, mus_any *g2);
Float mus_phase(mus_any *gen);
Float mus_set_phase(mus_any *gen, Float val);
Float mus_set_frequency(mus_any *gen, Float val);
Float mus_frequency(mus_any *gen);
Float mus_run(mus_any *gen, Float arg1, Float arg2);
int mus_length(mus_any *gen);
int mus_set_length(mus_any *gen, int len);
Float *mus_data(mus_any *gen);
Float *mus_set_data(mus_any *gen, Float *data);
char *mus_name(mus_any *ptr);
int mus_type(mus_any *ptr);
Float mus_scaler(mus_any *gen);
Float mus_set_scaler(mus_any *gen, Float val);
Float mus_offset(mus_any *gen);
Float mus_set_offset(mus_any *gen, Float val);
Float mus_width(mus_any *gen);
Float mus_set_width(mus_any *gen, Float val);
char *mus_file_name(mus_any *ptr);

Float mus_oscil(mus_any *o, Float fm, Float pm);
Float mus_oscil_0(mus_any *ptr);
Float mus_oscil_1(mus_any *ptr, Float fm);
Float mus_oscil_bank(Float *amps, mus_any **oscils, Float *inputs, int size);
int mus_oscil_p(mus_any *ptr);
mus_any *mus_make_oscil(Float freq, Float phase);

Float mus_sum_of_cosines(mus_any *gen, Float fm);
int mus_sum_of_cosines_p(mus_any *ptr);
mus_any *mus_make_sum_of_cosines(int cosines, Float freq, Float phase);
#define mus_cosines(Gen) mus_hop(Gen)
#define mus_set_cosines(Gen, Val) mus_set_hop(Gen, Val)

Float mus_delay(mus_any *gen, Float input, Float pm);
Float mus_delay_1(mus_any *ptr, Float input);
Float mus_tap(mus_any *gen, Float loc);
Float mus_tap_1(mus_any *gen);
mus_any *mus_make_delay(int size, Float *line, int line_size);
int mus_delay_p(mus_any *ptr);
int mus_delay_line_p(mus_any *gen); /* added 2-Mar-03 for tap error checks */

Float mus_comb(mus_any *gen, Float input, Float pm);
Float mus_comb_1(mus_any *gen, Float input);
mus_any *mus_make_comb(Float scaler, int size, Float *line, int line_size);
int mus_comb_p(mus_any *ptr);

Float mus_notch(mus_any *gen, Float input, Float pm);
Float mus_notch_1(mus_any *gen, Float input);
mus_any *mus_make_notch(Float scaler, int size, Float *line, int line_size);
int mus_notch_p(mus_any *ptr);

Float mus_all_pass(mus_any *gen, Float input, Float pm);
Float mus_all_pass_1(mus_any *gen, Float input);
mus_any *mus_make_all_pass(Float backward, Float forward, int size, Float *line, int line_size);
int mus_all_pass_p(mus_any *ptr);

#define mus_feedforward(Gen) mus_scaler(Gen)
#define mus_set_feedforward(Gen, Val) mus_set_scaler(Gen, Val)
#define mus_feedback(Gen) mus_increment(Gen)
#define mus_set_feedback(Gen, Val) mus_set_increment(Gen, Val)

Float mus_table_lookup(mus_any *gen, Float fm);
Float mus_table_lookup_1(mus_any *gen);
mus_any *mus_make_table_lookup(Float freq, Float phase, Float *wave, int wave_size);
int mus_table_lookup_p(mus_any *ptr);
Float *mus_partials2wave(Float *partial_data, int partials, Float *table, int table_size, int normalize);
Float *mus_phasepartials2wave(Float *partial_data, int partials, Float *table, int table_size, int normalize);

Float mus_sawtooth_wave(mus_any *gen, Float fm);
mus_any *mus_make_sawtooth_wave(Float freq, Float amp, Float phase);
int mus_sawtooth_wave_p(mus_any *gen);

Float mus_square_wave(mus_any *gen, Float fm);
mus_any *mus_make_square_wave(Float freq, Float amp, Float phase);
int mus_square_wave_p(mus_any *gen);

Float mus_triangle_wave(mus_any *gen, Float fm);
mus_any *mus_make_triangle_wave(Float freq, Float amp, Float phase);
int mus_triangle_wave_p(mus_any *gen);

Float mus_pulse_train(mus_any *gen, Float fm);
mus_any *mus_make_pulse_train(Float freq, Float amp, Float phase);
int mus_pulse_train_p(mus_any *gen);

void mus_set_rand_seed(unsigned long seed);
unsigned long mus_rand_seed(void);
Float mus_random(Float amp);
Float mus_frandom(Float amp);
int mus_irandom(int amp);

Float mus_rand(mus_any *gen, Float fm);
mus_any *mus_make_rand(Float freq, Float base);
int mus_rand_p(mus_any *ptr);

Float mus_rand_interp(mus_any *gen, Float fm);
mus_any *mus_make_rand_interp(Float freq, Float base);
int mus_rand_interp_p(mus_any *ptr);

Float mus_asymmetric_fm(mus_any *gen, Float index, Float fm);
Float mus_asymmetric_fm_1(mus_any *gen, Float index);
mus_any *mus_make_asymmetric_fm(Float freq, Float phase, Float r, Float ratio);
int mus_asymmetric_fm_p(mus_any *ptr);

Float mus_one_zero(mus_any *gen, Float input);
mus_any *mus_make_one_zero(Float a0, Float a1);
int mus_one_zero_p(mus_any *gen);

Float mus_one_pole(mus_any *gen, Float input);
mus_any *mus_make_one_pole(Float a0, Float b1);
int mus_one_pole_p(mus_any *gen);

Float mus_two_zero(mus_any *gen, Float input);
mus_any *mus_make_two_zero(Float a0, Float a1, Float a2);
int mus_two_zero_p(mus_any *gen);
mus_any *mus_make_zpolar(Float radius, Float frequency);

Float mus_two_pole(mus_any *gen, Float input);
mus_any *mus_make_two_pole(Float a0, Float b1, Float b2);
int mus_two_pole_p(mus_any *gen);
mus_any *mus_make_ppolar(Float radius, Float frequency);

#define mus_a0(Gen) mus_scaler(Gen)
#define mus_set_a0(Gen, Val) mus_set_scaler(Gen, Val)
#define mus_a1(Gen) mus_offset(Gen)
#define mus_set_a1(Gen, Val) mus_set_offset(Gen, Val)
#define mus_a2(Gen) mus_width(Gen)
#define mus_set_a2(Gen, Val) mus_set_width(Gen, Val)
#define mus_b1(Gen) mus_increment(Gen)
#define mus_set_b1(Gen, Val) mus_set_increment(Gen, Val)
Float mus_b2(mus_any *ptr);
Float mus_set_b2(mus_any *ptr, Float val);

Float mus_formant(mus_any *ptr, Float input); 
Float mus_formant_bank(Float *amps, mus_any **formants, Float inval, int size);
mus_any *mus_make_formant(Float radius, Float frequency, Float gain);
int mus_formant_p(mus_any *ptr);
#define mus_formant_radius(Gen) mus_phase(Gen)
#define mus_set_formant_radius(Gen, Val) mus_set_phase(Gen, Val)
void mus_set_formant_radius_and_frequency(mus_any *ptr, Float radius, Float frequency);

Float mus_x1(mus_any *gen);
Float mus_set_x1(mus_any *gen, Float val);
Float mus_x2(mus_any *gen);
Float mus_set_x2(mus_any *gen, Float val);
Float mus_y1(mus_any *gen);
Float mus_set_y1(mus_any *gen, Float val);
Float mus_y2(mus_any *gen);
Float mus_set_y2(mus_any *gen, Float val);

Float mus_sine_summation(mus_any *ptr, Float fm);
mus_any *mus_make_sine_summation(Float frequency, Float phase, int n, Float a, Float b_ratio);
int mus_sine_summation_p(mus_any *ptr);

Float mus_filter(mus_any *ptr, Float input);
mus_any *mus_make_filter(int order, Float *xcoeffs, Float *ycoeffs, Float *state);
int mus_filter_p(mus_any *ptr);

Float mus_fir_filter(mus_any *ptr, Float input);
mus_any *mus_make_fir_filter(int order, Float *xcoeffs, Float *state);
int mus_fir_filter_p(mus_any *ptr);

Float mus_iir_filter(mus_any *ptr, Float input);
mus_any *mus_make_iir_filter(int order, Float *ycoeffs, Float *state);
int mus_iir_filter_p(mus_any *ptr);
Float *mus_make_fir_coeffs(int order, Float *env, Float *aa);

Float *mus_xcoeffs(mus_any *ptr);
Float *mus_ycoeffs(mus_any *ptr);
#define mus_order(Gen) mus_length(Gen)
void mus_clear_filter_state(mus_any *gen);

Float mus_wave_train(mus_any *gen, Float fm);
Float mus_wave_train_1(mus_any *gen);
mus_any *mus_make_wave_train(Float freq, Float phase, Float *wave, int wsize);
int mus_wave_train_p(mus_any *gen);

Float mus_buffer2sample(mus_any *ptr);
Float mus_sample2buffer(mus_any *ptr, Float val);
mus_any *mus_make_buffer(Float *preloaded_buffer, int size, Float current_file_time);
int mus_buffer_p(mus_any *ptr);
int mus_buffer_empty_p(mus_any *ptr);
int mus_buffer_full_p(mus_any *ptr);
mus_any *mus_buffer2frame(mus_any *rb, mus_any *fr);
mus_any *mus_frame2buffer(mus_any *rb, mus_any *fr);

mus_any *mus_make_waveshape(Float frequency, Float phase, Float *table, int size);
Float mus_waveshape(mus_any *ptr, Float index, Float fm);
Float mus_waveshape_1(mus_any *ptr, Float index);
int mus_waveshape_p(mus_any *ptr);
Float *mus_partials2waveshape(int npartials, Float *partials, int size, Float *table);
Float *mus_partials2polynomial(int npartials, Float *partials, int kind);

Float mus_env(mus_any *ptr);
mus_any *mus_make_env(Float *brkpts, int pts, Float scaler, Float offset, Float base, Float duration, off_t start, off_t end, Float *odata);
int mus_env_p(mus_any *ptr);
void mus_restart_env(mus_any *ptr);
Float mus_env_interp(Float x, mus_any *env);
off_t *mus_env_passes(mus_any *gen); /* for Snd */
double *mus_env_rates(mus_any *gen); /* for Snd */
double mus_env_offset(mus_any *gen); /* for Snd */
double mus_env_scaler(mus_any *gen); /* for Snd */
double mus_env_initial_power(mus_any *gen); /* for Snd */
int mus_env_breakpoints(mus_any *gen); /* for Snd */
#define mus_position(Gen) mus_channels(Gen)

int mus_frame_p(mus_any *ptr);
mus_any *mus_make_empty_frame(int chans);
mus_any *mus_make_frame(int chans, ...);
mus_any *mus_frame_add(mus_any *f1, mus_any *f2, mus_any *res);
mus_any *mus_frame_multiply(mus_any *f1, mus_any *f2, mus_any *res);
Float mus_frame_ref(mus_any *f, int chan);
Float mus_frame_set(mus_any *f, int chan, Float val);
Float *mus_frame_data(mus_any *f);

int mus_mixer_p(mus_any *ptr);
mus_any *mus_make_empty_mixer(int chans);
mus_any *mus_make_identity_mixer(int chans);
mus_any *mus_make_mixer(int chans, ...);
Float **mus_mixer_data(mus_any *f);
Float mus_mixer_ref(mus_any *f, int in, int out);
Float mus_mixer_set(mus_any *f, int in, int out, Float val);
mus_any *mus_frame2frame(mus_any *f, mus_any *in, mus_any *out);
mus_any *mus_sample2frame(mus_any *f, Float in, mus_any *out);
Float mus_frame2sample(mus_any *f, mus_any *in);
mus_any *mus_mixer_multiply(mus_any *f1, mus_any *f2, mus_any *res);

int mus_file2sample_p(mus_any *ptr);
mus_any *mus_make_file2sample(const char *filename);
Float mus_file2sample(mus_any *ptr, off_t samp, int chan);

Float mus_readin(mus_any *rd);
mus_any *mus_make_readin(const char *filename, int chan, off_t start, int direction);
int mus_readin_p(mus_any *ptr);
Float mus_increment(mus_any *rd);
Float mus_set_increment(mus_any *rd, Float dir);
off_t mus_location(mus_any *rd);
off_t mus_set_location(mus_any *rd, off_t loc);
int mus_channel(mus_any *rd);

int mus_output_p(mus_any *ptr);
int mus_input_p(mus_any *ptr);
Float mus_in_any(off_t frame, int chan, mus_any *IO);
Float mus_ina(off_t frame, mus_any *inp);
Float mus_inb(off_t frame, mus_any *inp);

mus_any *mus_make_file2frame(const char *filename);
int mus_file2frame_p(mus_any *ptr);
mus_any *mus_file2frame(mus_any *ptr, off_t samp, mus_any *f);

int mus_sample2file_p(mus_any *ptr);
mus_any *mus_make_sample2file(const char *filename, int chans, int out_format, int out_type);
mus_any *mus_make_sample2file_with_comment(const char *filename, int out_chans, int out_format, int out_type, const char *comment);
Float mus_sample2file(mus_any *ptr, off_t samp, int chan, Float val);
int mus_close_file(mus_any *ptr);
mus_any *mus_continue_sample2file(const char *filename);

Float mus_out_any(off_t frame, Float val, int chan, mus_any *IO);
Float mus_outa(off_t frame, Float val, mus_any *IO);
Float mus_outb(off_t frame, Float val, mus_any *IO);
Float mus_outc(off_t frame, Float val, mus_any *IO);
Float mus_outd(off_t frame, Float val, mus_any *IO);

mus_any *mus_make_frame2file(const char *filename, int chans, int out_format, int out_type);
int mus_frame2file_p(mus_any *ptr);
mus_any *mus_frame2file(mus_any *ptr, off_t samp, mus_any *data);

mus_any *mus_locsig(mus_any *ptr, off_t loc, Float val);
mus_any *mus_make_locsig(Float degree, Float distance, Float reverb, int chans, mus_any *output, mus_any *revput, int type);
int mus_locsig_p(mus_any *ptr);
int mus_channels(mus_any *ptr);
Float mus_locsig_ref(mus_any *ptr, int chan);
Float mus_locsig_set(mus_any *ptr, int chan, Float val);
Float mus_locsig_reverb_ref(mus_any *ptr, int chan);
Float mus_locsig_reverb_set(mus_any *ptr, int chan, Float val);
void mus_move_locsig(mus_any *ptr, Float degree, Float distance);
void mus_fill_locsig(Float *arr, int chans, Float degree, Float scaler, int type);

mus_any *mus_make_src(Float(*input)(void *arg, int direction), Float srate, int width, void *environ);
Float mus_src(mus_any *srptr, Float sr_change, Float(*input)(void *arg, int direction));
int mus_src_p(mus_any *ptr);

int mus_convolve_p(mus_any *ptr);
Float mus_convolve(mus_any *ptr, Float(*input)(void *arg, int direction));
mus_any *mus_make_convolve(Float(*input)(void *arg, int direction), Float *filter, int fftsize, int filtersize, void *environ);
Float *mus_spectrum(Float *rdat, Float *idat, Float *window, int n, int type);
void mus_fft(Float *rl, Float *im, int n, int is);
#if HAVE_FFTW || HAVE_FFTW3
void mus_fftw(Float *rl, int n, int dir);
#endif
Float *mus_make_fft_window(int type, int size, Float beta);
Float *mus_make_fft_window_with_window(int type, int size, Float beta, Float *window);
Float *mus_convolution(Float* rl1, Float* rl2, int n);
void mus_convolve_files(const char *file1, const char *file2, Float maxamp, const char *output_file);

int mus_granulate_p(mus_any *ptr);
Float mus_granulate(mus_any *ptr, Float(*input)(void *arg, int direction));
mus_any *mus_make_granulate(Float(*input)(void *arg, int direction), 
				       Float expansion, Float length, Float scaler, 
				       Float hop, Float ramp, Float jitter, int max_size, void *environ);
int mus_ramp(mus_any *ptr);
int mus_set_ramp(mus_any *ptr, int val);
int mus_hop(mus_any *ptr);
int mus_set_hop(mus_any *ptr, int val);

int mus_set_file_buffer_size(int size);
int mus_file_buffer_size(void);

void mus_mix(const char *outfile, const char *infile, off_t out_start, off_t out_samps, off_t in_start, mus_any *mx, mus_any ***envs);
void mus_mix_with_reader_and_writer(mus_any *outf, mus_any *inf, off_t out_start, off_t out_frames, off_t in_start, mus_any *umx, mus_any ***envs);
int mus_file2fltarray(const char *filename, int chan, off_t start, int samples, Float *array);
int mus_fltarray2file(const char *filename, Float *ddata, int len, int srate, int channels);

Float mus_apply(mus_any *gen, ...);
Float mus_bank(mus_any **gens, Float *scalers, Float *arg1, Float *arg2, int size);

int mus_phase_vocoder_p(mus_any *ptr);
mus_any *mus_make_phase_vocoder(Float(*input)(void *arg, int direction), 
				       int fftsize, int overlap, int interp,
				       Float pitch,
				       int(*analyze)(void *arg, Float(*input)(void *arg1, int direction)),
				       int(*edit)(void *arg), 
				       Float(*synthesize)(void *arg), 
				       void *environ);
Float mus_phase_vocoder(mus_any *ptr, Float(*input)(void *arg, int direction));

Float *mus_phase_vocoder_amp_increments(mus_any *ptr);
Float *mus_phase_vocoder_amps(mus_any *ptr);
Float *mus_phase_vocoder_freqs(mus_any *ptr);
Float *mus_phase_vocoder_phases(mus_any *ptr);
Float *mus_phase_vocoder_phase_increments(mus_any *ptr);
Float *mus_phase_vocoder_previous_phases(mus_any *ptr);
int mus_phase_vocoder_outctr(mus_any *ptr);
int mus_phase_vocoder_set_outctr(mus_any *ptr, int val);

void mus_clear_sinc_tables(void);
void *mus_environ(mus_any *rd);

#ifdef __cplusplus
}
#endif

#endif
