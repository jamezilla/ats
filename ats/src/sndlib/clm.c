/* CLM (music V) implementation as a C module */
/*
 *   the leftovers now are: xcoeffs, ycoeffs, x1,x2,y1,y2
 *   to make global (gen-relative) method change, need way to set gen's class fields
 *   to make gen-local method change, need to copy class, reassign core, set desired field
 *   or add-to-existing would need access to current
 */

#if defined(HAVE_CONFIG_H)
  #include <config.h>
#endif

#if USE_SND
  #include "snd.h"
#endif

#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef MPW_C
#if (defined(HAVE_LIBC_H) && (!defined(HAVE_UNISTD_H)))
  #include <libc.h>
#else
  #ifndef _MSC_VER
    #include <unistd.h>
  #endif
#endif
#endif

#include "clm.h"
#include "clm-strings.h"

#if HAVE_GSL
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#endif

#if HAVE_FFTW3
  #include <fftw3.h>
#else
  #if HAVE_FFTW
    #include <rfftw.h>
  #endif
#endif

enum {MUS_OSCIL, MUS_SUM_OF_COSINES, MUS_DELAY, MUS_COMB, MUS_NOTCH, MUS_ALL_PASS,
      MUS_TABLE_LOOKUP, MUS_SQUARE_WAVE, MUS_SAWTOOTH_WAVE, MUS_TRIANGLE_WAVE, MUS_PULSE_TRAIN,
      MUS_RAND, MUS_RAND_INTERP, MUS_ASYMMETRIC_FM, MUS_ONE_ZERO, MUS_ONE_POLE, MUS_TWO_ZERO, MUS_TWO_POLE, MUS_FORMANT,
      MUS_WAVESHAPE, MUS_SRC, MUS_GRANULATE, MUS_SINE_SUMMATION, MUS_WAVE_TRAIN, MUS_BUFFER,
      MUS_FILTER, MUS_FIR_FILTER, MUS_IIR_FILTER, MUS_CONVOLVE, MUS_ENV, MUS_LOCSIG,
      MUS_FRAME, MUS_READIN, MUS_FILE2SAMPLE, MUS_FILE2FRAME,
      MUS_SAMPLE2FILE, MUS_FRAME2FILE, MUS_MIXER, MUS_PHASE_VOCODER,
      MUS_INITIAL_GEN_TAG};

enum {MUS_NOT_SPECIAL, MUS_SIMPLE_FILTER, MUS_FULL_FILTER, MUS_OUTPUT, MUS_INPUT, MUS_DELAY_LINE};

static int mus_class_tag = MUS_INITIAL_GEN_TAG;
int mus_make_class_tag(void) {return(mus_class_tag++);}

static Float sampling_rate = 22050.0;
static Float w_rate = (TWO_PI / 22050.0);
static int array_print_length = 8;
/* all these globals need to be set explicitly if we're using clm from a shared library */

static int clm_file_buffer_size = 8192;
int mus_file_buffer_size(void) {return(clm_file_buffer_size);}
int mus_set_file_buffer_size(int size) {clm_file_buffer_size = size; return(size);}

#define DESCRIBE_BUFFER_SIZE 2048
static char describe_buffer[DESCRIBE_BUFFER_SIZE];

#ifndef WITH_SINE_TABLE
  #define WITH_SINE_TABLE 1
#endif

#if WITH_SINE_TABLE

  /* use of "sin" rather than table lookup of a stored sine wave costs us about 
   * 1.5 (23/16) in compute time on a Pentium, using all doubles is slightly faster (~10%)
   * 2.5 (14/6) on an SGI
   * 4.5 (86/18) on a PPC
   * 4 (88/23) on a Sun
   * 5 (31/6) on a NeXT
   */

#define SINE_SIZE 8192
static Float *sine_table = NULL;

static void *clm_calloc(int num, int size, const char* what)
{
  register void *mem;
#if DEBUG_MEMORY
  mem = mem_calloc(num, size, what, __FILE__, __LINE__);
#else
  mem = CALLOC(num, size);
#endif
  if (mem == 0)
    mus_error(MUS_MEMORY_ALLOCATION_FAILED, "can't allocate %s", what);
  return(mem);
}

static void init_sine (void)
{
  int i;
  Float phase, incr;
  if (sine_table == NULL)
    {
      sine_table = (Float *)clm_calloc(SINE_SIZE + 1, sizeof(Float), "sine table");
      incr = TWO_PI / (Float)SINE_SIZE;
      for (i = 0, phase = 0.0; i < SINE_SIZE + 1; i++, phase += incr)
	sine_table[i] = (Float)sin(phase);
    }
}

Float mus_sin(Float phase)
{
  Float mag_phase, frac;
  int index;
  if (phase < 0.0)
    {
      index = (int)(phase / TWO_PI);
      phase += (Float)(((1 - index) * (TWO_PI)));
    }
  mag_phase = phase * SINE_SIZE / TWO_PI;
  index = (int)mag_phase;
  frac = mag_phase - index;
  index = index % SINE_SIZE;
  if (frac < 0.0001)
    return(sine_table[index]);
  else return(sine_table[index] + 
	      (frac * (sine_table[index + 1] - sine_table[index])));
}

#else
  Float mus_sin(Float phase) {return(sin(phase));}
#endif

Float mus_sum_of_sines(Float *amps, Float *phases, int size)
{
  int i;
  Float sum = 0.0;
  for (i = 0; i < size; i++) 
    if (amps[i] != 0.0)
      sum += (amps[i] * mus_sin(phases[i]));
  return(sum);
}

static int check_gen(mus_any *ptr, const char *name)
{
  if (ptr == NULL)
    return(mus_error(MUS_NO_GEN, "null gen passed to %s", name));
  return(TRUE);
}

int mus_type(mus_any *ptr) 
{
  if (check_gen(ptr, "mus-type"))
    return((ptr->core)->type);
  return(0);
}

char *mus_name(mus_any *ptr) 
{
  if (ptr == NULL)
    return("null");
  return((ptr->core)->name);
}

static void describe_bad_gen(mus_any *ptr, char *gen_name, char *an)
{
  if (!ptr)
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "arg to describe_%s is null", gen_name);
  else
    {
      if ((ptr->core)->type < mus_class_tag)
	mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "arg to describe_%s appears to be %s %s", gen_name, an, mus_name(ptr));
      else mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "arg to describe_%s is not %s %s", gen_name, an, gen_name);
    }
}

Float mus_radians2hz(Float rads) {return(rads / w_rate);}
Float mus_hz2radians(Float hz) {return(hz * w_rate);}
Float mus_degrees2radians(Float degree) {return(degree * TWO_PI / 360.0);}
Float mus_radians2degrees(Float rads) {return(rads * 360.0 / TWO_PI);}
Float mus_db2linear(Float x) {return(pow(10.0, x / 20.0));}
Float mus_linear2db(Float x) {return(20.0 * log10(x));}

Float mus_srate(void) {return(sampling_rate);}
Float mus_set_srate(Float val) {if (val > 0.0) sampling_rate = val; w_rate = (TWO_PI / sampling_rate); return(sampling_rate);}
off_t mus_seconds_to_samples(Float secs) {return((off_t)(secs * sampling_rate));}
Float mus_samples_to_seconds(off_t samps) {return((Float)((double)samps / (double)sampling_rate));}

int mus_array_print_length(void) {return(array_print_length);}
int mus_set_array_print_length(int val) {array_print_length = val; return(val);}

static char *print_array(Float *arr, int len, int loc)
{
  char *base, *str;
  int i, lim, k, size = 128;
  if (arr == NULL) 
    {
      str = (char *)CALLOC(4, sizeof(char));
      sprintf(str, "nil");
      return(str);
    }
  if ((array_print_length * 12) > size) size = array_print_length * 12;
  base = (char *)CALLOC(size, sizeof(char));
  str = (char *)CALLOC(64, sizeof(char));
  sprintf(base, "[");
  lim = len;
  if (lim > array_print_length) lim = array_print_length;
  k = loc;
  for (i = 0; i < lim - 1; i++)
    {
      mus_snprintf(str, 64, "%.3f ", arr[k]);
      strcat(base, str);
      if ((int)(strlen(base) + 32) > size)
	{
	  base = (char *)REALLOC(base, size * 2 * sizeof(char));
	  base[size] = 0;
	  size *= 2;
	}
      k++;
      if (k >= len) k = 0;
    }
  mus_snprintf(str, 64, "%.3f%s]", arr[k], (len > lim) ? "..." : "");
  strcat(base, str);
  FREE(str);
  return(base);
}

static char *print_double_array(double *arr, int len, int loc)
{
  char *base, *str;
  int i, lim, k, size = 128;
  if (arr == NULL) 
    {
      str = (char *)CALLOC(4, sizeof(char));
      sprintf(str, "nil");
      return(str);
    }
  if ((array_print_length * 16) > size) size = array_print_length * 16;
  base = (char *)CALLOC(size, sizeof(char));
  str = (char *)CALLOC(128, sizeof(char));
  sprintf(base, "[");
  lim = len;
  if (lim > array_print_length) lim = array_print_length;
  k = loc;
  for (i = 0; i < lim - 1; i++)
    {
      mus_snprintf(str, 128, "%.3f ", arr[k]);
      strcat(base, str);
      if ((int)(strlen(base) + 32) > size)
	{
	  base = (char *)REALLOC(base, size * 2 * sizeof(char));
	  base[size] = 0;
	  size *= 2;
	}
      k++;
      if (k >= len) k = 0;
    }
  mus_snprintf(str, 128, "%.3f%s]", arr[k], (len > lim) ? "..." : "");
  strcat(base, str);
  FREE(str);
  return(base);
}

static char *print_off_t_array(off_t *arr, int len, int loc)
{
  char *base, *str;
  int i, lim, k, size = 128;
  if (arr == NULL) 
    {
      str = (char *)CALLOC(4, sizeof(char));
      sprintf(str, "nil");
      return(str);
    }
  if ((array_print_length * 16) > size) size = array_print_length * 16;
  base = (char *)CALLOC(size, sizeof(char));
  str = (char *)CALLOC(32, sizeof(char));
  sprintf(base, "[");
  lim = len;
  if (lim > array_print_length) lim = array_print_length;
  k = loc;
  for (i = 0; i < lim - 1; i++)
    {
      mus_snprintf(str, 32, OFF_TD " ", arr[k]);
      strcat(base, str);
      if ((int)(strlen(base) + 32) > size)
	{
	  base = (char *)REALLOC(base, size * 2 * sizeof(char));
	  base[size] = 0;
	  size *= 2;
	}
      k++;
      if (k >= len) k = 0;
    }
  mus_snprintf(str, 32, OFF_TD "%s]", arr[k], (len > lim) ? "..." : "");
  strcat(base, str);
  FREE(str);
  return(base);
}


/* ---------------- generic functions ---------------- */

#ifndef S_setB
#if HAVE_RUBY
  #define S_setB "set_"
#else
  #if HAVE_GUILE
    #define S_setB "set! "
  #else
    #define S_setB "set-"
  #endif
#endif
#endif

int mus_free(mus_any *gen)
{
  if ((check_gen(gen, "mus-free")) &&
      ((gen->core)->release))
    return((*((gen->core)->release))(gen));
  return(mus_error(MUS_NO_FREE, "can't free %s", mus_name(gen)));
}

char *mus_describe(mus_any *gen)
{
  if (gen == NULL)
    return("null clm gen");
  if ((gen->core) && ((gen->core)->describe))
    return((*((gen->core)->describe))(gen));
  else mus_error(MUS_NO_DESCRIBE, "can't describe %s", mus_name(gen));
  return(NULL);
}

char *mus_inspect(mus_any *gen)
{
  if (gen == NULL)
    return("null clm gen");
  if ((gen->core) && ((gen->core)->inspect))
    return((*((gen->core)->inspect))(gen));
  else mus_error(MUS_NO_INSPECT, "can't inspect %s", mus_name(gen));
  return(NULL);
}

int mus_equalp(mus_any *p1, mus_any *p2)
{
  if ((p1) && (p2))
    {
      if ((p1->core)->equalp)
	return((*((p1->core)->equalp))(p1, p2));
      else return(p1 == p2);
    }
  return(TRUE); /* (eq nil nil) */
}

Float mus_frequency(mus_any *gen)
{
  if ((check_gen(gen, S_mus_frequency)) &&
      ((gen->core)->frequency))
    return((*((gen->core)->frequency))(gen));
  return((Float)mus_error(MUS_NO_FREQUENCY, "can't get %s's frequency", mus_name(gen)));
}

Float mus_set_frequency(mus_any *gen, Float val)
{
  if ((check_gen(gen, S_setB S_mus_frequency)) &&
      ((gen->core)->set_frequency))
    return((*((gen->core)->set_frequency))(gen, val));
  return((Float)mus_error(MUS_NO_FREQUENCY, "can't set %s's frequency", mus_name(gen)));
}

Float mus_phase(mus_any *gen)
{
  if ((check_gen(gen, S_mus_phase)) &&
      ((gen->core)->phase))
    return((*((gen->core)->phase))(gen));
  return((Float)mus_error(MUS_NO_PHASE, "can't get %s's phase", mus_name(gen)));
}

Float mus_set_phase(mus_any *gen, Float val)
{
  if ((check_gen(gen, S_setB S_mus_phase)) &&
      ((gen->core)->set_phase))
    return((*((gen->core)->set_phase))(gen, val));
  return((Float)mus_error(MUS_NO_PHASE, "can't set %s's phase", mus_name(gen)));
}

Float mus_scaler(mus_any *gen)
{
  if ((check_gen(gen, S_mus_scaler)) &&
      ((gen->core)->scaler))
    return((*((gen->core)->scaler))(gen));
  return((Float)mus_error(MUS_NO_SCALER, "can't get %s's scaler", mus_name(gen)));
}

Float mus_set_scaler(mus_any *gen, Float val)
{
  if ((check_gen(gen, S_setB S_mus_scaler)) &&
      ((gen->core)->set_scaler))
    return((*((gen->core)->set_scaler))(gen, val));
  return((Float)mus_error(MUS_NO_SCALER, "can't set %s's scaler", mus_name(gen)));
}

Float mus_offset(mus_any *gen)
{
  if ((check_gen(gen, S_mus_offset)) &&
      ((gen->core)->offset))
    return((*((gen->core)->offset))(gen));
  return((Float)mus_error(MUS_NO_OFFSET, "can't get %s's offset", mus_name(gen)));
}

Float mus_set_offset(mus_any *gen, Float val)
{
  if ((check_gen(gen, S_setB S_mus_offset)) &&
      ((gen->core)->set_offset))
    return((*((gen->core)->set_offset))(gen, val));
  return((Float)mus_error(MUS_NO_OFFSET, "can't set %s's offset", mus_name(gen)));
}

Float mus_width(mus_any *gen)
{
  if ((check_gen(gen, S_mus_width)) &&
      ((gen->core)->width))
    return((*((gen->core)->width))(gen));
  return((Float)mus_error(MUS_NO_WIDTH, "can't get %s's width", mus_name(gen)));
}

Float mus_set_width(mus_any *gen, Float val)
{
  if ((check_gen(gen, S_setB S_mus_width)) &&
      ((gen->core)->set_width))
    return((*((gen->core)->set_width))(gen, val));
  return((Float)mus_error(MUS_NO_WIDTH, "can't set %s's width", mus_name(gen)));
}

Float mus_b2(mus_any *gen)
{
  if ((check_gen(gen, S_mus_b2)) &&
      ((gen->core)->b2))
    return((*((gen->core)->b2))(gen));
  return((Float)mus_error(MUS_NO_B2, "can't get %s's b2", mus_name(gen)));
}

Float mus_set_b2(mus_any *gen, Float val)
{
  if ((check_gen(gen, S_setB S_mus_b2)) &&
      ((gen->core)->set_b2))
    return((*((gen->core)->set_b2))(gen, val));
  return((Float)mus_error(MUS_NO_B2, "can't set %s's b2", mus_name(gen)));
}

Float mus_increment(mus_any *gen)
{
  if ((check_gen(gen, S_mus_increment)) &&
      ((gen->core)->increment))
    return((*((gen->core)->increment))(gen));
  return((Float)mus_error(MUS_NO_INCREMENT, "can't get %s's increment", mus_name(gen)));
}

Float mus_set_increment(mus_any *gen, Float val)
{
  if ((check_gen(gen, S_setB S_mus_increment)) &&
      ((gen->core)->set_increment))
    return((*((gen->core)->set_increment))(gen, val));
  return((Float)mus_error(MUS_NO_INCREMENT, "can't set %s's increment", mus_name(gen)));
}

void *mus_environ(mus_any *gen)
{
  if (check_gen(gen, "mus-environ"))
    return((*(gen->core->environ))(gen));
  return(NULL);
}

Float mus_run(mus_any *gen, Float arg1, Float arg2)
{
  if ((check_gen(gen, "mus-run")) &&
      ((gen->core)->run))
    return((*((gen->core)->run))(gen, arg1, arg2));
  return((Float)mus_error(MUS_NO_RUN, "can't run %s", mus_name(gen)));
}

int mus_length(mus_any *gen)
{
  if ((check_gen(gen, S_mus_length)) &&
      ((gen->core)->length))
    return((*((gen->core)->length))(gen));
  return(mus_error(MUS_NO_LENGTH, "can't get %s's length", mus_name(gen)));
}

int mus_set_length(mus_any *gen, int len)
{
  if ((check_gen(gen, S_setB S_mus_length)) &&
      ((gen->core)->set_length))
    return((*((gen->core)->set_length))(gen, len));
  return(mus_error(MUS_NO_LENGTH, "can't set %s's length", mus_name(gen)));
}

int mus_channels(mus_any *gen)
{
  if ((check_gen(gen, S_mus_channels)) &&
      ((gen->core)->channels))
    return((*((gen->core)->channels))(gen));
  return(mus_error(MUS_NO_CHANNELS, "can't get %s's channels", mus_name(gen)));
}

int mus_channel(mus_any *gen)
{
  if ((check_gen(gen, S_mus_channel)) &&
      ((gen->core)->channel))
    return(((*(gen->core)->channel))(gen));
  return(mus_error(MUS_NO_CHANNEL, "can't get %s's channel", mus_name(gen)));
}

int mus_hop(mus_any *gen)
{
  if ((check_gen(gen, S_mus_hop)) &&
      ((gen->core)->hop))
    return((*((gen->core)->hop))(gen));
  return(mus_error(MUS_NO_HOP, "can't get %s's hop value", mus_name(gen)));
}

int mus_set_hop(mus_any *gen, int len)
{
  if ((check_gen(gen, S_setB S_mus_hop)) &&
      ((gen->core)->set_hop))
    return((*((gen->core)->set_hop))(gen, len));
  return(mus_error(MUS_NO_HOP, "can't set %s's hop value", mus_name(gen)));
}

int mus_ramp(mus_any *gen)
{
  if ((check_gen(gen, S_mus_ramp)) &&
      ((gen->core)->ramp))
    return((*((gen->core)->ramp))(gen));
  return(mus_error(MUS_NO_RAMP, "can't get %s's ramp value", mus_name(gen)));
}

int mus_set_ramp(mus_any *gen, int len)
{
  if ((check_gen(gen, S_setB S_mus_ramp)) &&
      ((gen->core)->set_ramp))
    return((*((gen->core)->set_ramp))(gen, len));
  return(mus_error(MUS_NO_RAMP, "can't set %s's ramp value", mus_name(gen)));
}

Float *mus_data(mus_any *gen)
{
  if ((check_gen(gen, S_mus_data)) &&
      ((gen->core)->data))
    return((*((gen->core)->data))(gen));
  mus_error(MUS_NO_DATA, "can't get %s's data", mus_name(gen));
  return(NULL);
}

/* every case that implements the data or set data functions needs to include
 * a var-allocated flag, since all such memory has to be handled via vcts
 * in guile; a subsequent free by the enclosing object could leave a dangling
 * pointer in guile -- see clm2xen.c
 */

Float *mus_set_data(mus_any *gen, Float *new_data)
{
  if (check_gen(gen, S_setB S_mus_data))
    {
      if ((gen->core)->set_data)
	{
	  (*((gen->core)->set_data))(gen, new_data);
	  return(new_data);
	}
      else mus_error(MUS_NO_DATA, "can't set %s's data", mus_name(gen));
    }
  return(new_data);
}

off_t mus_location(mus_any *gen)
{
  if ((check_gen(gen, S_mus_location)) &&
      ((gen->core)->location))
    return(((*(gen->core)->location))(gen));
  return((off_t)mus_error(MUS_NO_LOCATION, "can't get %s's location", mus_name(gen)));
}



/* ---------------- AM etc ---------------- */

Float mus_ring_modulate(Float sig1, Float sig2) {return(sig1 * sig2);}
Float mus_amplitude_modulate(Float carrier, Float sig1, Float sig2) {return(sig1 * (carrier + sig2));}
Float mus_contrast_enhancement(Float sig, Float index) {return(mus_sin((sig * M_PI_2) + (index * mus_sin(sig * TWO_PI))));}
void mus_clear_array(Float *arr, int size) {memset((void *)arr, 0, size * sizeof(Float));}

Float mus_dot_product(Float *data1, Float *data2, int size)
{
  int i;
  Float sum = 0.0;
  for (i = 0; i < size; i++) sum += (data1[i] * data2[i]);
  return(sum);
}

Float mus_polynomial(Float *coeffs, Float x, int ncoeffs)
{
  Float sum;
  int i;
  if (ncoeffs <= 0) return(x);
  sum = coeffs[ncoeffs - 1];
  for (i = ncoeffs - 2; i >= 0; i--) sum = (sum * x) + coeffs[i];
  return(sum);
}

void mus_multiply_arrays(Float *data, Float *window, int len)
{
  int i;
  for (i = 0; i < len; i++) data[i] *= window[i];
}

void mus_rectangular2polar(Float *rl, Float *im, int size) 
{
  int i; 
  Float temp; /* apparently floating underflows in sqrt are bringing us to a halt */
  for (i = 0; i < size; i++)
    {
      temp = rl[i] * rl[i] + im[i] * im[i];
      im[i] = -atan2(im[i], rl[i]); /* "-" here so that clockwise is positive? */
      if (temp < .00000001) 
	rl[i] = 0.0;
      else rl[i] = sqrt(temp);
    }
}

void mus_polar2rectangular(Float *rl, Float *im, int size) 
{
  int i; 
  Float temp;
  for (i = 0; i < size; i++)
    {
      temp = rl[i] * sin(-im[i]); /* minus to match sense of above */
      rl[i] *= cos(-im[i]);
      im[i] = temp;
    }
}

Float mus_array_interp(Float *wave, Float phase, int size)
{
  /* changed 26-Sep-00 to be closer to mus.lisp */
  int int_part, inx;
  Float frac_part;
  if ((phase < 0.0) || (phase > size))
    {
      /* 28-Mar-01 changed to fmod; I was hoping to avoid this... */
      phase = fmod((double)phase, (double)size);
      if (phase < 0.0) phase += size;
    }
  int_part = (int)floor(phase);
  frac_part = phase - int_part;
  if (int_part == size) int_part = 0; /* this can actually happen! */
  if (frac_part == 0.0) 
    return(wave[int_part]);
  else
    {
      inx = int_part + 1;
      if (inx >= size) inx = 0;
      return(wave[int_part] + (frac_part * (wave[inx] - wave[int_part])));
    }
}

static Float *array_normalize(Float *table, int table_size)
{
  Float amp = 0.0;
  int i;
  for (i = 0; i < table_size; i++) 
    if (amp < (fabs(table[i]))) 
      amp = fabs(table[i]);
  if ((amp > 0.0) && (amp != 1.0))
    for (i = 0; i < table_size; i++) 
      table[i] /= amp;
  return(table);
}



/* ---------------- oscil ---------------- */

typedef struct {
  mus_any_class *core;
  Float phase, freq;
} osc;

Float mus_oscil(mus_any *ptr, Float fm, Float pm)
{
  Float result;
  osc *gen = (osc *)ptr;
  result = mus_sin(gen->phase + pm);
  gen->phase += (gen->freq + fm);
  if ((gen->phase > 1000.0) || (gen->phase < -1000.0)) gen->phase = fmod(gen->phase, TWO_PI);
  return(result);
}

/* some optimization experiments */
Float mus_oscil_0(mus_any *ptr)
{
  Float result;
  osc *gen = (osc *)ptr;
  result = mus_sin(gen->phase);
  gen->phase += gen->freq;
  if ((gen->phase > 1000.0) || (gen->phase < -1000.0)) gen->phase = fmod(gen->phase, TWO_PI);
  return(result);
}

Float mus_oscil_1(mus_any *ptr, Float fm)
{
  Float result;
  osc *gen = (osc *)ptr;
  result = mus_sin(gen->phase);
  gen->phase += (gen->freq + fm);
  if ((gen->phase > 1000.0) || (gen->phase < -1000.0)) gen->phase = fmod(gen->phase, TWO_PI);
  return(result);
}

Float mus_oscil_bank(Float *amps, mus_any **oscils, Float *inputs, int size)
{
  int i;
  Float sum = 0.0;
  for (i = 0; i < size; i++) 
    sum += (amps[i] * mus_oscil_1(oscils[i], inputs[i]));
  return(sum);
}

int mus_oscil_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_OSCIL));}
static int free_oscil(mus_any *ptr) {if (ptr) FREE(ptr); return(0);}
static Float oscil_freq(mus_any *ptr) {return(mus_radians2hz(((osc *)ptr)->freq));}
static Float set_oscil_freq(mus_any *ptr, Float val) {((osc *)ptr)->freq = mus_hz2radians(val); return(val);}
static Float oscil_phase(mus_any *ptr) {return(fmod(((osc *)ptr)->phase, TWO_PI));}
static Float set_oscil_phase(mus_any *ptr, Float val) {((osc *)ptr)->phase = val; return(val);}
static int oscil_cosines(mus_any *ptr) {return(1);}

static int oscil_equalp(mus_any *p1, mus_any *p2)
{
  return((p1 == p2) ||
	 ((mus_oscil_p((mus_any *)p1)) && 
	  (mus_oscil_p((mus_any *)p2)) &&
	  ((((osc *)p1)->freq) == (((osc *)p2)->freq)) &&
	  ((((osc *)p1)->phase) == (((osc *)p2)->phase))));
}

static char *describe_oscil(mus_any *ptr)
{
  if (mus_oscil_p((mus_any *)ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "oscil freq: %.3fHz, phase: %.3f", mus_radians2hz(((osc *)ptr)->freq), ((osc *)ptr)->phase);
  else describe_bad_gen(ptr, "oscil", "an");
  return(describe_buffer);
}

static char *inspect_oscil(mus_any *ptr)
{
  osc *gen = (osc *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "osc freq: %f, phase: %f", gen->freq, gen->phase);
  return(describe_buffer);
}

static mus_any_class OSCIL_CLASS = {
  MUS_OSCIL,
  "oscil",
  &free_oscil,
  &describe_oscil,
  &inspect_oscil,
  &oscil_equalp,
  0, 0, 0, 0, /* data length */
  &oscil_freq,
  &set_oscil_freq,
  &oscil_phase,
  &set_oscil_phase,
  0, 0, 0, 0,
  &mus_oscil,
  MUS_NOT_SPECIAL, 
  NULL,
  0, 
  0, 0, 0, 0, 0, 0, &oscil_cosines, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_oscil(Float freq, Float phase)
{
  osc *gen;
  gen = (osc *)clm_calloc(1, sizeof(osc), "oscil");
  gen->core = &OSCIL_CLASS;
  gen->freq = mus_hz2radians(freq);
  gen->phase = phase;
  return((mus_any *)gen);
}

/* ---------------- sum-of-cosines ---------------- */

typedef struct {
  mus_any_class *core;
  int cosines;
  Float scaler;
  Float phase;
  Float freq;
} cosp;

Float mus_sum_of_cosines(mus_any *ptr, Float fm)
{
  Float val;
  cosp *gen = (cosp *)ptr;
  if ((gen->phase == 0.0) || (gen->phase == TWO_PI))
    val = 1.0;
  else 
    {
      val = (gen->scaler * mus_sin(gen->phase * (gen->cosines + 0.5))) / mus_sin(gen->phase * 0.5);
      if (val > 1.0) val = 1.0;
    }
  gen->phase += (gen->freq + fm);
  while (gen->phase > TWO_PI) gen->phase -= TWO_PI;
  while (gen->phase < 0.0) gen->phase += TWO_PI;
  return(val);
}

int mus_sum_of_cosines_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_SUM_OF_COSINES));}

static int free_sum_of_cosines(mus_any *ptr) {if (ptr) FREE(ptr); return(0);}
static Float sum_of_cosines_freq(mus_any *ptr) {return(mus_radians2hz(((cosp *)ptr)->freq));}
static Float set_sum_of_cosines_freq(mus_any *ptr, Float val) {((cosp *)ptr)->freq = mus_hz2radians(val); return(val);}
static Float sum_of_cosines_phase(mus_any *ptr) {return(fmod(((cosp *)ptr)->phase, TWO_PI));}
static Float set_sum_of_cosines_phase(mus_any *ptr, Float val) {((cosp *)ptr)->phase = val; return(val);}
static Float sum_of_cosines_scaler(mus_any *ptr) {return(((cosp *)ptr)->scaler);}
static Float set_sum_of_cosines_scaler(mus_any *ptr, Float val) {((cosp *)ptr)->scaler = val; return(val);}
static int sum_of_cosines_cosines(mus_any *ptr) {return(((cosp *)ptr)->cosines);}
static int set_sum_of_cosines_cosines(mus_any *ptr, int val) {((cosp *)ptr)->cosines = val; return(val);}
static Float run_sum_of_cosines(mus_any *ptr, Float fm, Float unused) {return(mus_sum_of_cosines(ptr, fm));}

static int sum_of_cosines_equalp(mus_any *p1, mus_any *p2)
{
  return((p1 == p2) ||
	 ((mus_sum_of_cosines_p((mus_any *)p1)) && (mus_sum_of_cosines_p((mus_any *)p2)) &&
	  ((((cosp *)p1)->freq) == (((cosp *)p2)->freq)) &&
	  ((((cosp *)p1)->phase) == (((cosp *)p2)->phase)) &&
	  ((((cosp *)p1)->cosines) == (((cosp *)p2)->cosines)) &&
	  ((((cosp *)p1)->scaler) == (((cosp *)p2)->scaler))));
}

static char *describe_sum_of_cosines(mus_any *ptr)
{
  if (mus_sum_of_cosines_p((mus_any *)ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "sum_of_cosines freq: %.3fHz, phase: %.3f, cosines: %d",
		 mus_radians2hz(((cosp *)ptr)->freq), 
		 ((cosp *)ptr)->phase, 
		 ((cosp *)ptr)->cosines);
  else describe_bad_gen(ptr, "sum_of_cosines", "a");
  return(describe_buffer);
}

static char *inspect_sum_of_cosines(mus_any *ptr)
{
  cosp *gen = (cosp *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
	       "cosp freq: %f, phase: %f, cosines: %d, scaler: %f", 
	       gen->freq, gen->phase, gen->cosines, gen->scaler);
  return(describe_buffer);
}

static mus_any_class SUM_OF_COSINES_CLASS = {
  MUS_SUM_OF_COSINES,
  "sum_of_cosines",
  &free_sum_of_cosines,
  &describe_sum_of_cosines,
  &inspect_sum_of_cosines,
  &sum_of_cosines_equalp,
  0, 0, /* data */
  &sum_of_cosines_cosines,
  &set_sum_of_cosines_cosines,
  &sum_of_cosines_freq,
  &set_sum_of_cosines_freq,
  &sum_of_cosines_phase,
  &set_sum_of_cosines_phase,
  &sum_of_cosines_scaler,
  &set_sum_of_cosines_scaler,
  0, 0,
  &run_sum_of_cosines,
  MUS_NOT_SPECIAL, 
  NULL,
  0,
  0, 0, 0, 0, 0, 0, 
  &sum_of_cosines_cosines, &set_sum_of_cosines_cosines, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_sum_of_cosines(int cosines, Float freq, Float phase)
{
  cosp *gen;
  gen = (cosp *)clm_calloc(1, sizeof(cosp), "sum_of_cosines");
  gen->core = &SUM_OF_COSINES_CLASS;
  if (cosines == 0) cosines = 1;
  gen->scaler = 1.0 / (Float)(1 + 2 * cosines);
  gen->cosines = cosines;
  gen->freq = mus_hz2radians(freq);
  gen->phase = phase;
  return((mus_any *)gen);
}


/* ---------------- delay, comb, notch, all-pass ---------------- */

typedef struct {
  mus_any_class *core;
  int loc, size, zdly, line_allocated;
  Float *line;
  int zloc, zsize;
  Float xscl, yscl;
} dly;


Float mus_delay(mus_any *ptr, Float input, Float pm)
{
  /* changed 26-Sep-00 to match mus.lisp */
  dly *gen = (dly *)ptr;
  Float result;
  result = mus_tap(ptr, pm);
  gen->line[gen->loc] = input;
  gen->loc++;
  if (gen->zdly)
    {
      if (gen->loc >= gen->zsize) gen->loc = 0;
      gen->zloc++;
      if (gen->zloc >= gen->zsize) gen->zloc = 0;
    }
  else
    {
      if (gen->loc >= gen->size) gen->loc = 0;
    }
  return(result);
}

Float mus_delay_1(mus_any *ptr, Float input)
{
  dly *gen = (dly *)ptr;
  Float result;
  result = gen->line[gen->loc];
  gen->line[gen->loc] = input;
  gen->loc++;
  if (gen->loc >= gen->size) gen->loc = 0;
  return(result);
}

Float mus_tap(mus_any *ptr, Float loc)
{
  dly *gen = (dly *)ptr;
  int taploc;
  if (gen->zdly)
    {
      if (loc == 0.0) 
	return(gen->line[gen->zloc]);
      else return(mus_array_interp(gen->line, gen->zloc - loc, gen->zsize));
    }
  else
    {
      if ((int)loc == 0) 
	return(gen->line[gen->loc]);
      else
	{
	  taploc = (int)(gen->loc - (int)loc) % gen->size;
	  if (taploc < 0) taploc += gen->size;
	  return(gen->line[taploc]);
	}
    }
}

Float mus_tap_1(mus_any *ptr)
{
  dly *gen = (dly *)ptr;
  return(gen->line[gen->loc]);
}

static int free_delay(mus_any *gen) 
{
  dly *ptr = (dly *)gen;
  if (ptr) 
    {
      if ((ptr->line) && (ptr->line_allocated)) FREE(ptr->line);
      FREE(ptr);
    }
  return(0);
}

static char *inspect_delay(mus_any *ptr)
{
  dly *gen = (dly *)ptr;
  char *arr = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "dly line[%d,%d at %d,%d (%s)]: %s, xscl: %f, yscl: %f",
	  gen->size,
	  gen->zsize,
	  gen->loc,
	  gen->zloc,
	  (gen->line_allocated) ? "local" : "external",
	  arr = print_array(gen->line, gen->size, (gen->zdly) ? gen->zloc : gen->loc),
	  gen->xscl,
	  gen->yscl);
  if (arr) FREE(arr);
  return(describe_buffer);
}

static char *describe_delay(mus_any *ptr)
{
  char *str = NULL;
  dly *gen = (dly *)ptr;
  if (mus_delay_p((mus_any *)ptr))
    {
      if (gen->zdly)
	mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		     "delay: line[%d,%d]: %s", 
		     gen->size, gen->zsize, str = print_array(gen->line, gen->size, gen->zloc));
      else mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
			"delay: line[%d]: %s", 
			gen->size, str = print_array(gen->line, gen->size, gen->loc));
      if (str) FREE(str);
    }
  else describe_bad_gen(ptr, "delay", "a");
  return(describe_buffer);
}

static char *describe_comb(mus_any *ptr)
{
  char *str = NULL;
  dly *gen = (dly *)ptr;
  if (mus_comb_p((mus_any *)ptr))
    {
      if (gen->zdly)
	mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		     "comb: scaler: %.3f, line[%d,%d]: %s", 
		     gen->yscl, gen->size, gen->zsize, 
		     str = print_array(gen->line, gen->size, gen->zloc));
      else mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
			"comb: scaler: %.3f, line[%d]: %s", 
			gen->yscl, gen->size, 
			str = print_array(gen->line, gen->size, gen->loc));
      if (str) FREE(str);
    }
  else describe_bad_gen(ptr, "comb", "a");
  return(describe_buffer);
}

static char *describe_notch(mus_any *ptr)
{
  char *str = NULL;
  dly *gen = (dly *)ptr;
  if (mus_notch_p((mus_any *)ptr))
    {
      if (gen->zdly)
	mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		     "notch: scaler: %.3f, line[%d,%d]: %s", 
		     gen->xscl, gen->size, gen->zsize, 
		     str = print_array(gen->line, gen->size, gen->zloc));
      else mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
			"notch: scaler: %.3f, line[%d]: %s", 
			gen->xscl, gen->size, 
			str = print_array(gen->line, gen->size, gen->loc));
      if (str) FREE(str);
    }
  else describe_bad_gen(ptr, "notch", "a");
  return(describe_buffer);
}

static char *describe_all_pass(mus_any *ptr)
{
  char *str = NULL;
  dly *gen = (dly *)ptr;
  if (mus_all_pass_p((mus_any *)ptr))
    {
      if (gen->zdly)
	mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		     "all_pass: feedback: %.3f, feedforward: %.3f, line[%d,%d]:%s",
		     gen->yscl, gen->xscl, gen->size, gen->zsize, 
		     str = print_array(gen->line, gen->size, gen->zloc));
      else mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
			"all_pass: feedback: %.3f, feedforward: %.3f, line[%d]:%s",
			gen->yscl, gen->xscl, gen->size, 
			str = print_array(gen->line, gen->size, gen->loc));
      if (str) FREE(str);
    }
  else describe_bad_gen(ptr, "all_pass", "an");
  return(describe_buffer);
}

static int delay_equalp(mus_any *p1, mus_any *p2)
{
  int i;
  dly *d1 = (dly *)p1;
  dly *d2 = (dly *)p2;
  if (p1 == p2) return(TRUE);
  if ((d1) && (d2) &&
      (d1->core->type == d2->core->type) &&
      (d1->size == d2->size) &&
      (d1->loc == d2->loc) &&
      (d1->zdly == d2->zdly) &&
      (d1->zloc == d2->zloc) &&
      (d1->zsize == d2->zsize) &&
      (d1->xscl == d2->xscl) &&
      (d1->yscl == d2->yscl))
    {
      for (i = 0; i < d1->size; i++)
	if (d1->line[i] != d2->line[i])
	  return(FALSE);
      return(TRUE);
    }
  return(FALSE);
}

static int delay_length(mus_any *ptr) {return(((dly *)ptr)->size);}
static int set_delay_length(mus_any *ptr, int val) {((dly *)ptr)->size = val; return(val);}
static Float *delay_data(mus_any *ptr) {return(((dly *)ptr)->line);}
static Float delay_scaler(mus_any *ptr) {return(((dly *)ptr)->xscl);}
static Float set_delay_scaler(mus_any *ptr, Float val) {((dly *)ptr)->xscl = val; return(val);}
static Float delay_fb(mus_any *ptr) {return(((dly *)ptr)->yscl);}
static Float set_delay_fb(mus_any *ptr, Float val) {((dly *)ptr)->yscl = val; return(val);}

static Float *delay_set_data(mus_any *ptr, Float *val) 
{
  dly *gen = (dly *)ptr;
  if (gen->line_allocated) {FREE(gen->line); gen->line_allocated = FALSE;}
  gen->line = val; 
  return(val);
}

int mus_delay_line_p(mus_any *gen)
{
  return((gen) && ((gen->core)->extended_type == MUS_DELAY_LINE));
}

static mus_any_class DELAY_CLASS = {
  MUS_DELAY,
  "delay",
  &free_delay,
  &describe_delay,
  &inspect_delay,
  &delay_equalp,
  &delay_data,
  &delay_set_data,
  &delay_length,
  &set_delay_length,
  0, 0, 0, 0, 0, 0, /* freq phase scaler */
  0, 0,
  &mus_delay,
  MUS_DELAY_LINE,
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_delay(int size, Float *preloaded_line, int line_size) 
{
  /* if preloaded_line null, allocated locally */
  /* if size == line_size, normal (non-interpolating) delay */
  dly *gen;
  gen = (dly *)clm_calloc(1, sizeof(dly), "delay");
  gen->core = &DELAY_CLASS;
  gen->loc = 0;
  gen->size = size;
  gen->zsize = line_size;
  gen->zdly = (line_size != size);
  if (preloaded_line)
    {
      gen->line = preloaded_line;
      gen->line_allocated = FALSE;
    }
  else 
    {
      gen->line = (Float *)clm_calloc(line_size, sizeof(Float), "delay line");
      gen->line_allocated = TRUE;
    }
  gen->zloc = line_size - size;
  return((mus_any *)gen);
}

int mus_delay_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_DELAY));}

Float mus_comb(mus_any *ptr, Float input, Float pm) 
{
  dly *gen = (dly *)ptr;
  if (gen->zdly)
    return(mus_delay(ptr, input + (gen->yscl * mus_tap(ptr, pm)), pm)); 
  /* mus.lisp has 0 in place of the final pm -- the question is whether the delay
     should interpolate as well as the tap.  There is a subtle difference in
     output (the pm case is low-passed by the interpolation ("average"),
     but I don't know if there's a standard here, or what people expect.
     We're doing the outer-level interpolation in notch and all-pass.
     Should mus.lisp be changed?
  */
  else return(mus_delay(ptr, input + (gen->line[gen->loc] * gen->yscl), 0.0));
}

Float mus_comb_1(mus_any *ptr, Float input) 
{
  dly *gen = (dly *)ptr;
  return(mus_delay_1(ptr, input + (gen->line[gen->loc] * gen->yscl)));
}

static mus_any_class COMB_CLASS = {
  MUS_COMB,
  "comb",
  &free_delay,
  &describe_comb,
  &inspect_delay,
  &delay_equalp,
  &delay_data,
  &delay_set_data,
  &delay_length,
  &set_delay_length,
  0, 0, 0, 0, /* freq phase */
  &delay_scaler,
  &set_delay_scaler,
  &delay_fb,
  &set_delay_fb,
  &mus_comb,
  MUS_DELAY_LINE,
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_comb (Float scaler, int size, Float *line, int line_size)
{
  dly *gen;
  gen = (dly *)mus_make_delay(size, line, line_size);
  if (gen)
    {
      gen->core = &COMB_CLASS;
      gen->yscl = scaler;
      return((mus_any *)gen);
    }
  return(NULL);
}

int mus_comb_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_COMB));}

static mus_any_class NOTCH_CLASS = {
  MUS_NOTCH,
  "notch",
  &free_delay,
  &describe_notch,
  &inspect_delay,
  &delay_equalp,
  &delay_data,
  &delay_set_data,
  &delay_length,
  &set_delay_length,
  0, 0, 0, 0, /* freq phase */
  &delay_scaler,
  &set_delay_scaler,
  0, 0,
  &mus_notch,
  MUS_DELAY_LINE,
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

Float mus_notch(mus_any *ptr, Float input, Float pm) 
{
  dly *gen = (dly *)ptr;
  return((input * gen->xscl) + mus_delay(ptr, input, pm));
}

Float mus_notch_1(mus_any *ptr, Float input) 
{
  return((input * ((dly *)ptr)->xscl) + mus_delay_1(ptr, input));
}

int mus_notch_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_NOTCH));}

mus_any *mus_make_notch (Float scaler, int size, Float *line, int line_size)
{
  dly *gen;
  gen = (dly *)mus_make_delay(size, line, line_size);
  if (gen)
    {
      gen->core = &NOTCH_CLASS;
      gen->xscl = scaler;
      return((mus_any *)gen);
    }
  return(NULL);
}

Float mus_all_pass(mus_any *ptr, Float input, Float pm)
{
  Float din;
  dly *gen = (dly *)ptr;
  if (gen->zdly)
    din = input + (gen->yscl * mus_tap(ptr, pm));
  else din = input + (gen->yscl * gen->line[gen->loc]);
  return(mus_delay(ptr, din, pm) + (gen->xscl * din));
}

Float mus_all_pass_1(mus_any *ptr, Float input)
{
  Float din;
  dly *gen = (dly *)ptr;
  din = input + (gen->yscl * gen->line[gen->loc]);
  return(mus_delay_1(ptr, din) + (gen->xscl * din));
}

int mus_all_pass_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_ALL_PASS));}

static mus_any_class ALL_PASS_CLASS = {
  MUS_ALL_PASS,
  "all_pass",
  &free_delay,
  &describe_all_pass,
  &inspect_delay,
  &delay_equalp,
  &delay_data,
  &delay_set_data,
  &delay_length,
  &set_delay_length,
  0, 0, 0, 0, /* freq phase */
  &delay_scaler,
  &set_delay_scaler,
  &delay_fb,
  &set_delay_fb,
  &mus_all_pass,
  MUS_DELAY_LINE,
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_all_pass (Float backward, Float forward, int size, Float *line, int line_size)
{
  dly *gen;
  gen = (dly *)mus_make_delay(size, line, line_size);
  if (gen)
    {
      gen->core = &ALL_PASS_CLASS;
      gen->xscl = forward;
      gen->yscl = backward;
      return((mus_any *)gen);
    }
  return(NULL);
}



/* ---------------- table lookup ---------------- */

typedef struct {
  mus_any_class *core;
  Float freq;
  Float internal_mag;
  Float phase;
  Float *table;
  int table_size, table_allocated;
} tbl;

Float *mus_partials2wave(Float *partial_data, int partials, Float *table, int table_size, int normalize)
{
  int partial, i, k;
  Float amp, freq, angle;
  memset((void *)table, 0, table_size * sizeof(Float));
  for (partial = 0, k = 1; partial < partials; partial++, k += 2)
    {
      amp = partial_data[k];
      if (amp != 0.0)
	{
	  freq = (partial_data[partial * 2] * TWO_PI) / (Float)table_size;
	  for (i = 0, angle = 0.0; i < table_size; i++, angle += freq) 
	    table[i] += amp * mus_sin(angle);
	}
    }
  if (normalize) 
    return(array_normalize(table, table_size));
  return(table);
}

Float *mus_phasepartials2wave(Float *partial_data, int partials, Float *table, int table_size, int normalize)
{
  int partial, i, k, n;
  Float amp, freq, angle; 
  memset((void *)table, 0, table_size * sizeof(Float));
  for (partial = 0, k = 1, n = 2; partial < partials; partial++, k += 3, n += 3)
    {
      amp = partial_data[k];
      if (amp != 0.0)
	{
	  freq = (partial_data[partial * 3] * TWO_PI) / (Float)table_size;
	  for (i = 0, angle = partial_data[n]; i < table_size; i++, angle += freq) 
	    table[i] += amp * mus_sin(angle);
	}
    }
  if (normalize) 
    return(array_normalize(table, table_size));
  return(table);
}

Float mus_table_lookup(mus_any *ptr, Float fm)
{
  tbl *gen = (tbl *)ptr;
  Float result;
  result = mus_array_interp(gen->table, gen->phase, gen->table_size);
  gen->phase += (gen->freq + (fm * gen->internal_mag));
  while (gen->phase >= gen->table_size) gen->phase -= gen->table_size;
  while (gen->phase < 0.0) gen->phase += gen->table_size;
  return(result);
}

Float mus_table_lookup_1(mus_any *ptr)
{
  tbl *gen = (tbl *)ptr;
  Float result;
  result = mus_array_interp(gen->table, gen->phase, gen->table_size);
  gen->phase += gen->freq;
  while (gen->phase >= gen->table_size) gen->phase -= gen->table_size;
  while (gen->phase < 0.0) gen->phase += gen->table_size;
  return(result);
}

static Float run_table_lookup(mus_any *ptr, Float fm, Float unused) {return(mus_table_lookup(ptr, fm));}
int mus_table_lookup_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_TABLE_LOOKUP));}
static int table_lookup_length(mus_any *ptr) {return(((tbl *)ptr)->table_size);}
static Float *table_lookup_data(mus_any *ptr) {return(((tbl *)ptr)->table);}
static Float table_lookup_freq(mus_any *ptr) {return((((tbl *)ptr)->freq * sampling_rate) / (Float)(((tbl *)ptr)->table_size));}
static Float set_table_lookup_freq(mus_any *ptr, Float val) {((tbl *)ptr)->freq = (val * ((tbl *)ptr)->table_size) / sampling_rate; return(val);}
static Float table_lookup_phase(mus_any *ptr) {return(fmod(((TWO_PI * ((tbl *)ptr)->phase) / ((tbl *)ptr)->table_size), TWO_PI));}
static Float set_table_lookup_phase(mus_any *ptr, Float val) {((tbl *)ptr)->phase = (val * ((tbl *)ptr)->table_size) / TWO_PI; return(val);}

static char *describe_table_lookup(mus_any *ptr)
{
  tbl *gen = (tbl *)ptr;
  if (mus_table_lookup_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "table_lookup: freq: %.3fHz, phase: %.3f, length: %d",
		 gen->freq * sampling_rate / gen->table_size, 
		 gen->phase, gen->table_size);
  else describe_bad_gen(ptr, "table_lookup", "a");
  return(describe_buffer);
}

static char *inspect_table_lookup(mus_any *ptr)
{
  tbl *gen = (tbl *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "tbl freq: %f, phase: %f, length: %d, mag: %f, table: %s",
	       gen->freq, gen->phase, gen->table_size, gen->internal_mag, 
	       (gen->table_allocated) ? "local" : "external");
  return(describe_buffer);
}

static int table_lookup_equalp(mus_any *p1, mus_any *p2)
{
  int i;
  tbl *t1 = (tbl *)p1;
  tbl *t2 = (tbl *)p2;
  if (p1 == p2) return(TRUE);
  if ((t1) && (t2) &&
      (t1->core->type == t2->core->type) &&
      (t1->table_size == t2->table_size) &&
      (t1->freq == t2->freq) &&
      (t1->phase == t2->phase) &&
      (t1->internal_mag == t2->internal_mag))
    {
      for (i = 0; i < t1->table_size; i++)
	if (t1->table[i] != t2->table[i])
	  return(FALSE);
      return(TRUE);
    }
  return(FALSE);
}

static int free_table_lookup(mus_any *ptr) 
{
  tbl *gen = (tbl *)ptr;
  if (gen)
    {
      if ((gen->table) && (gen->table_allocated)) FREE(gen->table); 
      FREE(gen); 
    }
  return(0);
}

static Float *table_set_data(mus_any *ptr, Float *val) 
{
  tbl *gen = (tbl *)ptr;
  if (gen->table_allocated) {FREE(gen->table); gen->table_allocated = FALSE;}
  gen->table = val; 
  return(val);
}

static mus_any_class TABLE_LOOKUP_CLASS = {
  MUS_TABLE_LOOKUP,
  "table_lookup",
  &free_table_lookup,
  &describe_table_lookup,
  &inspect_table_lookup,
  &table_lookup_equalp,
  &table_lookup_data,
  &table_set_data,
  &table_lookup_length,
  0,
  &table_lookup_freq,
  &set_table_lookup_freq,
  &table_lookup_phase,
  &set_table_lookup_phase,
  0, 0,
  0, 0,
  &run_table_lookup,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_table_lookup (Float freq, Float phase, Float *table, int table_size)
{
  tbl *gen;
  gen = (tbl *)clm_calloc(1, sizeof(tbl), "table_lookup");
  gen->core = &TABLE_LOOKUP_CLASS;
  gen->table_size = table_size;
  gen->internal_mag = (Float)table_size / TWO_PI;
  gen->freq = (freq * table_size) / sampling_rate;
  gen->phase = (phase * table_size) / TWO_PI;
  if (table)
    {
      gen->table = table;
      gen->table_allocated = FALSE;
    }
  else
    {
      gen->table = (Float *)clm_calloc(table_size, sizeof(Float), "table lookup table");
      gen->table_allocated = TRUE;
    }
  return((mus_any *)gen);
}



/* ---------------- sawtooth et al ---------------- */

typedef struct {
  mus_any_class *core;
  Float current_value;
  Float freq;
  Float phase;
  Float base;
  Float width;
} sw;

static char *inspect_sw(mus_any *ptr)
{
  sw *gen = (sw *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
	       "sw current_value: %f, freq: %f, phase: %f, base: %f", 
	       gen->current_value, gen->freq, gen->phase, gen->base);
  return(describe_buffer);
}

static int free_sw(mus_any *ptr) 
{
  sw *gen = (sw *)ptr;
  if (gen) FREE(gen);
  return(0);
}

Float mus_sawtooth_wave(mus_any *ptr, Float fm)
{
  sw *gen = (sw *)ptr;
  Float result;
  result = gen->current_value;
  gen->phase += (gen->freq + fm);
  while (gen->phase >= TWO_PI) gen->phase -= TWO_PI;
  while (gen->phase < 0.0) gen->phase += TWO_PI;
  gen->current_value = gen->base * (gen->phase - M_PI);
  return(result);
}

static Float run_sawtooth_wave(mus_any *ptr, Float fm, Float unused) {return(mus_sawtooth_wave(ptr, fm));}
int mus_sawtooth_wave_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_SAWTOOTH_WAVE));}
static Float sw_freq(mus_any *ptr) {return(mus_radians2hz(((sw *)ptr)->freq));}
static Float sw_phase(mus_any *ptr) {return(fmod(((sw *)ptr)->phase, TWO_PI));}
static Float set_sw_freq(mus_any *ptr, Float val) {((sw *)ptr)->freq = mus_hz2radians(val); return(val);}
static Float set_sw_phase(mus_any *ptr, Float val) {((sw *)ptr)->phase = val; return(val);}
static Float sw_width(mus_any *ptr) {return((((sw *)ptr)->width) / ( 2 * M_PI));}
static Float sw_set_width(mus_any *ptr, Float val) {((sw *)ptr)->width = (2 * M_PI * val); return(val);}

static Float sawtooth_scaler(mus_any *ptr) {return(((sw *)ptr)->base * M_PI);}
static Float set_sawtooth_scaler(mus_any *ptr, Float val) {((sw *)ptr)->base = val / M_PI; return(val);}

static int sw_equalp(mus_any *p1, mus_any *p2)
{
  sw *s1, *s2;
  s1 = (sw *)p1;
  s2 = (sw *)p2;
  return((p1 == p2) ||
	 ((s1) && (s2) &&
	  (s1->core->type == s2->core->type) &&
	  (s1->freq == s2->freq) &&
	  (s1->phase == s2->phase) &&
	  (s1->base == s2->base) &&
	  (s1->current_value == s2->current_value)));
}

static char *describe_sawtooth(mus_any *ptr)
{
  if (mus_sawtooth_wave_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "sawtooth freq: %.3fHz, phase: %.3f, amp: %.3f",
		 mus_radians2hz(((sw *)ptr)->freq), 
		 ((sw *)ptr)->phase, 
		 ((sw *)ptr)->base * M_PI);
  else describe_bad_gen(ptr, "sawtooth_wave", "a");
  return(describe_buffer);
}

static mus_any_class SAWTOOTH_WAVE_CLASS = {
  MUS_SAWTOOTH_WAVE,
  "sawtooth_wave",
  &free_sw,
  &describe_sawtooth,
  &inspect_sw,
  &sw_equalp,
  0, 0, 0, 0,
  &sw_freq,
  &set_sw_freq,
  &sw_phase,
  &set_sw_phase,
  &sawtooth_scaler,
  &set_sawtooth_scaler,
  0, 0,
  &run_sawtooth_wave,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_sawtooth_wave(Float freq, Float amp, Float phase) /* M_PI as initial phase, normally */
{
  sw *gen;
  gen = (sw *)clm_calloc(1, sizeof(sw), "sawtooth_wave");
  gen->core = &SAWTOOTH_WAVE_CLASS;
  gen->freq = mus_hz2radians(freq);
  gen->base = (amp / M_PI);
  gen->phase = phase;
  gen->current_value = gen->base * (gen->phase - M_PI);
  return((mus_any *)gen);
}

Float mus_square_wave(mus_any *ptr, Float fm)
{
  sw *gen = (sw *)ptr;
  Float result;
  result = gen->current_value;
  gen->phase += (gen->freq + fm);
  while (gen->phase >= TWO_PI) gen->phase -= TWO_PI;
  while (gen->phase < 0.0) gen->phase += TWO_PI;
  if (gen->phase < gen->width) gen->current_value = gen->base; else gen->current_value = 0.0;
  return(result);
}

int mus_square_wave_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_SQUARE_WAVE));}

static char *describe_square_wave(mus_any *ptr)
{
  if (mus_square_wave_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "square_wave freq: %.3fHz, phase: %.3f, amp: %.3f",
		 mus_radians2hz(((sw *)ptr)->freq), 
		 ((sw *)ptr)->phase,
		 ((sw *)ptr)->base);
  else describe_bad_gen(ptr, "square_wave", "a");
  return(describe_buffer);
}

static Float run_square_wave(mus_any *ptr, Float fm, Float unused) {return(mus_square_wave(ptr, fm));}
static Float square_wave_scaler(mus_any *ptr) {return(((sw *)ptr)->base);}
static Float set_square_wave_scaler(mus_any *ptr, Float val) {((sw *)ptr)->base = val; return(val);}

static mus_any_class SQUARE_WAVE_CLASS = {
  MUS_SQUARE_WAVE,
  "square_wave",
  &free_sw,
  &describe_square_wave,
  &inspect_sw,
  &sw_equalp,
  0, 0, 0, 0,
  &sw_freq,
  &set_sw_freq,
  &sw_phase,
  &set_sw_phase,
  &square_wave_scaler,
  &set_square_wave_scaler,
  0, 0,
  &run_square_wave,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 
  &sw_width, &sw_set_width, 
  0, 0, 
  0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_square_wave(Float freq, Float amp, Float phase)
{
  sw *gen;
  gen = (sw *)clm_calloc(1, sizeof(sw), "square_wave");
  gen->core = &SQUARE_WAVE_CLASS;
  gen->freq = mus_hz2radians(freq);
  gen->base = amp;
  gen->phase = phase;
  gen->width = M_PI;
  if (gen->phase < gen->width) 
    gen->current_value = gen->base; 
  else gen->current_value = 0.0;
  return((mus_any *)gen);
}

Float mus_triangle_wave(mus_any *ptr, Float fm)
{
  sw *gen = (sw *)ptr;
  Float result;
  result = gen->current_value;
  gen->phase += (gen->freq + fm);
  while (gen->phase >= TWO_PI) gen->phase -= TWO_PI;
  while (gen->phase < 0.0) gen->phase += TWO_PI;
  if (gen->phase < (M_PI / 2.0)) 
    gen->current_value = gen->base * gen->phase;
  else
    if (gen->phase < (M_PI * 1.5)) 
      gen->current_value = gen->base * (M_PI - gen->phase);
    else gen->current_value = gen->base * (gen->phase - TWO_PI);
  return(result);
}

int mus_triangle_wave_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_TRIANGLE_WAVE));}

static Float run_triangle_wave(mus_any *ptr, Float fm, Float unused) {return(mus_triangle_wave(ptr, fm));}
static Float triangle_wave_scaler(mus_any *ptr) {return(((sw *)ptr)->base * M_PI_2);}
static Float set_triangle_wave_scaler(mus_any *ptr, Float val) {((sw *)ptr)->base = (val * 2.0 / M_PI); return(val);}

static char *describe_triangle_wave(mus_any *ptr)
{
  if (mus_triangle_wave_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "triangle_wave freq: %.3fHz, phase: %.3f, amp: %.3f",
		 mus_radians2hz(((sw *)ptr)->freq), 
		 ((sw *)ptr)->phase, 
		 ((sw *)ptr)->base * M_PI_2);
  else describe_bad_gen(ptr, "triangle_wave", "a");
  return(describe_buffer);
}

static mus_any_class TRIANGLE_WAVE_CLASS = {
  MUS_TRIANGLE_WAVE,
  "triangle_wave",
  &free_sw,
  &describe_triangle_wave,
  &inspect_sw,
  &sw_equalp,
  0, 0, 0, 0,
  &sw_freq,
  &set_sw_freq,
  &sw_phase,
  &set_sw_phase,
  &triangle_wave_scaler,
  &set_triangle_wave_scaler,
  0, 0,
  &run_triangle_wave,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_triangle_wave(Float freq, Float amp, Float phase)
{
  sw *gen;
  gen = (sw *)clm_calloc(1, sizeof(sw), "triangle_wave");
  gen->core = &TRIANGLE_WAVE_CLASS;
  gen->freq = mus_hz2radians(freq);
  gen->base = (2.0 * amp / M_PI);
  gen->phase = phase;
  if (gen->phase < M_PI_2) 
    gen->current_value = gen->base * gen->phase;
  else
    if (gen->phase < (M_PI * 1.5)) 
      gen->current_value = gen->base * (M_PI - gen->phase);
    else gen->current_value = gen->base * (gen->phase - TWO_PI);
  return((mus_any *)gen);
}

Float mus_pulse_train(mus_any *ptr, Float fm)
{
  sw *gen = (sw *)ptr;
  if (fabs(gen->phase) >= TWO_PI)
    {
      while (gen->phase >= TWO_PI) gen->phase -= TWO_PI;
      while (gen->phase < 0.0) gen->phase += TWO_PI;
      gen->current_value = gen->base;
    }
  else gen->current_value = 0.0;
  gen->phase += (gen->freq + fm);
  return(gen->current_value);
}

int mus_pulse_train_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_PULSE_TRAIN));}

static Float run_pulse_train(mus_any *ptr, Float fm, Float unused) {return(mus_pulse_train(ptr, fm));}
static Float pulse_train_scaler(mus_any *ptr) {return(((sw *)ptr)->base);}
static Float set_pulse_train_scaler(mus_any *ptr, Float val) {((sw *)ptr)->base = val; return(val);}

static char *describe_pulse_train(mus_any *ptr)
{
  if (mus_pulse_train_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "pulse_train freq: %.3fHz, phase: %.3f, amp: %.3f",
		 mus_radians2hz(((sw *)ptr)->freq), 
		 ((sw *)ptr)->phase, 
		 ((sw *)ptr)->base);
  else describe_bad_gen(ptr, "pulse_train", "a");
  return(describe_buffer);
}

static mus_any_class PULSE_TRAIN_CLASS = {
  MUS_PULSE_TRAIN,
  "pulse_train",
  &free_sw,
  &describe_pulse_train,
  &inspect_sw,
  &sw_equalp,
  0, 0, 0, 0,
  &sw_freq,
  &set_sw_freq,
  &sw_phase,
  &set_sw_phase,
  &pulse_train_scaler,
  &set_pulse_train_scaler,
  0, 0,
  &run_pulse_train,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_pulse_train(Float freq, Float amp, Float phase) /* TWO_PI initial phase, normally */
{
  sw *gen;
  gen = (sw *)clm_calloc(1, sizeof(sw), "pulse_train");
  gen->core = &PULSE_TRAIN_CLASS;
  gen->freq = mus_hz2radians(freq);
  gen->base = amp;
  gen->phase = phase;
  gen->current_value = 0.0;
  return((mus_any *)gen);
}



/* ---------------- rand, rand_interp ---------------- */

typedef struct {
  mus_any_class *core;
  Float freq;
  Float base;
  Float phase;
  Float output;
  Float incr;
} noi;

static char *inspect_noi(mus_any *ptr)
{
  noi *gen = (noi *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
	       "noi freq: %f, phase: %f, base: %f, output: %f, incr: %f",
	       gen->freq, gen->phase, gen->base, gen->output, gen->incr);
  return(describe_buffer);
}

/* rand taken from the ANSI C standard (essentially the same as the Cmix form used earlier) */
static unsigned long randx = 1;
#define INVERSE_MAX_RAND  0.0000610351563
#define INVERSE_MAX_RAND2 0.000030517579

void mus_set_rand_seed(unsigned long val) {randx = val;}
unsigned long mus_rand_seed(void) {return(randx);}

static Float next_random(void)
{
  randx = randx * 1103515245 + 12345;
  return((Float)((unsigned int)(randx >> 16) & 32767));
}

Float mus_random(Float amp) /* -amp to amp as Float */
{
  return(amp * (next_random() * INVERSE_MAX_RAND - 1.0));
}

Float mus_frandom(Float amp) /* 0.0 to amp as Float */
{
  return(amp * next_random() * INVERSE_MAX_RAND2);
}

int mus_irandom(int amp)
{
  return((int)(amp * next_random() * INVERSE_MAX_RAND2));
}

static int irandom(int amp) /* original form -- surely this off by a factor of 2? */
{
  return((int)(amp * next_random() * INVERSE_MAX_RAND));
}

Float mus_rand(mus_any *ptr, Float fm)
{
  noi *gen = (noi *)ptr;
  if (gen->phase >= TWO_PI)
    {
      while (gen->phase >= TWO_PI) gen->phase -= TWO_PI;
      gen->output = mus_random(gen->base);
    }
  gen->phase += (gen->freq + fm);
  while (gen->phase < 0.0) gen->phase += TWO_PI;
  return(gen->output);
}

Float mus_rand_interp(mus_any *ptr, Float fm)
{
  noi *gen = (noi *)ptr;
  gen->output += gen->incr;
  if (gen->phase >= TWO_PI)
    {
      while (gen->phase >= TWO_PI) gen->phase -= TWO_PI;
      gen->incr = (mus_random(gen->base) - gen->output) * (gen->freq + fm) / TWO_PI;
    }
  gen->phase += (gen->freq + fm);
  while (gen->phase < 0.0) gen->phase += TWO_PI;
  return(gen->output);
}

static Float run_rand(mus_any *ptr, Float fm, Float unused) {return(mus_rand(ptr, fm));}
static Float run_rand_interp(mus_any *ptr, Float fm, Float unused) {return(mus_rand_interp(ptr, fm));}
int mus_rand_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_RAND));}
int mus_rand_interp_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_RAND_INTERP));}


static int free_noi(mus_any *ptr) {if (ptr) FREE(ptr); return(0);}
static Float noi_freq(mus_any *ptr) {return(mus_radians2hz(((noi *)ptr)->freq));}
static Float set_noi_freq(mus_any *ptr, Float val) {((noi *)ptr)->freq = mus_hz2radians(val); return(val);}
static Float noi_phase(mus_any *ptr) {return(fmod(((noi *)ptr)->phase, TWO_PI));}
static Float set_noi_phase(mus_any *ptr, Float val) {((noi *)ptr)->phase = val; return(val);}
static Float noi_scaler(mus_any *ptr) {return(((noi *)ptr)->base);}
static Float set_noi_scaler(mus_any *ptr, Float val) {((noi *)ptr)->base = val; return(val);}

static int noi_equalp(mus_any *p1, mus_any *p2)
{
  noi *g1 = (noi *)p1;
  noi *g2 = (noi *)p2;
  return((p1 == p2) ||
	 ((g1) && (g2) &&
	  (g1->core->type == g2->core->type) &&
	  (g1->freq == g2->freq) &&
	  (g1->phase == g2->phase) &&
	  (g1->output == g2->output) &&
	  (g1->incr == g2->incr) &&
	  (g1->base == g2->base)));
}

static char *describe_noi(mus_any *ptr)
{
  if (mus_rand_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "rand freq: %.3fHz, phase: %.3f, amp: %.3f",
		 mus_radians2hz(((noi *)ptr)->freq), 
		 ((noi *)ptr)->phase, 
		 ((noi *)ptr)->base);
  else
    {
      if (mus_rand_interp_p(ptr))
	mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		     "rand_interp freq: %.3fHz, phase: %.3f, base: %.3f, incr: %.3f",
		     mus_radians2hz(((noi *)ptr)->freq),
		     ((noi *)ptr)->phase, 
		     ((noi *)ptr)->base, 
		     ((noi *)ptr)->incr);
      else describe_bad_gen(ptr, "rand", "a");
    }
  return(describe_buffer);
}

static mus_any_class RAND_INTERP_CLASS = {
  MUS_RAND_INTERP,
  "rand_interp",
  &free_noi,
  &describe_noi,
  &inspect_noi,
  &noi_equalp,
  0, 0, 0, 0,
  &noi_freq,
  &set_noi_freq,
  &noi_phase,
  &set_noi_phase,
  &noi_scaler,
  &set_noi_scaler,
  0, 0,
  &run_rand_interp,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_rand_interp(Float freq, Float base)
{
  noi *gen;
  gen = (noi *)clm_calloc(1, sizeof(noi), "rand_interp");
  gen->core = &RAND_INTERP_CLASS;
  gen->freq = mus_hz2radians(freq);
  gen->base = base;
  gen->output = 0.0;
  gen->incr =  mus_random(base) * freq / sampling_rate;
  return((mus_any *)gen);
}

static mus_any_class RAND_CLASS = {
  MUS_RAND,
  "rand",
  &free_noi,
  &describe_noi,
  &inspect_noi,
  &noi_equalp,
  0, 0, 0, 0,
  &noi_freq,
  &set_noi_freq,
  &noi_phase,
  &set_noi_phase,
  &noi_scaler,
  &set_noi_scaler,
  0, 0,
  &run_rand,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_rand(Float freq, Float base)
{
  noi *gen;
  gen = (noi *)clm_calloc(1, sizeof(noi), "rand");
  gen->core = &RAND_CLASS;
  gen->freq = mus_hz2radians(freq);
  gen->base = base;
  gen->incr = 0.0;
  gen->output = 0.0;
  return((mus_any *)gen);
}


/* ---------------- asymmetric-fm ---------------- */

typedef struct {
  mus_any_class *core;
  Float r;
  Float freq;
  Float ratio;
  Float phase;
  Float cosr;
  Float sinr;
} asyfm;

static char *inspect_asyfm (mus_any *ptr)
{
  asyfm *gen = (asyfm *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
	       "asyfm r: %f, freq: %f, phase: %f, ratio: %f, cosr: %f, sinr: %f",
	       gen->r, gen->freq, gen->phase, gen->ratio, gen->cosr, gen->sinr);
  return(describe_buffer);
}

static int free_asymmetric_fm(mus_any *ptr) {if (ptr) FREE(ptr); return(0);}
static Float asyfm_freq(mus_any *ptr) {return(mus_radians2hz(((asyfm *)ptr)->freq));}
static Float set_asyfm_freq(mus_any *ptr, Float val) {((asyfm *)ptr)->freq = mus_hz2radians(val); return(val);}
static Float asyfm_phase(mus_any *ptr) {return(fmod(((asyfm *)ptr)->phase, TWO_PI));}
static Float set_asyfm_phase(mus_any *ptr, Float val) {((asyfm *)ptr)->phase = val; return(val);}
static Float asyfm_r(mus_any *ptr) {return(((asyfm *)ptr)->r);}
static Float set_asyfm_r(mus_any *ptr, Float val) 
{
  asyfm *gen = (asyfm *)ptr;
  if (val != 0.0)
    {
      gen->r = val; 
      gen->cosr = 0.5 * (val - (1.0 / val));
      gen->sinr = 0.5 * (val + (1.0 / val));
    }
  return(val);
}

static int asyfm_equalp(mus_any *p1, mus_any *p2)
{
  return((p1 == p2) ||
	 (((p1->core)->type == (p2->core)->type) &&
	  ((((asyfm *)p1)->freq) == (((asyfm *)p2)->freq)) && 
	  ((((asyfm *)p1)->phase) == (((asyfm *)p2)->phase)) &&
	  ((((asyfm *)p1)->ratio) == (((asyfm *)p2)->ratio)) &&
	  ((((asyfm *)p1)->r) == (((asyfm *)p2)->r))));
}

static char *describe_asyfm(mus_any *ptr)
{
  if (mus_asymmetric_fm_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "asymmetric-fm freq: %.3fHz, phase: %.3f, ratio: %.3f, r: %.3f",
		 mus_radians2hz(((asyfm *)ptr)->freq), 
		 ((asyfm *)ptr)->phase, 
		 ((asyfm *)ptr)->ratio, 
		 ((asyfm *)ptr)->r);
  else describe_bad_gen(ptr, "asymmetric_fm", "an");
  return(describe_buffer);
}

int mus_asymmetric_fm_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_ASYMMETRIC_FM));}

Float mus_asymmetric_fm(mus_any *ptr, Float index, Float fm)
{
  asyfm *gen = (asyfm *)ptr;
  Float result, mth;
  mth = gen->ratio * gen->phase;
  result = exp(index * gen->cosr * cos(mth)) * mus_sin(gen->phase + index * gen->sinr * mus_sin(mth));
  /* second index factor added 4-Mar-02 */
  gen->phase += (gen->freq + fm);
  if ((gen->phase > 100.0) || (gen->phase < -100.0)) gen->phase = fmod(gen->phase, TWO_PI);
  return(result);
}

Float mus_asymmetric_fm_1(mus_any *ptr, Float index) /* mostly for internal optimizer consistency */
{
  asyfm *gen = (asyfm *)ptr;
  Float result, mth;
  mth = gen->ratio * gen->phase;
  result = exp(index * gen->cosr * cos(mth)) * mus_sin(gen->phase + index * gen->sinr * mus_sin(mth));
  /* second index factor added 4-Mar-02 */
  gen->phase += gen->freq;
  if ((gen->phase > 100.0) || (gen->phase < -100.0)) gen->phase = fmod(gen->phase, TWO_PI);
  return(result);
}

static mus_any_class ASYMMETRIC_FM_CLASS = {
  MUS_ASYMMETRIC_FM,
  "asymmetric_fm",
  &free_asymmetric_fm,
  &describe_asyfm,
  &inspect_asyfm,
  &asyfm_equalp,
  0, 0, 0, 0,
  &asyfm_freq,
  &set_asyfm_freq,
  &asyfm_phase,
  &set_asyfm_phase,
  &asyfm_r,
  &set_asyfm_r,
  0, 0,
  &mus_asymmetric_fm,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_asymmetric_fm(Float freq, Float phase, Float r, Float ratio) /* r default 1.0, ratio 1.0 */
{
 asyfm *gen = NULL;
 if (r == 0.0)
   mus_error(MUS_ARG_OUT_OF_RANGE, "r can't be 0.0");
 else
   {
     gen = (asyfm *)clm_calloc(1, sizeof(asyfm), "asymmetric_fm");
     gen->core = &ASYMMETRIC_FM_CLASS;
     gen->freq = mus_hz2radians(freq);
     gen->phase = phase;
     gen->r = r;
     gen->ratio = ratio;
     gen->cosr = 0.5 * (r - (1.0 / r)); /* 0.5 factor for I/2 */
     gen->sinr = 0.5 * (r + (1.0 / r));
   }
 return((mus_any *)gen);
}


/* ---------------- simple filters ---------------- */

typedef struct {
  mus_any_class *core;
  Float a0;
  Float a1;
  Float a2;
  Float b1;
  Float b2;
  Float x1;
  Float x2;
  Float y1;
  Float y2;
  Float gain, radius, frequency;
} smpflt;

static char *inspect_smpflt(mus_any *ptr)
{
  smpflt *gen = (smpflt *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
	       "smpflt a0: %f, a1: %f, a2: %f, b1: %f, b2: %f, x1: %f, x2: %f, y1: %f, y2: %f",
	       gen->a0, gen->a1, gen->a2, gen->b1, gen->b2, gen->x1, gen->x2, gen->y1, gen->y2);
  return(describe_buffer);
}

static int free_smpflt(mus_any *ptr) {if (ptr) FREE(ptr); return(0);}

static int smpflt_equalp(mus_any *p1, mus_any *p2)
{
  smpflt *g1 = (smpflt *)p1;
  smpflt *g2 = (smpflt *)p2;
  return((p1 == p2) ||
	 ((g1->core->type == g2->core->type) &&
	  (g1->a0 == g2->a0) &&
	  (g1->a1 == g2->a1) &&
	  (g1->a2 == g2->a2) &&
	  (g1->b1 == g2->b1) &&
	  (g1->b2 == g2->b2) &&
	  (g1->x1 == g2->x1) &&
	  (g1->x2 == g2->x2) &&
	  (g1->y1 == g2->y1) &&
	  (g1->y2 == g2->y2)));
}

static char *describe_smpflt(mus_any *ptr)
{
  smpflt *gen = (smpflt *)ptr;
  switch ((gen->core)->type)
    {
    case MUS_ONE_ZERO: 
      mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "one_zero: a0: %.3f, a1: %.3f, x1: %.3f", 
		   gen->a0, gen->a1, gen->x1); 
      break;
    case MUS_ONE_POLE: 
      mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "one_pole: a0: %.3f, b1: %.3f, y1: %.3f", 
		   gen->a0, gen->b1, gen->y1); 
      break;
    case MUS_TWO_ZERO: 
      mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "two_zero: a0: %.3f, a1: %.3f, a2: %.3f, x1: %.3f, x2: %.3f",
		   gen->a0, gen->a1, gen->a2, gen->x1, gen->x2); 
      break;
    case MUS_TWO_POLE: 
      mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "two_pole: a0: %.3f, b1: %.3f, b2: %.3f, y1: %.3f, y2: %.3f",
		   gen->a0, gen->b1, gen->b2, gen->y1, gen->y2); 
      break;
    default: describe_bad_gen(ptr, "simple filter", "a");
    }
  return(describe_buffer);
}

Float mus_one_zero(mus_any *ptr, Float input)
{
  smpflt *gen = (smpflt *)ptr;
  Float result;
  result = (gen->a0 * input) + (gen->a1 * gen->x1);
  gen->x1 = input;
  return(result);
}

static Float run_one_zero(mus_any *ptr, Float input, Float unused) {return(mus_one_zero(ptr, input));}
static int one_length(mus_any *ptr) {return(1);}
static int two_length(mus_any *ptr) {return(2);}
static Float smp_a0(mus_any *ptr) {return(((smpflt *)ptr)->a0);}
static Float smp_set_a0(mus_any *ptr, Float val) {((smpflt *)ptr)->a0 = val; return(val);}
static Float smp_a1(mus_any *ptr) {return(((smpflt *)ptr)->a1);}
static Float smp_set_a1(mus_any *ptr, Float val) {((smpflt *)ptr)->a1 = val; return(val);}
static Float smp_a2(mus_any *ptr) {return(((smpflt *)ptr)->a2);}
static Float smp_set_a2(mus_any *ptr, Float val) {((smpflt *)ptr)->a2 = val; return(val);}
static Float smp_b1(mus_any *ptr) {return(((smpflt *)ptr)->b1);}
static Float smp_set_b1(mus_any *ptr, Float val) {((smpflt *)ptr)->b1 = val; return(val);}
static Float smp_b2(mus_any *ptr) {return(((smpflt *)ptr)->b2);}
static Float smp_set_b2(mus_any *ptr, Float val) {((smpflt *)ptr)->b2 = val; return(val);}

static mus_any_class ONE_ZERO_CLASS = {
  MUS_ONE_ZERO,
  "one_zero",
  &free_smpflt,
  &describe_smpflt,
  &inspect_smpflt,
  &smpflt_equalp,
  0, 0,
  &one_length, 0,
  0, 0, 0, 0,
  &smp_a0,
  &smp_set_a0,
  0, 0,
  &run_one_zero,
  MUS_SIMPLE_FILTER, 
  NULL, 0,
  &smp_a1, &smp_set_a1, 
  0, 0,
  0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_one_zero(Float a0, Float a1)
{
  smpflt *gen;
  gen = (smpflt *)clm_calloc(1, sizeof(smpflt), "one_zero");
  gen->core = &ONE_ZERO_CLASS;
  gen->a0 = a0;
  gen->a1 = a1;
  return((mus_any *)gen);
}

int mus_one_zero_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_ONE_ZERO));}

Float mus_one_pole(mus_any *ptr, Float input)
{
  smpflt *gen = (smpflt *)ptr;
  gen->y1 = (gen->a0 * input) - (gen->b1 * gen->y1);
  return(gen->y1);
}

static Float run_one_pole(mus_any *ptr, Float input, Float unused) {return(mus_one_pole(ptr, input));}

static mus_any_class ONE_POLE_CLASS = {
  MUS_ONE_POLE,
  "one_pole",
  &free_smpflt,
  &describe_smpflt,
  &inspect_smpflt,
  &smpflt_equalp,
  0, 0,
  &one_length, 0,
  0, 0, 0, 0,
  &smp_a0,
  &smp_set_a0,
  &smp_b1,
  &smp_set_b1,
  &run_one_pole,
  MUS_SIMPLE_FILTER, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_one_pole(Float a0, Float b1)
{
  smpflt *gen;
  gen = (smpflt *)clm_calloc(1, sizeof(smpflt), "one_pole");
  gen->core = &ONE_POLE_CLASS;
  gen->a0 = a0;
  gen->b1 = b1;
  return((mus_any *)gen);
}

int mus_one_pole_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_ONE_POLE));}

Float mus_two_zero(mus_any *ptr, Float input)
{
  smpflt *gen = (smpflt *)ptr;
  Float result;
  result = (gen->a0 * input) + (gen->a1 * gen->x1) + (gen->a2 * gen->x2);
  gen->x2 = gen->x1;
  gen->x1 = input;
  return(result);
}

static Float run_two_zero(mus_any *ptr, Float input, Float unused) {return(mus_two_zero(ptr, input));}

static mus_any_class TWO_ZERO_CLASS = {
  MUS_TWO_ZERO,
  "two_zero",
  &free_smpflt,
  &describe_smpflt,
  &inspect_smpflt,
  &smpflt_equalp,
  0, 0,
  &two_length, 0,
  0, 0, 0, 0,
  &smp_a0,
  &smp_set_a0,
  0, 0,
  &run_two_zero,
  MUS_SIMPLE_FILTER, 
  NULL, 0,
  &smp_a1, &smp_set_a1, 
  &smp_a2, &smp_set_a2, 
  0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_two_zero(Float a0, Float a1, Float a2)
{
  smpflt *gen;
  gen = (smpflt *)clm_calloc(1, sizeof(smpflt), "two_zero");
  gen->core = &TWO_ZERO_CLASS;
  gen->a0 = a0;
  gen->a1 = a1;
  gen->a2 = a2;
  return((mus_any *)gen);
}

int mus_two_zero_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_TWO_ZERO));}

mus_any *mus_make_zpolar(Float radius, Float frequency)
{
  return(mus_make_two_zero(1.0, -2.0 * radius * cos(mus_hz2radians(frequency)), radius * radius));
}

Float mus_two_pole(mus_any *ptr, Float input)
{
  smpflt *gen = (smpflt *)ptr;
  Float result;
  result = (gen->a0 * input) - (gen->b1 * gen->y1) - (gen->b2 * gen->y2);
  gen->y2 = gen->y1;
  gen->y1 = result;
  return(result);
}

static Float run_two_pole(mus_any *ptr, Float input, Float unused) {return(mus_two_pole(ptr, input));}

static mus_any_class TWO_POLE_CLASS = {
  MUS_TWO_POLE,
  "two_pole",
  &free_smpflt,
  &describe_smpflt,
  &inspect_smpflt,
  &smpflt_equalp,
  0, 0,
  &two_length, 0,
  0, 0, 0, 0,
  &smp_a0,
  &smp_set_a0,
  &smp_b1,
  &smp_set_b1,
  &run_two_pole,
  MUS_SIMPLE_FILTER, 
  NULL, 0,
  0, 0, 0, 0,
  &smp_b2, &smp_set_b2, 
  0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_two_pole(Float a0, Float b1, Float b2)
{
 smpflt *gen;
 if (fabs(b1) >= 2.0) 
   mus_error(MUS_UNSTABLE_TWO_POLE_ERROR, "make_two_pole: b1 = %.3f", b1);
 else
   {
     if (fabs(b2) >= 1.0) 
       mus_error(MUS_UNSTABLE_TWO_POLE_ERROR, "make_two_pole: b2 = %.3f", b2);
     else
       {
	 if ( ((b1 * b1) - (b2 * 4.0) >= 0.0) &&
	      ( ((b1 + b2) >= 1.0) || 
		((b2 - b1) >= 1.0)))
	   mus_error(MUS_UNSTABLE_TWO_POLE_ERROR, "make_two_pole: b1 = %.3f, b2 = %.3f", b1, b2);
	 else
	   {
	     gen = (smpflt *)clm_calloc(1, sizeof(smpflt), "two_pole");
	     gen->core = &TWO_POLE_CLASS;
	     gen->a0 = a0;
	     gen->b1 = b1;
	     gen->b2 = b2;
	     return((mus_any *)gen);
	    }
	}
    }
  return(NULL);
}

int mus_two_pole_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_TWO_POLE));}

mus_any *mus_make_ppolar(Float radius, Float frequency)
{
  return(mus_make_two_pole(1.0, -2.0 * radius * cos(mus_hz2radians(frequency)), radius * radius));
}



/* ---------------- formant ---------------- */

int mus_formant_p (mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_FORMANT));}

static char *describe_formant(mus_any *ptr)
{
  smpflt *gen = (smpflt *)ptr;
  if (mus_formant_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "formant: radius: %.3f, frequency: %.3f, (gain: %.3f)",
		 gen->radius, gen->frequency, gen->gain);
  else describe_bad_gen(ptr, "formant", "a");
  return(describe_buffer);
}

Float mus_formant(mus_any *ptr, Float input) 
{
  smpflt *gen = (smpflt *)ptr;
  Float inval, tpinval, output;
  inval = gen->a0 * input;
  tpinval = inval + (gen->a2 * gen->x2);
  output = tpinval - (gen->b1 * gen->y1) - (gen->b2 * gen->y2);
  gen->y2 = gen->y1;
  gen->y1 = output;
  gen->x2 = gen->x1;
  gen->x1 = inval;
  return(output);
}

static Float run_formant(mus_any *ptr, Float input, Float unused) {return(mus_formant(ptr, input));}

Float mus_formant_bank(Float *amps, mus_any **formants, Float inval, int size)
{
  int i;
  Float sum = 0.0;
  for (i = 0; i < size; i++) 
    sum += (amps[i] * mus_formant(formants[i], inval));
  return(sum);
}

void mus_set_formant_radius_and_frequency(mus_any *ptr, Float radius, Float frequency)
{
  Float fw;
  smpflt *gen = (smpflt *)ptr;
  fw = mus_hz2radians(frequency);
  gen->radius = radius;
  gen->frequency = frequency;
  gen->b2 = radius * radius;
  gen->a0 = gen->gain * sin(fw) * (1.0 - gen->b2);
  gen->a2 = -radius;
  gen->b1 = -2.0 * radius * cos(fw);
}

static Float formant_frequency(mus_any *ptr)
{
  smpflt *gen = (smpflt *)ptr;
  return(gen->frequency);
}

static Float set_formant_frequency(mus_any *ptr, Float val)
{
  smpflt *gen = (smpflt *)ptr;
  mus_set_formant_radius_and_frequency(ptr, gen->radius, val);
  return(val);
}

static Float f_radius(mus_any *ptr) {return(((smpflt *)ptr)->radius);}
static Float f_set_radius(mus_any *ptr, Float val) {mus_set_formant_radius_and_frequency(ptr, val, ((smpflt *)ptr)->frequency); return(val);}

static mus_any_class FORMANT_CLASS = {
  MUS_FORMANT,
  "formant",
  &free_smpflt,
  &describe_formant,
  &inspect_smpflt,
  &smpflt_equalp,
  0, 0,
  &two_length, 0,
  &formant_frequency,
  &set_formant_frequency,
  &f_radius,
  &f_set_radius,
  &smp_a0,
  &smp_set_a0,
  &smp_b1,
  &smp_set_b1,
  &run_formant,
  MUS_SIMPLE_FILTER, 
  NULL, 0,
  &smp_a1, &smp_set_a1, 
  &smp_a2, &smp_set_a2, 
  &smp_b2, &smp_set_b2, 
  0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_formant(Float radius, Float frequency, Float gain)
{
  smpflt *gen;
  gen = (smpflt *)clm_calloc(1, sizeof(smpflt), "formant");
  gen->core = &FORMANT_CLASS;
  gen->gain = gain;
  gen->a1 = gain; /* for backwards compatibility */
  mus_set_formant_radius_and_frequency((mus_any *)gen, radius, frequency);
  return((mus_any *)gen);
}



/*---------------- sine-summation ---------------- */

typedef struct {
  mus_any_class *core;
  Float freq;
  Float phase;
  Float a, b, an, a2;
  int n;
} sss;

static char *inspect_sss(mus_any *ptr)
{
  sss *gen = (sss *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
	       "sss freq: %f, phase: %f, a: %f, b: %f, an: %f, a2: %f, n: %d",
	       gen->freq, gen->phase, gen->a, gen->b, gen->an, gen->a2, gen->n);
  return(describe_buffer);
}

static int free_sss(mus_any *ptr) {if (ptr) FREE(ptr); return(0);}
static Float sss_freq(mus_any *ptr) {return(mus_radians2hz(((sss *)ptr)->freq));}
static Float set_sss_freq(mus_any *ptr, Float val) {((sss *)ptr)->freq = mus_hz2radians(val); return(val);}
static Float sss_phase(mus_any *ptr) {return(fmod(((sss *)ptr)->phase, TWO_PI));}
static Float set_sss_phase(mus_any *ptr, Float val) {((sss *)ptr)->phase = val; return(val);}
static Float sss_a(mus_any *ptr) {return(((sss *)ptr)->a);}
static Float set_sss_a(mus_any *ptr, Float val) 
{
  sss *gen = (sss *)ptr;
  gen->a = val;
  gen->a2 = 1.0 + val * val;
  gen->an = pow(val, gen->n + 1);
  return(val);
}

static int sss_equalp(mus_any *p1, mus_any *p2)
{
  sss *g1 = (sss *)p1;
  sss *g2 = (sss *)p2;
  return((p1 == p2) ||
	 (((g1->core)->type == (g2->core)->type) &&
	  (g1->freq == g2->freq) &&
	  (g1->phase == g2->phase) &&
	  (g1->n == g2->n) &&
	  (g1->a == g2->a) &&
	  (g1->b == g2->b)));
}

int mus_sine_summation_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_SINE_SUMMATION));}

static char *describe_sss(mus_any *ptr)
{
  sss *gen = (sss *)ptr;
  if (mus_sine_summation_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "sine_summation: frequency: %.3f, phase: %.3f, n: %d, a: %.3f, ratio: %.3f",
		 mus_radians2hz(gen->freq), 
		 gen->phase, gen->n, gen->a, gen->b);
  else describe_bad_gen(ptr, "sine_summation", "a");
  return(describe_buffer);
}

Float mus_sine_summation(mus_any *ptr, Float fm)
{
  sss *gen = (sss *)ptr;
  Float B, thB, result, divisor;
  B = gen->b * gen->phase;
  thB = gen->phase - B;
  divisor = (gen->a2 - (2 * gen->a * cos(B)));
  if (divisor == 0.0) 
    result = 0.0;
  /* if a=1.0, the formula given by Moorer is extremely unstable anywhere near phase=0.0 
   *   (map-channel (let ((gen (make-sine-summation 100.0 0.0 1 1 1.0))) (lambda (y) (sine-summation gen))))
   * or even worse:
   *   (map-channel (let ((gen (make-sine-summation 100.0 0.0 0 1 1.0))) (lambda (y) (sine-summation gen))))
   * which should be a sine wave! 
   * I wonder if that formula is incorrect...
   */
  else result = (mus_sin(gen->phase) - (gen->a * mus_sin(thB)) - 
		 (gen->an * (mus_sin(gen->phase + (B * (gen->n + 1))) - 
			     (gen->a * mus_sin(gen->phase + (B * gen->n)))))) / divisor;
  gen->phase += (gen->freq + fm);
  gen->phase = fmod(gen->phase, TWO_PI);
  return(result);
}

static Float run_sine_summation(mus_any *ptr, Float fm, Float unused) {return(mus_sine_summation(ptr, fm));}

static mus_any_class SINE_SUMMATION_CLASS = {
  MUS_SINE_SUMMATION,
  "sine_summation",
  &free_sss,
  &describe_sss,
  &inspect_sss,
  &sss_equalp,
  0, 0, 0, 0,
  &sss_freq,
  &set_sss_freq,
  &sss_phase,
  &set_sss_phase,
  &sss_a,
  &set_sss_a,
  0, 0,
  &run_sine_summation,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_sine_summation(Float frequency, Float phase, int n, Float a, Float b_ratio)
{
  sss *gen;
  gen = (sss *)clm_calloc(1, sizeof(sss), "sine_summation");
  gen->core = &SINE_SUMMATION_CLASS;
  gen->freq = mus_hz2radians(frequency);
  gen->phase = phase;
  gen->an = pow(a, n + 1);
  gen->a2 = 1.0 + a * a;
  gen->a = a;
  gen->n = n;
  gen->b = b_ratio;
  return((mus_any *)gen);
}



/* ---------------- filter ---------------- */

typedef struct {
  mus_any_class *core;
  int order, state_allocated;
  Float *x, *y, *state;
} flt;

static char *inspect_flt(mus_any *ptr)
{
  flt *gen = (flt *)ptr;
  char *arr1 = NULL, *arr2 = NULL, *arr3 = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "flt order: %d, state (%s): %s, x: %s, y: %s",
	       gen->order,
	       (gen->state_allocated) ? "local" : "external",
	       arr1 = print_array(gen->state, gen->order, 0),
	       arr2 = print_array(gen->x, gen->order, 0),
	       arr3 = print_array(gen->y, gen->order, 0));
  if (arr1) FREE(arr1);
  if (arr2) FREE(arr2);
  if (arr3) FREE(arr3);
  return(describe_buffer);
}

Float mus_filter (mus_any *ptr, Float input)
{
  flt *gen = (flt *)ptr;
  Float xout;
  int j;
  xout = 0.0;
  gen->state[0] = input;
  for (j = gen->order - 1; j >= 1; j--) 
    {
      xout += gen->state[j] * gen->x[j];
      gen->state[0] -= gen->y[j] * gen->state[j];
      gen->state[j] = gen->state[j - 1];
    }
  return(xout + (gen->state[0] * gen->x[0]));
}

Float mus_fir_filter (mus_any *ptr, Float input)
{
  Float xout;
  int j;
  flt *gen = (flt *)ptr;
  xout = 0.0;
  gen->state[0] = input;
  for (j = gen->order - 1; j >= 1; j--) 
    {
      xout += gen->state[j] * gen->x[j];
      gen->state[j] = gen->state[j - 1];
    }
  return(xout + (gen->state[0] * gen->x[0]));
}

Float mus_iir_filter (mus_any *ptr, Float input)
{
  int j;
  flt *gen = (flt *)ptr;
  gen->state[0] = input;
  for (j = gen->order - 1; j >= 1; j--) 
    {
      gen->state[0] -= gen->y[j] * gen->state[j];
      gen->state[j] = gen->state[j - 1];
    }
  return(gen->state[0]);
}

static Float run_filter(mus_any *ptr, Float input, Float unused) {return(mus_filter(ptr, input));}
static Float run_fir_filter(mus_any *ptr, Float input, Float unused) {return(mus_fir_filter(ptr, input));}
static Float run_iir_filter(mus_any *ptr, Float input, Float unused) {return(mus_iir_filter(ptr, input));}

int mus_filter_p (mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_FILTER));}
int mus_fir_filter_p (mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_FIR_FILTER));}
int mus_iir_filter_p (mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_IIR_FILTER));}
static Float *filter_data(mus_any *ptr) {return(((flt *)ptr)->state);}
static int filter_length(mus_any *ptr) {return(((flt *)ptr)->order);}
static int set_filter_length(mus_any *ptr, int val) {((flt *)ptr)->order = val; return(val);}

static int free_filter(mus_any *ptr)
{
  flt *gen = (flt *)ptr;
  if (gen)
    {
      if ((gen->state) && (gen->state_allocated)) FREE(gen->state);
      FREE(gen);
    }
  return(0);
}

static int filter_equalp(mus_any *p1, mus_any *p2) 
{
  flt *f1, *f2;
  int i;
  if (((p1->core)->type == (p2->core)->type) &&
      ((mus_filter_p(p1)) || (mus_fir_filter_p(p1)) || (mus_iir_filter_p(p1))))
    {
      f1 = (flt *)p1;
      f2 = (flt *)p2;
      if (f1->order == f2->order)
	{
	  for (i = 0; i < f1->order; i++)
	    {
	      if ((f1->x) && (f2->x) && (f1->x[i] != f2->x[i])) return(FALSE);
	      if ((f1->y) && (f2->y) && (f1->y[i] != f2->y[i])) return(FALSE);
	      if (f1->state[i] != f2->state[i]) return(FALSE);
	    }
	  return(TRUE);
	}
    }
  return(FALSE);
}

static char *describe_filter(mus_any *ptr)
{
  flt *gen = (flt *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, "%s: order: %d", (gen->core)->name, gen->order);
  return(describe_buffer);
}

static mus_any_class FILTER_CLASS = {
  MUS_FILTER,
  "filter",
  &free_filter,
  &describe_filter,
  &inspect_flt,
  &filter_equalp,
  &filter_data, 0,
  &filter_length,
  &set_filter_length,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  &run_filter,
  MUS_FULL_FILTER, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

static mus_any_class FIR_FILTER_CLASS = {
  MUS_FIR_FILTER,
  "fir_filter",
  &free_filter,
  &describe_filter,
  &inspect_flt,
  &filter_equalp,
  &filter_data, 0,
  &filter_length,
  &set_filter_length,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  &run_fir_filter,
  MUS_FULL_FILTER, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

static mus_any_class IIR_FILTER_CLASS = {
  MUS_IIR_FILTER,
  "iir_filter",
  &free_filter,
  &describe_filter,
  &inspect_flt,
  &filter_equalp,
  &filter_data, 0,
  &filter_length,
  &set_filter_length,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  &run_iir_filter,
  MUS_FULL_FILTER, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

static mus_any *make_filter(mus_any_class *cls, const char *name, int order, Float *xcoeffs, Float *ycoeffs, Float *state) /* if state null, allocated locally */
{
  flt *gen;
  if (order <= 0)
    mus_error(MUS_ARG_OUT_OF_RANGE, "%s order = %d?", name, order);
  else
    {
      gen = (flt *)clm_calloc(1, sizeof(flt), name);
      if (state)
	gen->state = state;
      else 
	{
	  gen->state = (Float *)clm_calloc(order, sizeof(Float), "filter coeff space");
	  gen->state_allocated = TRUE;
	}
      gen->core = cls;
      gen->order = order;
      gen->x = xcoeffs;
      gen->y = ycoeffs;
      return((mus_any *)gen);
    }
  return(NULL);
}

mus_any *mus_make_filter(int order, Float *xcoeffs, Float *ycoeffs, Float *state)
{
  return(make_filter(&FILTER_CLASS, S_make_filter, order, xcoeffs, ycoeffs, state));
}

mus_any *mus_make_fir_filter(int order, Float *xcoeffs, Float *state)
{
  return(make_filter(&FIR_FILTER_CLASS, S_make_fir_filter, order, xcoeffs, NULL, state));
}

mus_any *mus_make_iir_filter(int order, Float *ycoeffs, Float *state)
{
  return(make_filter(&IIR_FILTER_CLASS, S_make_iir_filter, order, NULL, ycoeffs, state));
}

Float *mus_xcoeffs(mus_any *ptr)
{
  flt *gen = (flt *)ptr;
  if (check_gen(ptr, S_mus_xcoeffs))
    {
      if (((gen->core)->type == MUS_FILTER) || ((gen->core)->type == MUS_FIR_FILTER))
	return(gen->x);
    }
  return(NULL);
}

Float *mus_ycoeffs(mus_any *ptr)
{
  flt *gen = (flt *)ptr;
  if (check_gen(ptr, S_mus_ycoeffs))
    {
      if (((gen->core)->type == MUS_FILTER) || ((gen->core)->type == MUS_IIR_FILTER))
	return(gen->y);
    }
  return(NULL);
}

Float *mus_make_fir_coeffs(int order, Float *envl, Float *aa)
{
 /* envl = evenly sampled freq response, has order samples */
  int n, m, i, j, jj;
  Float am, q, xt = 0.0, xt0;
  Float *a;
  n = order;
  if (n <= 0) return(aa);
  if (aa) 
    a = aa;
  else a = (Float *)clm_calloc(order, sizeof(Float), "coeff space");
  m = (n + 1) / 2;
  am = 0.5 * (n + 1);
  q = TWO_PI / (Float)n;
  xt0 = envl[0] * 0.5;
  for (j = 0, jj = n - 1; j < m; j++, jj--)
    {
      xt = xt0;
      for (i = 1; i < m; i++)
	xt += (envl[i] * cos(q * (am - j - 1) * i));
      a[j] = 2.0 * xt / (Float)n;
      a[jj] = a[j];
    }
  return(a);
}


void mus_clear_filter_state(mus_any *gen)
{
  if (((gen->core)->extended_type == MUS_FULL_FILTER) ||
      ((gen->core)->extended_type == MUS_DELAY_LINE))
    {
      int len;
      Float *state;
      state = mus_data(gen);
      len = mus_length(gen);
      memset((void *)state, 0, len * sizeof(Float));
    }
  else
    {
      if ((gen->core)->extended_type == MUS_SIMPLE_FILTER)
	{
	  smpflt *ptr;
	  ptr = (smpflt *)gen;
	  ptr->x1 = 0.0;
	  ptr->x2 = 0.0;
	  ptr->y1 = 0.0;
	  ptr->y2 = 0.0;
	}
    }
}

Float mus_x1(mus_any *gen)
{
  if ((gen) &&
      ((gen->core)->extended_type == MUS_SIMPLE_FILTER))
    return(((smpflt *)gen)->x1);
  return((Float)mus_error(MUS_NO_X1, "can't get %s's x1", mus_name(gen)));
}

Float mus_set_x1(mus_any *gen, Float val)
{
  if ((gen) &&
      ((gen->core)->extended_type == MUS_SIMPLE_FILTER))
    ((smpflt *)gen)->x1 = val;
  else mus_error(MUS_NO_X1, "can't set %s's x1", mus_name(gen));
  return(val);
}

Float mus_x2(mus_any *gen)
{
  if ((gen) &&
      ((gen->core)->extended_type == MUS_SIMPLE_FILTER))
    return(((smpflt *)gen)->x2);
  return((Float)mus_error(MUS_NO_X2, "can't get %s's x2", mus_name(gen)));
}

Float mus_set_x2(mus_any *gen, Float val)
{
  if ((gen) &&
      ((gen->core)->extended_type == MUS_SIMPLE_FILTER))
    ((smpflt *)gen)->x2 = val;
  else mus_error(MUS_NO_X2, "can't set %s's x2", mus_name(gen));
  return(val);
}

Float mus_y1(mus_any *gen)
{
  if ((gen) &&
      ((gen->core)->extended_type == MUS_SIMPLE_FILTER))
    return(((smpflt *)gen)->y1);
  return((Float)mus_error(MUS_NO_Y1, "can't get %s's y1", mus_name(gen)));
}

Float mus_set_y1(mus_any *gen, Float val)
{
  if ((gen) &&
      ((gen->core)->extended_type == MUS_SIMPLE_FILTER))
    ((smpflt *)gen)->y1 = val;
  else mus_error(MUS_NO_Y1, "can't set %s's y1", mus_name(gen));
  return(val);
}

Float mus_y2(mus_any *gen)
{
  if ((gen) &&
      ((gen->core)->extended_type == MUS_SIMPLE_FILTER))
    return(((smpflt *)gen)->y2);
  return((Float)mus_error(MUS_NO_Y2, "can't get %s's y2", mus_name(gen)));
}

Float mus_set_y2(mus_any *gen, Float val)
{
  if ((gen) &&
      ((gen->core)->extended_type == MUS_SIMPLE_FILTER))
    ((smpflt *)gen)->y2 = val;
  else mus_error(MUS_NO_Y2, "can't set %s's y2", mus_name(gen));
  return(val);
}



/* ---------------- waveshape ---------------- */

typedef struct {
  mus_any_class *core;
  Float freq;
  Float phase;
  Float *table;
  int table_size;
  Float offset;
  int table_allocated;
} ws;

static char *inspect_ws(mus_any *ptr)
{
  ws *gen = (ws *)ptr;
  char *arr = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
	       "ws freq: %f, phase: %f, offset: %f, table[%d (%s)]: %s",
	       gen->freq, gen->phase, gen->offset, gen->table_size,
	       (gen->table_allocated) ? "local" : "external",
	       arr = print_array(gen->table, gen->table_size, 0));
  if (arr) FREE(arr);
  return(describe_buffer);
}

static int free_ws(mus_any *pt) 
{
  ws *ptr = (ws *)pt;
  if (ptr) 
    {
      if ((ptr->table) && (ptr->table_allocated)) FREE(ptr->table);
      FREE(ptr); 
    }
  return(0);
}

static Float ws_freq(mus_any *ptr) {return(mus_radians2hz(((ws *)ptr)->freq));}
static Float set_ws_freq(mus_any *ptr, Float val) {((ws *)ptr)->freq = mus_hz2radians(val); return(val);}
static Float ws_phase(mus_any *ptr) {return(fmod(((ws *)ptr)->phase, TWO_PI));}
static Float set_ws_phase(mus_any *ptr, Float val) {((ws *)ptr)->phase = val; return(val);}
static int ws_size(mus_any *ptr) {return(((ws *)ptr)->table_size);}
static int set_ws_size(mus_any *ptr, int val) {((ws *)ptr)->table_size = val; return(val);}
static Float *ws_data(mus_any *ptr) {return(((ws *)ptr)->table);}

static int ws_equalp(mus_any *p1, mus_any *p2)
{
  int i;
  ws *w1 = (ws *)p1;
  ws *w2 = (ws *)p2;
  if (p1 == p2) return(TRUE);
  if ((w1) && (w2) &&
      (w1->core->type == w2->core->type) &&
      (w1->freq == w2->freq) &&
      (w1->phase == w2->phase) &&
      (w1->table_size == w2->table_size) &&
      (w1->offset == w2->offset))
    {
      for (i = 0; i < w1->table_size; i++)
	if (w1->table[i] != w2->table[i])
	  return(FALSE);
      return(TRUE);
    }
  return(FALSE);
}

static Float *set_ws_data(mus_any *ptr, Float *val) 
{
  ws *gen = (ws *)ptr;
  if (gen->table_allocated) {FREE(gen->table); gen->table_allocated = FALSE;}
  gen->table = val; 
  return(val);
}

static char *describe_waveshape(mus_any *ptr)
{
  if (mus_waveshape_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "waveshape freq: %.3fHz, phase: %.3f, size: %d",
		 mus_radians2hz(((ws *)ptr)->freq), 
		 ((ws *)ptr)->phase, 
		 ((ws *)ptr)->table_size);
  else describe_bad_gen(ptr, "waveshape", "a");
  return(describe_buffer);
}

static mus_any_class WAVESHAPE_CLASS = {
  MUS_WAVESHAPE,
  "waveshape",
  &free_ws,
  &describe_waveshape,
  &inspect_ws,
  &ws_equalp,
  &ws_data,
  &set_ws_data,
  &ws_size,
  &set_ws_size,
  &ws_freq,
  &set_ws_freq,
  &ws_phase,
  &set_ws_phase,
  0, 0,
  0, 0,
  &mus_waveshape,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

int mus_waveshape_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_WAVESHAPE));}

mus_any *mus_make_waveshape(Float frequency, Float phase, Float *table, int size)
{
  ws *gen;
  gen = (ws *)clm_calloc(1, sizeof(ws), "waveshape");
  gen->core = &WAVESHAPE_CLASS;
  gen->freq = mus_hz2radians(frequency);
  gen->phase = phase;
  if (table)
    {
      gen->table = table;
      gen->table_allocated = FALSE;
    }
  else
    {
      gen->table = (Float *)clm_calloc(size, sizeof(Float), "waveshape table");
      gen->table_allocated = TRUE;
    }
  gen->table_size = size;
  gen->offset = (Float)size / 2.0;
  return((mus_any *)gen);
}

Float mus_waveshape(mus_any *ptr, Float index, Float fm)
{
  ws *gen = (ws *)ptr;
  Float oscval;
  oscval = mus_sin(gen->phase);
  gen->phase += (gen->freq + fm);
  if ((gen->phase > 100.0) || (gen->phase < -100.0)) gen->phase = fmod(gen->phase, TWO_PI);
  return(mus_array_interp(gen->table, gen->offset * (1.0 + (oscval * index)), gen->table_size));
}

Float mus_waveshape_1(mus_any *ptr, Float index)
{
  ws *gen = (ws *)ptr;
  Float oscval;
  oscval = mus_sin(gen->phase);
  gen->phase += gen->freq;
  if ((gen->phase > 100.0) || (gen->phase < -100.0)) gen->phase = fmod(gen->phase, TWO_PI);
  return(mus_array_interp(gen->table, gen->offset * (1.0 + (oscval * index)), gen->table_size));
}

Float *mus_partials2waveshape(int npartials, Float *partials, int size, Float *table)
{
  /* partials incoming is a list of partials amps indexed by partial number */
  /* #<0.0, 1.0, 0.0> = 2nd partial 1.0, rest 0. */
  int i, hnum;
  Float sum = 0.0, maxI2, temp, x, Tn, Tn1;
  Float *data;
  if (partials == NULL) return(NULL);
  for (i = 0; i < npartials; i++) sum += partials[i];
  if (sum != 0.0) for (i = 0; i < npartials; i++) partials[i] /= sum;
  for (i = 2; i < npartials; i += 4)
    {
      partials[i] = (-partials[i]);
      if (npartials > (i + 1)) partials[i + 1] = (-partials[i + 1]);
    }
  if (table == NULL)
    data = (Float *)clm_calloc(size, sizeof(Float), "waveshape table");
  else data = table;
  if (data == NULL) return(NULL);
  maxI2 = 2.0 / (Float)size;
  for (i = 0, x = -1.0; i < size; i++, x += maxI2)
    {
      sum = 0.0;
      temp = 0.0;
      Tn = 1.0;
      Tn1 = x;
      for (hnum = 0; hnum < npartials; hnum++)
	{
	  sum += (Tn * partials[hnum]);
	  temp = Tn1;
	  Tn1 = (2.0 * Tn1 * x) - Tn;
	  Tn = temp;
	}
      data[i] = sum;
    }
  return(array_normalize(data, size));
}

Float *mus_partials2polynomial(int npartials, Float *partials, int kind)
{
  /* coeffs returned in partials */
  int i, k;
  Float amp = 0.0;
  int *T0, *T1, *Tn;
  Float *Cc1;
  T0 = (int *)clm_calloc(npartials + 1, sizeof(int), "partials2polynomial t0");
  T1 = (int *)clm_calloc(npartials + 1, sizeof(int), "partials2polynomial t1");
  Tn = (int *)clm_calloc(npartials + 1, sizeof(int), "partials2polynomial tn");
  Cc1 = (Float *)clm_calloc(npartials + 1, sizeof(Float), "partials2polynomial cc1");
  T0[0] = kind;
  T1[1] = 1;
  for (i = 1; i < npartials; i++)
    {
      amp = partials[i];
      if (amp != 0.0)
	{
	  if (kind == 1)
	    for (k = 0; k <= i; k++) 
	      Cc1[k] += (amp * T1[k]);
	  else
	    for (k = 1; k <= i; k++) 
	      Cc1[k - 1] += (amp * T1[k]);
	}
      for (k = i + 1; k > 0; k--) 
	Tn[k] = (2 * T1[k - 1]) - T0[k];
      Tn[0] = -T0[0];
      for (k = i + 1; k >= 0; k--)
	{
	  T0[k] = T1[k];
	  T1[k] = Tn[k];
	}
    }
  for (i = 0; i < npartials; i++) 
    partials[i] = Cc1[i];
  FREE(T0);
  FREE(T1);
  FREE(Tn);
  FREE(Cc1);
  return(partials);
}



/* ---------------- env ---------------- */

/* although a pain, this way of doing env is 5 times faster than a table lookup,
 * in the linear segment and step cases.  In the exponential case, it is
 * only slightly slower (but more accurate).
 */

typedef struct {
  mus_any_class *core;
  double rate, current_value, base, offset, scaler, power, init_y, init_power, original_scaler, original_offset;
  off_t pass, end;
  int style, index, size, data_allocated;
  Float *original_data;
  double *rates;
  off_t *passes;
} seg;

enum {ENV_SEG, ENV_STEP, ENV_EXP};

static char *inspect_seg(mus_any *ptr)
{
  seg *gen = (seg *)ptr;
  char *arr = NULL, *str1 = NULL, *str2 = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "seg scaler: %.4f, offset: %.4f, rate: %f, current_value: %f, base: %f, offset: %f, scaler: %f, power: %f, init_y: %f, init_power: %f, \
pass: " OFF_TD ", end: " OFF_TD ", style: %d, index: %d, size: %d, original_data[%d]: %s, rates[%d]: %s, passes[%d]: %s",
	       gen->original_scaler, gen->original_offset,
	       gen->rate, gen->current_value, gen->base, gen->offset, gen->scaler, gen->power, gen->init_y, gen->init_power,
	       gen->pass, gen->end, gen->style, gen->index, gen->size,
	       gen->size * 2,
	       arr = print_array(gen->original_data, gen->size * 2, 0),
	       gen->size,
	       str1 = print_double_array(gen->rates, gen->size, 0),
	       gen->size,
	       str2 = print_off_t_array(gen->passes, gen->size, 0));
  if (arr) FREE(arr);
  if (str1) FREE(str1);
  if (str2) FREE(str2);
  return(describe_buffer);
}

/* what about breakpoint triples for per-segment exp envs? */

int mus_env_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_ENV));}

Float mus_env(mus_any *ptr)
{
  seg *gen = (seg *)ptr;
  Float val;
  val = gen->current_value;
  if ((gen->index < gen->size) && (gen->pass >= gen->passes[gen->index]))
    {
      gen->index++;
      gen->rate = gen->rates[gen->index];
    }
  switch (gen->style)
    {
    case ENV_SEG: gen->current_value += gen->rate; break;
    case ENV_STEP: gen->current_value = gen->rate; break;
    case ENV_EXP:
      if (gen->rate != 0.0)
	{
	  gen->power += gen->rate;
	  gen->current_value = gen->offset + (gen->scaler * exp(gen->power));
	}
      break;
    }
  gen->pass++;
  return(val);
}

static Float run_env (mus_any *ptr, Float unused1, Float unused2) {return(mus_env(ptr));}

static void dmagify_env(seg *e, Float *data, int pts, off_t dur, Float scaler)
{ 
  int i, j;
  off_t passes;
  double curx, curpass = 0.0;
  double x0, y0, x1 = 0.0, y1 = 0.0, xmag = 1.0;
  e->size = pts;
  if (pts > 1)
    {
      x1 = data[0];
      if (data[pts * 2 - 2] != x1)
	xmag = (double)(dur - 1) / (double)(data[pts * 2 - 2] - x1); /* was dur, 7-Apr-02 */
      y1 = data[1];
    }
  e->rates = (double *)clm_calloc(pts, sizeof(double), "env rates");
  e->passes = (off_t *)clm_calloc(pts, sizeof(off_t), "env passes");
  for (j = 0, i = 2; i < pts * 2; i += 2, j++)
    {
      x0 = x1;
      x1 = data[i];
      y0 = y1;
      y1 = data[i + 1];
      curx = xmag * (x1 - x0);
      if (curx < 1.0) curx = 1.0;
      curpass += curx;
      if (e->style == ENV_STEP)
	e->passes[j] = (off_t)curpass; /* this is the change boundary (confusing...) */
      else e->passes[j] = (off_t)(curpass + 0.5);
      if (j == 0) 
	passes = e->passes[0]; 
      else passes = e->passes[j] - e->passes[j - 1];
      if (e->style == ENV_STEP)
	e->rates[j] = e->offset + (scaler * y0);
      else
	{
	  if ((y0 == y1) || (passes == 0))
	    e->rates[j] = 0.0;
	  else e->rates[j] = scaler * (y1 - y0) / (double)passes;
	}
    }
  if ((pts > 1) && (e->passes[pts - 2] != e->end))
    e->passes[pts - 2] = e->end;
  if ((pts > 1) && (e->style == ENV_STEP))
    e->rates[pts - 1] = e->rates[pts - 2]; /* stick at last value, which in this case is the value (not 0 as increment) */
  e->passes[pts - 1] = 100000000;
}

static Float *fixup_exp_env(seg *e, Float *data, int pts, Float offset, Float scaler, Float base)
{
  Float min_y, max_y, val = 0.0, tmp = 0.0, b1;
  int flat, len, i;
  Float *result = NULL;
  if ((base <= 0.0) || (base == 1.0)) return(NULL);
  min_y = offset + scaler * data[1];
  max_y = min_y;
  b1 = base - 1.0;
  len = pts * 2;
  result = (Float *)clm_calloc(len, sizeof(Float), "env data");
  result[0] = data[0];
  result[1] = min_y;
  for (i = 2; i < len; i += 2)
    {
      tmp = offset + scaler * data[i + 1];
      result[i] = data[i];
      result[i + 1] = tmp;
      if (tmp < min_y) min_y = tmp;
      if (tmp > max_y) max_y = tmp;
    }
  flat = (min_y == max_y);
  if (!flat) val = 1.0 / (max_y - min_y);
  for (i = 1; i < len; i += 2)
    {
      if (flat) 
	tmp = 1.0;
      else tmp = val * (result[i] - min_y);
      result[i] = log(1.0 + (tmp * b1));
    }
  e->scaler = (max_y - min_y) / b1;
  e->offset = min_y;
  return(result);
}

static int env_equalp(mus_any *p1, mus_any *p2)
{
  int i;
  seg *e1 = (seg *)p1;
  seg *e2 = (seg *)p2;
  if (p1 == p2) return(TRUE);
  if ((e1) && (e2) &&
      (e1->core->type == e2->core->type) &&
      (e1->pass == e2->pass) &&
      (e1->end == e2->end) &&
      (e1->style == e2->style) &&
      (e1->index == e2->index) &&
      (e1->size == e2->size) &&

      (e1->rate == e2->rate) &&
      (e1->base == e2->base) &&
      (e1->power == e2->power) &&
      (e1->current_value == e2->current_value) &&
      (e1->scaler == e2->scaler) &&
      (e1->offset == e2->offset) &&
      (e1->init_y == e2->init_y) &&
      (e1->init_power == e2->init_power))
    {
      for (i = 0; i < e1->size * 2; i++)
	if (e1->original_data[i] != e2->original_data[i])
	  return(FALSE);
      return(TRUE);
    }
  return(FALSE);
}

static char *describe_env(mus_any *ptr)
{
  char *str = NULL;
  seg *e = (seg *)ptr;
  if (mus_env_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
		 "env: %s, pass: " OFF_TD " (dur: " OFF_TD "), index: %d, scaler: %.4f, offset: %.4f, data: %s",
		 ((e->style == ENV_SEG) ? "linear" : ((e->style == ENV_EXP) ? "exponential" : "step")),
		 e->pass, e->end + 1, e->index,
		 e->original_scaler, e->original_offset,
		 str = print_array(e->original_data, e->size * 2, 0));
  else describe_bad_gen(ptr, "env", "an");
  if (str) FREE(str);
  return(describe_buffer);
}

static int free_env_gen(mus_any *pt) 
{
  seg *ptr = (seg *)pt;
  if (ptr) 
    {
      if (ptr->passes) FREE(ptr->passes);
      if (ptr->rates) FREE(ptr->rates);
      if ((ptr->original_data) && (ptr->data_allocated)) FREE(ptr->original_data);
      FREE(ptr); 
    }
  return(0);
}

static Float *env_data(mus_any *ptr) {return(((seg *)ptr)->original_data);}
static Float env_scaler(mus_any *ptr) {return(((seg *)ptr)->original_scaler);}
static Float env_offset(mus_any *ptr) {return(((seg *)ptr)->original_offset);}
static Float set_env_offset(mus_any *ptr, Float val) {((seg *)ptr)->original_offset = val; return(val);}
int mus_env_breakpoints(mus_any *ptr) {return(((seg *)ptr)->size);}
static int env_length(mus_any *ptr) {return((int)(((seg *)ptr)->end));}
static Float env_current_value(mus_any *ptr) {return(((seg *)ptr)->current_value);}
off_t *mus_env_passes(mus_any *gen) {return(((seg *)gen)->passes);}
double *mus_env_rates(mus_any *gen) {return(((seg *)gen)->rates);}
static int env_position(mus_any *ptr) {return(((seg *)ptr)->index);}
double mus_env_offset(mus_any *gen) {return(((seg *)gen)->offset);}
double mus_env_scaler(mus_any *gen) {return(((seg *)gen)->scaler);}
double mus_env_initial_power(mus_any *gen) {return(((seg *)gen)->init_power);}
static off_t seg_pass(mus_any *ptr) {return(((seg *)ptr)->pass);}
static void set_env_location(mus_any *ptr, off_t val);
static off_t seg_set_pass(mus_any *ptr, off_t val) {set_env_location(ptr, val); return(val);}

static Float env_increment(mus_any *rd)
{
  if (((seg *)rd)->style == ENV_STEP)
    return(0.0);
  return(((seg *)rd)->base);
}

static mus_any_class ENV_CLASS = {
  MUS_ENV,
  "env",
  &free_env_gen,
  &describe_env,
  &inspect_seg,
  &env_equalp,
  &env_data,
  0,
  &env_length,
  0,
  0, 0, 
  &env_current_value, 0,
  &env_scaler,
  0,
  &env_increment,
  0,
  &run_env,
  MUS_NOT_SPECIAL, 
  NULL,
  &env_position,
  &env_offset, &set_env_offset, 
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 
  &seg_pass, &seg_set_pass,
  0
};

mus_any *mus_make_env(Float *brkpts, int npts, Float scaler, Float offset, Float base, Float duration, off_t start, off_t end, Float *odata)
{
  int i;
  off_t dur_in_samples;
  Float *edata;
  seg *e = NULL;
  for (i = 2; i < npts * 2; i += 2)
    if (brkpts[i - 2] > brkpts[i])
      {
	mus_error(MUS_BAD_ENVELOPE, "env at %d: %f > %f", i / 2, brkpts[i - 2], brkpts[i]);
	return(NULL);
      }
  e = (seg *)clm_calloc(1, sizeof(seg), "env");
  e->core = &ENV_CLASS;
  if (duration != 0.0)
    dur_in_samples = (off_t)(duration * sampling_rate);
  else dur_in_samples = (end - start + 1);
  e->init_y = offset + scaler * brkpts[1];
  e->current_value = e->init_y;
  e->rate = 0.0;
  e->offset = offset;
  e->scaler = scaler;
  e->original_offset = offset;
  e->original_scaler = scaler;
  e->base = base;
  e->end = (dur_in_samples - 1);
  e->pass = 0;
  e->index = 0; /* ? */
  if (odata)
    e->original_data = odata;
  else
    {
      e->original_data = (Float *)clm_calloc(npts * 2, sizeof(Float), "env original data");
      e->data_allocated = TRUE;
    }
  for (i = 0; i < npts * 2; i++) e->original_data[i] = brkpts[i];
  if (base == 0.0)
    {
      e->style = ENV_STEP;
      dmagify_env(e, brkpts, npts, dur_in_samples, scaler);
    }
  else
    {
      if (base == 1.0)
	{
	  e->style = ENV_SEG;
	  dmagify_env(e, brkpts, npts, dur_in_samples, scaler);
	}
      else
	{
	  e->style = ENV_EXP;
	  edata = fixup_exp_env(e, brkpts, npts, offset, scaler, base);
	  if (edata == NULL)
	    {
	      if ((e->original_data) && (e->data_allocated)) FREE(e->original_data);
	      FREE(e);
	      return(NULL);
	    }
	  dmagify_env(e, edata, npts, dur_in_samples, 1.0);
	  e->power = edata[1];
	  e->init_power = e->power;
	  if (edata) FREE(edata);
	  e->offset -= e->scaler;
	}
    }
  e->rate = e->rates[0];
  return((mus_any *)e);
}

void mus_restart_env (mus_any *ptr)
{
  seg *gen = (seg *)ptr;
  gen->current_value = gen->init_y;
  gen->pass = 0;
  gen->index = 0;
  gen->rate = gen->rates[0];
  gen->power = gen->init_power;
}

static void set_env_location(mus_any *ptr, off_t val)
{
  seg *gen = (seg *)ptr;
  off_t passes, ctr = 0;
  mus_restart_env(ptr);
  gen->pass = val;
  while ((gen->index < gen->size) && 
	 (ctr < val))
    {
      if (val > gen->passes[gen->index])
	passes = gen->passes[gen->index] - ctr;
      else passes = val - ctr;
      switch (gen->style)
	{
	case ENV_SEG: 
	  gen->current_value += (passes * gen->rate);
	  break;
	case ENV_STEP: 
	  gen->current_value = gen->rate; 
	  break;
	case ENV_EXP: 
	  gen->power += (passes * gen->rate); 
	  gen->current_value = gen->offset + (gen->scaler * exp(gen->power));
	  break;
	}
      ctr += passes;
      if (ctr < val)
	{
	  gen->index++;
	  if (gen->index < gen->size)
	    gen->rate = gen->rates[gen->index];
	}
    }
}

Float mus_env_interp(Float x, mus_any *ptr)
{
  seg *gen = (seg *)ptr;
  Float *data;
  Float intrp, val = 0.0;
  int i;
  if (gen)
    {
      data = gen->original_data;
      if (data)
	{
	  for (i = 0; i < gen->size * 2 - 2; i += 2)
	    {
	      if (x < data[i + 2])
		{
		  switch (gen->style)
		    {
		    case ENV_STEP: 
		      val = data[i + 1]; 
		      break;
		    case ENV_SEG:
		      if ((x <= data[i]) || 
			  (data[i + 1] == data[i + 3])) 
			val = data[i + 1];
		      else val = data[i + 1] + ((x - data[i]) / (data[i + 2] - data[i])) * (data[i + 3] - data[i + 1]);
		      break;
		    case ENV_EXP:
		      intrp = data[i + 1] + ((x - data[i]) / (data[i + 2] - data[i])) * (data[i + 3] - data[i + 1]);
		      val = exp(intrp * log(gen->base));
		      break;
		    }
		  return(gen->offset + (gen->scaler * val));
		}
	    }
	  if (gen->style == ENV_EXP)
	    val = exp(data[gen->size * 2 - 1] * log(gen->base));
	  else val = data[gen->size * 2 - 1];
	  return(gen->offset + (gen->scaler * val));
	}
    }
  return(gen->offset);
}




/* ---------------- frame ---------------- */

typedef struct {
  mus_any_class *core;
  int chans;
  Float *vals;
} mus_frame;

static int free_frame(mus_any *pt)
{
  mus_frame *ptr = (mus_frame *)pt;
  if (ptr)
    {
      if (ptr->vals) FREE(ptr->vals);
      FREE(ptr);
    }
  return(0);
}

static char *describe_frame(mus_any *ptr)
{
  mus_frame *gen;
  char *str = NULL;
  if (mus_frame_p(ptr))
    {
      gen = (mus_frame *)ptr;
      mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		   "frame[%d]: %s", 
		   gen->chans,
		   str = print_array(gen->vals, gen->chans, 0));
      if (str) FREE(str);
    }
  else describe_bad_gen(ptr, "frame", "a");
  return(describe_buffer);
}

static char *inspect_frame(mus_any *ptr) {return(describe_frame(ptr));}

int mus_frame_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_FRAME));}

static int equalp_frame(mus_any *p1, mus_any *p2)
{
  mus_frame *g1, *g2;
  int i;
  if (p1 == p2) return(TRUE);
  g1 = (mus_frame *)p1;
  g2 = (mus_frame *)p2;
  if (((g1->core)->type != (g2->core)->type) ||
      (g1->chans != g2->chans))
    return(FALSE);
  for (i = 0; i < g1->chans; i++)
    if (g1->vals[i] != g2->vals[i])
      return(FALSE);
  return(TRUE);
}

static Float *frame_data(mus_any *ptr) {return(((mus_frame *)ptr)->vals);}
static Float *set_frame_data(mus_any *ptr, Float *new_data) {((mus_frame *)ptr)->vals = new_data; return(new_data);}
static int frame_length(mus_any *ptr) {return(((mus_frame *)ptr)->chans);}
static int set_frame_length(mus_any *ptr, int new_len) {((mus_frame *)ptr)->chans = new_len; return(new_len);}

static mus_any_class FRAME_CLASS = {
  MUS_FRAME,
  "frame",
  &free_frame,
  &describe_frame,
  &inspect_frame,
  &equalp_frame,
  &frame_data,
  &set_frame_data,
  &frame_length,
  &set_frame_length,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  0,
  MUS_NOT_SPECIAL, 
  NULL,
  &frame_length,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_empty_frame(int chans)
{
  mus_frame *nf;
  nf = (mus_frame *)clm_calloc(1, sizeof(mus_frame), "frame");
  nf->core = &FRAME_CLASS;
  nf->chans = chans;
  nf->vals = (Float *)clm_calloc(chans, sizeof(Float), "frame data");
  return((mus_any *)nf);
}

mus_any *mus_make_frame(int chans, ...)
{
  mus_frame *nf;
  va_list ap;
  int i;
  nf = (mus_frame *)mus_make_empty_frame(chans);
  if (nf)
    {
      va_start(ap, chans);
      for (i = 0; i < chans; i++)
	nf->vals[i] = (Float)(va_arg(ap, double)); /* float not safe here apparently */
      va_end(ap);
      return((mus_any *)nf);
    }
  return(NULL);
}

mus_any *mus_frame_add(mus_any *uf1, mus_any *uf2, mus_any *ures)
{
  int chans, i;
  mus_frame *f1 = (mus_frame *)uf1;
  mus_frame *f2 = (mus_frame *)uf2;
  mus_frame *res = (mus_frame *)ures;
  chans = f1->chans;
  if (f2->chans < chans) chans = f2->chans;
  if (res)
    {
      if (res->chans < chans) chans = res->chans;
    }
  else res = (mus_frame *)mus_make_empty_frame(chans);
  for (i = 0; i < chans; i++) 
    res->vals[i] = f1->vals[i] + f2->vals[i];
  return((mus_any *)res);
}

mus_any *mus_frame_multiply(mus_any *uf1, mus_any *uf2, mus_any *ures)
{
  int chans, i;
  mus_frame *f1 = (mus_frame *)uf1;
  mus_frame *f2 = (mus_frame *)uf2;
  mus_frame *res = (mus_frame *)ures;
  chans = f1->chans;
  if (f2->chans < chans) chans = f2->chans;
  if (res)
    {
      if (res->chans < chans) chans = res->chans;
    }
  else res = (mus_frame *)mus_make_empty_frame(chans);
  for (i = 0; i < chans; i++) 
    res->vals[i] = f1->vals[i] * f2->vals[i];
  return((mus_any *)res);
}

Float mus_frame_ref(mus_any *f, int chan) {return(((mus_frame *)f)->vals[chan]);}
Float mus_frame_set(mus_any *f, int chan, Float val) {((mus_frame *)f)->vals[chan] = val; return(val);}
Float *mus_frame_data(mus_any *f) {return(((mus_frame *)f)->vals);}



/* ---------------- mixer ---------------- */

typedef struct {
  mus_any_class *core;
  int chans;
  Float **vals;
} mus_mixer;

static int free_mixer(mus_any *pt)
{
  int i;
  mus_mixer *ptr = (mus_mixer *)pt;
  if (ptr)
    {
      if (ptr->vals) 
	{
	  for (i = 0; i < ptr->chans; i++) FREE(ptr->vals[i]);
	  FREE(ptr->vals);
	}
      FREE(ptr);
    }
  return(0);
}

static char *describe_mixer(mus_any *ptr)
{
  mus_mixer *gen;
  char *str;
  int i, j, lim = 8;
  if (mus_mixer_p(ptr))
    {
      gen = (mus_mixer *)ptr;
      mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
		   "mixer: chans: %d, vals: [", gen->chans);
      str = (char *)CALLOC(64, sizeof(char));
      if (gen->chans < 8) lim = gen->chans;
      for (i = 0; i < lim; i++)
	for (j = 0; j < lim; j++)
	  {
	    mus_snprintf(str, 64, "%s%.3f%s%s",
			 (j == 0) ? "(" : "",
			 gen->vals[i][j],
			 (j == (gen->chans - 1)) ? ")" : "",
			 ((i == (gen->chans - 1)) && (j == (gen->chans - 1))) ? "]" : " ");
	    if ((strlen(describe_buffer) + strlen(str)) < (DESCRIBE_BUFFER_SIZE - 1))
	      strcat(describe_buffer, str);
	    else break;
	  }
      FREE(str);
    }
  else describe_bad_gen(ptr, "mixer", "a");
  return(describe_buffer);
}

static char *inspect_mixer(mus_any *ptr) {return(describe_mixer(ptr));}

int mus_mixer_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_MIXER));}

static int equalp_mixer(mus_any *p1, mus_any *p2)
{
  mus_mixer *g1, *g2;
  int i, j;
  if (p1 == p2) return(TRUE);
  if ((p1 == NULL) || (p2 == NULL)) return(FALSE);
  g1 = (mus_mixer *)p1;
  g2 = (mus_mixer *)p2;
  if (((g1->core)->type != (g2->core)->type) ||
      (g1->chans != g2->chans))
    return(FALSE);
  for (i = 0; i < g1->chans; i++)
    for (j = 0; j < g1->chans; j++)
      if (g1->vals[i][j] != g2->vals[i][j])
	return(FALSE);
  return(TRUE);
}

static int mixer_length(mus_any *ptr) {return(((mus_mixer *)ptr)->chans);}
static Float *mixer_data(mus_any *ptr) {return((Float *)(((mus_mixer *)ptr)->vals));}

static mus_any_class MIXER_CLASS = {
  MUS_MIXER,
  "mixer",
  &free_mixer,
  &describe_mixer,
  &inspect_mixer,
  &equalp_mixer,
  &mixer_data, 0,
  &mixer_length,
  0,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  0,
  MUS_NOT_SPECIAL, 
  NULL,
  &mixer_length,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_empty_mixer(int chans)
{
  mus_mixer *nf;
  int i;
  nf = (mus_mixer *)clm_calloc(1, sizeof(mus_mixer), "mixer");
  nf->core = &MIXER_CLASS;
  nf->chans = chans;
  nf->vals = (Float **)clm_calloc(chans, sizeof(Float *), "mixer data");
  for (i = 0; i < chans; i++)
    nf->vals[i] = (Float *)clm_calloc(chans, sizeof(Float), "mixer data");
  return((mus_any *)nf);
}

mus_any *mus_make_identity_mixer(int chans)
{
  mus_mixer *mx;
  int i;
  mx = (mus_mixer *)mus_make_empty_mixer(chans);
  if (mx) for (i = 0; i < chans; i++) mx->vals[i][i] = 1.0;
  return((mus_any *)mx);
}

mus_any *mus_make_mixer(int chans, ...)
{
  mus_mixer *mx;
  int i, j;
  va_list ap;
  if (chans <= 0) return(NULL);
  mx = (mus_mixer *)mus_make_empty_mixer(chans);
  if (mx) 
    {
      va_start(ap, chans);
      for (i = 0; i < chans; i++)
	for (j = 0; j < chans; j++)
	  mx->vals[i][j] = (Float)(va_arg(ap, double));
      va_end(ap);
    }
  return((mus_any *)mx);
}

Float **mus_mixer_data(mus_any *f) {return(((mus_mixer *)f)->vals);}
Float mus_mixer_ref(mus_any *f, int in, int out) {return(((mus_mixer *)f)->vals[in][out]);}
Float mus_mixer_set(mus_any *f, int in, int out, Float val) {((mus_mixer *)f)->vals[in][out] = val; return(val);}

mus_any *mus_frame2frame(mus_any *uf, mus_any *uin, mus_any *uout)
{
  mus_mixer *f = (mus_mixer *)uf;
  mus_frame *in = (mus_frame *)uin;
  mus_frame *out = (mus_frame *)uout;
  int i, j, in_chans, out_chans;
  in_chans = in->chans;
  if (in_chans > f->chans) in_chans = f->chans;
  out_chans = f->chans;
  if (out)
    {
      if (out->chans < out_chans) out_chans = out->chans;
    }
  else out = (mus_frame *)mus_make_empty_frame(out_chans);
  for (i = 0; i < out_chans; i++)
    {
      out->vals[i] = 0.0;
      for (j = 0; j < in_chans; j++)
	out->vals[i] += (in->vals[j] * f->vals[j][i]);
    }
  return((mus_any *)out);
}

mus_any *mus_sample2frame(mus_any *f, Float in, mus_any *uout)
{
  int i, chans;
  mus_mixer *mx;
  mus_frame *fr;
  mus_frame *out = (mus_frame *)uout;
  if (mus_frame_p(f))
    {
      fr = (mus_frame *)f;
      chans = fr->chans;
      if (out)
	{
	  if (out->chans < chans) chans = out->chans;
	}
      else out = (mus_frame *)mus_make_empty_frame(chans);
      for (i = 0; i < chans; i++)
	out->vals[i] += (in * fr->vals[i]);
    }
  else
    {
      if (mus_mixer_p(f))
	{
	  mx = (mus_mixer *)f;
	  chans = mx->chans;
	  if (out)
	    {
	      if (out->chans < chans) chans = out->chans;
	    }
	  else out = (mus_frame *)mus_make_empty_frame(chans);
	  for (i = 0; i < chans; i++)
	    out->vals[i] += (in * mx->vals[0][i]);
	}
      else mus_error(MUS_ARG_OUT_OF_RANGE,"sample2frame: gen not frame or mixer");
    }
  return((mus_any *)out);
}

Float mus_frame2sample(mus_any *f, mus_any *uin)
{
  int i, chans;
  mus_frame *in = (mus_frame *)uin;
  Float val = 0.0;
  mus_mixer *mx;
  mus_frame *fr;
  if (mus_frame_p(f))
    {
      fr = (mus_frame *)f;
      chans = in->chans;
      if (fr->chans < chans) chans = fr->chans;
      for (i = 0; i < chans; i++)
	val += (in->vals[i] * fr->vals[i]); 
    }
  else
    {
      if (mus_mixer_p(f))
	{
	  mx = (mus_mixer *)f;
	  chans = in->chans;
	  if (mx->chans < chans) chans = mx->chans;
	  for (i = 0; i < chans; i++)
	    val += (in->vals[i] * mx->vals[i][0]);
	}
      else mus_error(MUS_ARG_OUT_OF_RANGE,"frame2sample: gen not frame or mixer");
    }
  return(val);
}

mus_any *mus_mixer_multiply(mus_any *uf1, mus_any *uf2, mus_any *ures)
{
  int i, j, k, chans;
  mus_mixer *f1 = (mus_mixer *)uf1;
  mus_mixer *f2 = (mus_mixer *)uf2;
  mus_mixer *res = (mus_mixer *)ures;
  chans = f1->chans;
  if (f2->chans < chans) chans = f2->chans;
  if (res)
    {
      if (res->chans < chans) chans = res->chans;
    }
  else res = (mus_mixer *)mus_make_empty_mixer(chans);
  for (i = 0; i < chans; i++)
    for (j = 0; j < chans; j++)
      {
	res->vals[i][j] = 0.0;
	for (k = 0; k < chans; k++) 
	  res->vals[i][j] += (f1->vals[i][k] * f2->vals[k][j]);
      }
  return((mus_any *)res);
}



/* ---------------- buffer ---------------- */

typedef struct {
  mus_any_class *core;
  Float *buf;
  int size;
  int loc;
  Float fill_time;
  int empty;
  int buf_allocated;
} rblk;

static char *inspect_rblk(mus_any *ptr)
{
  rblk *gen = (rblk *)ptr;
  char *arr = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "rblk buf[%d (%s)]: %s, loc: %d, fill_time: %f, empty: %d",
	       gen->size, (gen->buf_allocated) ? "local" : "external",
	       arr = print_array(gen->buf, gen->size, 0),
	       gen->loc, gen->fill_time, gen->empty);
  if (arr) FREE(arr);
  return(describe_buffer);
}

static int mus_free_buffer(mus_any *gen) 
{
  rblk *ptr = (rblk *)gen;
  if (ptr)
    {
      if ((ptr->buf) && (ptr->buf_allocated)) FREE(ptr->buf);
      FREE(ptr);
    }
  return(0);
}

static Float *rblk_data(mus_any *ptr) {return(((rblk *)ptr)->buf);}
static int rblk_length(mus_any *ptr) {return(((rblk *)ptr)->size);}
static int rblk_set_length(mus_any *ptr, int new_size) {((rblk *)ptr)->size = new_size; return(new_size);}

int mus_buffer_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_BUFFER));}

static int rblk_equalp(mus_any *p1, mus_any *p2) 
{
  int i;
  rblk *b1 = (rblk *)p1;
  rblk *b2 = (rblk *)p2;
  if (p1 == p2) return(TRUE);
  if ((b1) && (b2) &&
      (b1->core->type == b2->core->type) &&
      (b1->size == b2->size) &&
      (b1->loc == b2->loc) &&
      (b1->fill_time == b2->fill_time) &&
      (b1->empty == b2->empty))
    {
      for (i = 0; i < b1->size; i++)
	if (b1->buf[i] != b2->buf[i])
	  return(FALSE);
      return(TRUE);
    }
  return(FALSE);
}

static Float *rblk_set_data(mus_any *ptr, Float *new_data) 
{
  rblk *rb = (rblk *)ptr;
  if (rb->buf_allocated) {FREE(rb->buf); rb->buf_allocated = FALSE;}
  rb->buf = new_data; 
  return(new_data);
}

static char *describe_genbuffer(mus_any *ptr)
{
  rblk *gen = (rblk *)ptr;
  if (mus_buffer_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
		 "buffer: length: %d, loc: %d, fill: %.3f",
		 gen->size, gen->loc, gen->fill_time);
  else describe_bad_gen(ptr, "buffer", "a");
  return(describe_buffer);
}

Float mus_buffer2sample(mus_any *ptr)
{
  rblk *gen = (rblk *)ptr;
  int loc, i, j;
  Float val;
  loc = gen->loc;
  if (loc < gen->size) 
    val = gen->buf[loc];
  else val = 0.0;
  loc++;
  if ((gen->empty == FALSE) && ((Float)loc >= gen->fill_time))
    {
      i = 0;
      for (j = loc; j < gen->size; i++, j++) gen->buf[i] = gen->buf[j];
      for (; i < gen->size; i++) gen->buf[i] = 0.0;
      gen->fill_time -= (Float)loc;
      gen->loc = 0;
      gen->empty = TRUE; 
    }
  else gen->loc = loc;
  return(val);
}

Float mus_sample2buffer(mus_any *ptr, Float val)
{
  /* place val at fill_time and increment */
  int i, j, loc, old_size;
  Float *tmp;
  rblk *gen = (rblk *)ptr;
  if (gen->fill_time >= (Float)(gen->size))
    {
      if (gen->loc == 0)
	{
	  /* have to make more space */
	  old_size = gen->size;
	  gen->size += 256;
	  /* gotta do realloc by hand -- gen->buf may not belong to us */
	  tmp = (Float *)clm_calloc(gen->size, sizeof(Float), "buffer space");
	  for (i = 0; i < old_size; i++) tmp[i] = gen->buf[i];
	  if (gen->buf_allocated) FREE(gen->buf);
	  gen->buf = tmp;
	  gen->buf_allocated = TRUE;
	}
      else
	{
	  i = 0;
	  loc = gen->loc;
	  for (j = loc; j < gen->size; i++, j++) gen->buf[i] = gen->buf[j];
	  for (; i < gen->size; i++) gen->buf[i] = 0.0;
	  gen->fill_time -= (Float)loc;
	  gen->loc = 0;
	}
    }
  gen->buf[(int)(gen->fill_time)] = val;
  gen->fill_time += 1.0;
  return(val);
}

static Float run_buffer (mus_any *ptr, Float unused1, Float unused2) {return(mus_buffer2sample(ptr));}
static Float buffer_increment(mus_any *rd) {return(((rblk *)rd)->fill_time);}
static Float buffer_set_increment(mus_any *rd, Float val) {((rblk *)rd)->fill_time = val; ((rblk *)rd)->empty = (val == 0.0); return(val);}

static mus_any_class BUFFER_CLASS = {
  MUS_BUFFER,
  "buffer",
  &mus_free_buffer,
  &describe_genbuffer,
  &inspect_rblk,
  &rblk_equalp,
  &rblk_data,
  &rblk_set_data,
  &rblk_length,
  &rblk_set_length,
  0, 0, 0, 0,
  0, 0,
  &buffer_increment,
  &buffer_set_increment,
  &run_buffer,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_buffer(Float *preloaded_buffer, int size, Float current_fill_time)
{
  rblk *gen;
  gen = (rblk *)clm_calloc(1, sizeof(rblk), "buffer");
  gen->core = &BUFFER_CLASS;
  if (size <= 0) size = 512;
  gen->size = size;
  gen->loc = 0;
  gen->fill_time = current_fill_time;
  if (preloaded_buffer)
    {
      gen->buf = preloaded_buffer;
      gen->buf_allocated = FALSE;
    }
  else
    {
      gen->buf = (Float *)clm_calloc(size, sizeof(Float), "buffer data");
      gen->buf_allocated = TRUE;
    }
  if (current_fill_time == 0) 
    gen->empty = TRUE; 
  else gen->empty = FALSE;
  return((mus_any *)gen);
}

int mus_buffer_empty_p(mus_any *ptr) {return(((rblk *)ptr)->empty);}

int mus_buffer_full_p(mus_any *ptr)
{
  rblk *gen = (rblk *)ptr;
  return((gen->fill_time >= (Float)(gen->size)) && 
	 (gen->loc == 0));
}

mus_any *mus_frame2buffer(mus_any *rb, mus_any *fr)
{
  mus_frame *frm = (mus_frame *)fr;
  int i;
  for (i = 0; i < frm->chans; i++) 
    mus_sample2buffer(rb, frm->vals[i]);
  return(fr);
}

mus_any *mus_buffer2frame(mus_any *rb, mus_any *fr)
{
  mus_frame *frm = (mus_frame *)fr;
  int i;
  for (i = 0; i < frm->chans; i++) 
    frm->vals[i] = mus_buffer2sample(rb);
  return(fr);
}



/* ---------------- wave-train ---------------- */

typedef struct {
  mus_any_class *core;
  Float freq;
  Float phase;
  Float *wave;
  int wsize;
  rblk *b;
  int wave_allocated;
} wt;

static char *inspect_wt(mus_any *ptr)
{
  wt *gen = (wt *)ptr;
  char *arr = NULL, *str = NULL;
#if HAVE_STRDUP
  str = strdup(inspect_rblk((mus_any *)(gen->b)));
#endif
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "wt freq: %f, phase: %f, wave[%d (%s)]: %s, b: %s",
	       gen->freq, gen->phase, gen->wsize, 
	       (gen->wave_allocated) ? "local" : "external",
	       arr = print_array(gen->wave, gen->wsize, 0),
	       (str) ? str : "rblk...");
  if (str) free(str);
  if (arr) FREE(arr);
  return(describe_buffer);
}

static Float wt_freq(mus_any *ptr) {return(((wt *)ptr)->freq);}
static Float set_wt_freq(mus_any *ptr, Float val) {((wt *)ptr)->freq = val; return(val);}
static Float wt_phase(mus_any *ptr) {return(fmod(((TWO_PI * ((wt *)ptr)->phase) / ((Float)((wt *)ptr)->wsize)), TWO_PI));}
static Float set_wt_phase(mus_any *ptr, Float val) {((wt *)ptr)->phase = (fmod(val, TWO_PI) * ((wt *)ptr)->wsize) / TWO_PI; return(val);}
static int wt_length(mus_any *ptr) {return(((wt *)ptr)->wsize);}
static int wt_set_length(mus_any *ptr, int val) {((wt *)ptr)->wsize = val; return(val);}
static Float *wt_data(mus_any *ptr) {return(((wt *)ptr)->wave);}

static int wt_equalp(mus_any *p1, mus_any *p2)
{
  int i;
  wt *w1 = (wt *)p1;
  wt *w2 = (wt *)p2;
  if (p1 == p2) return(TRUE);
  if ((w1) && (w2) &&
      (w1->core->type == w2->core->type) &&
      (w1->freq == w2->freq) &&
      (w1->phase == w2->phase) &&
      (w1->wsize == w2->wsize))
    {
      for (i = 0; i < w1->wsize; i++)
	if (w1->wave[i] != w2->wave[i])
	  return(FALSE);
      return(mus_equalp((mus_any *)(w1->b), (mus_any *)(w2->b)));
    }
  return(FALSE);
}

static Float *wt_set_data(mus_any *ptr, Float *data) 
{
  wt *gen = (wt *)ptr;
  if (gen->wave_allocated) {FREE(gen->wave); gen->wave_allocated = FALSE;}
  gen->wave = data; 
  return(data);
}

static char *describe_wt(mus_any *ptr)
{
  if (mus_wave_train_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "wave_train freq: %.3fHz, phase: %.3f, size: %d",
		 wt_freq(ptr), wt_phase(ptr), wt_length(ptr));
  else describe_bad_gen(ptr, "wave_train", "a");
  return(describe_buffer);
}

Float mus_wave_train(mus_any *ptr, Float fm) 
{
  wt *gen = (wt *)ptr;
  rblk *b;
  int i;
  b = gen->b;
  if (b->empty)
    {
      for (i = 0; i < b->size; i++) 
	b->buf[i] += mus_array_interp(gen->wave, gen->phase + i, gen->wsize);
      b->fill_time += ((Float)sampling_rate / (gen->freq + (fm / w_rate)));
      b->empty = FALSE;
    }
  return(mus_buffer2sample((mus_any *)(gen->b)));
}

Float mus_wave_train_1(mus_any *ptr) 
{
  wt *gen = (wt *)ptr;
  rblk *b;
  int i;
  b = gen->b;
  if (b->empty)
    {
      for (i = 0; i < b->size; i++) 
	b->buf[i] += mus_array_interp(gen->wave, gen->phase + i, gen->wsize);
      b->fill_time += ((Float)sampling_rate / gen->freq);
      b->empty = FALSE;
    }
  return(mus_buffer2sample((mus_any *)(gen->b)));
}

static Float run_wave_train(mus_any *ptr, Float fm, Float unused) {return(mus_wave_train(ptr, fm));}

static int free_wt(mus_any *p) 
{
  wt *ptr = (wt *)p;
  if (ptr) 
    {
      if ((ptr->wave) && (ptr->wave_allocated)) FREE(ptr->wave);
      if (ptr->b) mus_free_buffer((mus_any *)(ptr->b));
      FREE(ptr);
    }
  return(0);
}

static mus_any_class WAVE_TRAIN_CLASS = {
  MUS_WAVE_TRAIN,
  "wave_train",
  &free_wt,
  &describe_wt,
  &inspect_wt,
  &wt_equalp,
  &wt_data,
  &wt_set_data,
  &wt_length,
  &wt_set_length,
  &wt_freq,
  &set_wt_freq,
  &wt_phase,
  &set_wt_phase,
  0, 0,
  0, 0,
  &run_wave_train,
  MUS_NOT_SPECIAL, 
  NULL, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_wave_train(Float freq, Float phase, Float *wave, int wsize)
{
 wt *gen;
 gen = (wt *)clm_calloc(1, sizeof(wt), "wave_train");
 gen->core = &WAVE_TRAIN_CLASS;
 gen->freq = freq;
 gen->phase = (wsize * phase) / TWO_PI;
 gen->wave = wave;
 gen->wsize = wsize;
 gen->b = (rblk *)mus_make_buffer(NULL, wsize, 0.0);
 return((mus_any *)gen);
}

int mus_wave_train_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_WAVE_TRAIN));}



/* ---------------- input/output ---------------- */

static Float mus_read_sample(mus_any *fd, off_t frame, int chan) 
{
  if ((check_gen(fd, "mus-read-sample")) &&
      ((fd->core)->read_sample))
    return(((*(fd->core)->read_sample))(fd, frame, chan));
  return((Float)mus_error(MUS_NO_SAMPLE_INPUT, 
			  "can't find %s's sample input function", 
			  mus_name(fd)));
}

static Float mus_write_sample(mus_any *fd, off_t frame, int chan, Float samp) 
{
  if ((check_gen(fd, "mus-write-sample")) &&
      ((fd->core)->write_sample))
    return(((*(fd->core)->write_sample))(fd, frame, chan, samp));
  return((Float)mus_error(MUS_NO_SAMPLE_OUTPUT, 
			  "can't find %s's sample output function", 
			  mus_name(fd)));
}

char *mus_file_name(mus_any *gen)
{
  if ((check_gen(gen, "mus-file-name")) &&
      ((gen->core)->file_name))
    return((*((gen->core)->file_name))(gen));
  else mus_error(MUS_NO_FILE_NAME, "can't get %s's file name", mus_name(gen));
  return(NULL);
}


int mus_input_p(mus_any *gen) 
{
  return((gen) && 
	 ((gen->core)->extended_type == MUS_INPUT));
}

int mus_output_p(mus_any *gen) 
{
  return((gen) && 
	 ((gen->core)->extended_type == MUS_OUTPUT));
}



/* ---------------- file->sample ---------------- */

typedef struct {
  mus_any_class *core;
  int chan;
  int dir;
  off_t loc;
  char *file_name;
  int chans;
  mus_sample_t **ibufs;
  off_t data_start, data_end, file_end;
} rdin;

static char *inspect_rdin(mus_any *ptr)
{
  rdin *gen = (rdin *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "rdin chan: %d, dir: %d, loc: " OFF_TD ", chans: %d, data_start: " OFF_TD ", data_end: " OFF_TD ", file_end: " OFF_TD ", file_name: %s",
	       gen->chan, gen->dir, gen->loc, gen->chans, gen->data_start, gen->data_end, gen->file_end, gen->file_name);
  return(describe_buffer);
}

static char *describe_file2sample(mus_any *ptr)
{
  rdin *gen = (rdin *)ptr;
  if (mus_file2sample_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "file2sample: %s", 
		 gen->file_name);
  else describe_bad_gen(ptr, "file2sample", "a");
  return(describe_buffer);
}

static int rdin_equalp(mus_any *p1, mus_any *p2) 
{
  rdin *r1 = (rdin *)p1;
  rdin *r2 = (rdin *)p2;
  return ((p1 == p2) ||
	  ((r1) && (r2) &&
	   (r1->core->type == r2->core->type) &&
	   (r1->chan == r2->chan) &&
	   (r1->loc == r2->loc) &&
	   (r1->dir == r2->dir) &&
	   (r1->file_name) &&
	   (r2->file_name) &&
	   (strcmp(r1->file_name, r2->file_name) == 0)));
}

static int free_file2sample(mus_any *p) 
{
  rdin *ptr = (rdin *)p;
  if (ptr) 
    {
      if ((ptr->core)->end) ((*(ptr->core)->end))(p);
      FREE(ptr->file_name);
      FREE(ptr);
    }
  return(0);
}

static int file2sample_length(mus_any *ptr) {return((int)(((rdin *)ptr)->file_end));} /* actually off_t */
static int file2sample_channels(mus_any *ptr) {return((int)(((rdin *)ptr)->chans));}
static Float file2sample_increment(mus_any *rd) {return((Float)(((rdin *)rd)->dir));}
static Float file2sample_set_increment(mus_any *rd, Float val) {((rdin *)rd)->dir = (int)val; return(val);}
static char *file2sample_file_name(mus_any *ptr) {return(((rdin *)ptr)->file_name);}
static Float file_sample(mus_any *ptr, off_t samp, int chan);
static int file2sample_end(mus_any *ptr);

static mus_any_class FILE2SAMPLE_CLASS = {
  MUS_FILE2SAMPLE,
  "file2sample",
  &free_file2sample,
  &describe_file2sample,
  &inspect_rdin,
  &rdin_equalp,
  0, 0, 
  &file2sample_length, 0,
  0, 0, 0, 0,
  0, 0,
  &file2sample_increment, 
  &file2sample_set_increment,
  0,
  MUS_INPUT,
  NULL,
  &file2sample_channels,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  &file_sample,
  0,
  &file2sample_file_name,
  &file2sample_end,
  0, /* location */
  0, /* set_location */
  0 /* channel */
};

static Float file_sample(mus_any *ptr, off_t samp, int chan)
{
  /* check in-core buffer bounds,
   * if needed read new buffer (taking into account dir)
   * return Float at samp (frame) 
   */
  rdin *gen = (rdin *)ptr;
  int fd, i;
  off_t newloc;
  if ((samp < 0) || (samp > gen->file_end)) return(0.0);
  if ((samp > gen->data_end) || 
      (samp < gen->data_start))
    {
      /* read in first buffer start either at samp (dir > 0) or samp-bufsize (dir < 0) */
      if (gen->dir >= 0) newloc = samp; else newloc = (int)(samp - (clm_file_buffer_size * .75));
      /* The .75 in the backwards read is trying to avoid reading the full buffer on 
       * nearly every sample when we're oscillating around the
       * nominal buffer start/end (in src driven by an oscil for example)
       */
      if (newloc < 0) newloc = 0;
      gen->data_start = newloc;
      gen->data_end = newloc + clm_file_buffer_size - 1;
      fd = mus_sound_open_input(gen->file_name);
      if (fd == -1)
	return((Float)mus_error(MUS_CANT_OPEN_FILE, 
				"open(%s) -> %s", 
				gen->file_name, strerror(errno)));
      else
	{ 
	  if (gen->ibufs == NULL) 
	    {
	      gen->ibufs = (mus_sample_t **)clm_calloc(gen->chans, sizeof(mus_sample_t *), "input buffers");
	      for (i = 0; i < gen->chans; i++)
		gen->ibufs[i] = (mus_sample_t *)clm_calloc(clm_file_buffer_size, sizeof(mus_sample_t), "input buffer");
	    }
	  mus_sound_seek_frame(fd, gen->data_start);
	  mus_file_read_chans(fd, 0, clm_file_buffer_size - 1, gen->chans, gen->ibufs, (mus_sample_t *)(gen->ibufs));
	  mus_sound_close_input(fd);
	  if (gen->data_end > gen->file_end) gen->data_end = gen->file_end;
	}
    }
  return((Float)MUS_SAMPLE_TO_FLOAT(gen->ibufs[chan][samp - gen->data_start]));
}

static int file2sample_end(mus_any *ptr)
{
  rdin *gen = (rdin *)ptr;
  int i;
  if (gen)
    {
      if (gen->ibufs)
	{
	  for (i = 0; i < gen->chans; i++)
	    if (gen->ibufs[i]) 
	      FREE(gen->ibufs[i]);
	  FREE(gen->ibufs);
	  gen->ibufs = NULL;
	}
    }
  return(0);
}

int mus_file2sample_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_FILE2SAMPLE));}

mus_any *mus_make_file2sample(const char *filename)
{
  rdin *gen;
  if (filename == NULL)
    mus_error(MUS_NO_FILE_NAME_PROVIDED, S_make_file2sample " requires a file name");
  else
    {
      gen = (rdin *)clm_calloc(1, sizeof(rdin), "readin");
      gen->core = &FILE2SAMPLE_CLASS;
      gen->file_name = (char *)clm_calloc(strlen(filename) + 1, sizeof(char), "readin filename");
      strcpy(gen->file_name, filename);
      gen->data_end = -1; /* force initial read */
      gen->chans = mus_sound_chans(gen->file_name);
      if (gen->chans <= 0) mus_error(MUS_NO_CHANNELS, "%s chans: %d", filename, gen->chans);
      gen->file_end = mus_sound_frames(gen->file_name);
      if (gen->file_end < 0) mus_error(MUS_NO_LENGTH, "%s frames: " OFF_TD, filename, gen->file_end);
      return((mus_any *)gen);
    }
  return(NULL);
}

Float mus_file2sample(mus_any *ptr, off_t samp, int chan)
{
  return(mus_read_sample(ptr, samp, chan));
}



/* ---------------- readin ---------------- */

/* readin reads only the desired channel and increments the location by the direction
 *   it inherits from and specializes the file2sample class 
 */

static char *describe_readin(mus_any *ptr)
{
  rdin *gen = (rdin *)ptr;
  if (mus_readin_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "readin: %s[chan %d], loc: " OFF_TD ", dir: %d", 
		 gen->file_name, gen->chan, gen->loc, gen->dir);
  else describe_bad_gen(ptr, "readin", "a");
  return(describe_buffer);
}

static int free_readin(mus_any *p) 
{
  rdin *ptr = (rdin *)p;
  if (ptr) 
    {
      if ((ptr->core)->end) ((*(ptr->core)->end))(p);
      FREE(ptr->file_name);
      FREE(ptr);
    }
  return(0);
}

static Float run_readin(mus_any *ptr, Float unused1, Float unused2) {return(mus_readin(ptr));}
static Float rd_increment(mus_any *ptr) {return((Float)(((rdin *)ptr)->dir));}
static Float rd_set_increment(mus_any *ptr, Float val) {((rdin *)ptr)->dir = (int)val; return(val);}
static off_t rd_location(mus_any *rd) {return(((rdin *)rd)->loc);}
static off_t rd_set_location(mus_any *rd, off_t loc) {((rdin *)rd)->loc = loc; return(loc);}
static int rd_channel(mus_any *rd) {return(((rdin *)rd)->chan);}

static mus_any_class READIN_CLASS = {
  MUS_READIN,
  "readin",
  &free_readin,
  &describe_readin,
  &inspect_rdin,
  &rdin_equalp,
  0, 0, 
  &file2sample_length, 0,
  0, 0, 0, 0,
  0, 0,
  &rd_increment,
  &rd_set_increment,
  &run_readin,
  MUS_INPUT,
  NULL,
  &file2sample_channels,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  &file_sample,
  0,
  &file2sample_file_name,
  &file2sample_end,
  &rd_location,
  &rd_set_location,
  &rd_channel
};

int mus_readin_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_READIN));}

mus_any *mus_make_readin(const char *filename, int chan, off_t start, int direction)
{
  rdin *gen;
  gen = (rdin *)mus_make_file2sample(filename);
  if (gen)
    {
      gen->core = &READIN_CLASS;
      gen->loc = start;
      gen->dir = direction;
      gen->chan = chan;
      gen->ibufs = (mus_sample_t **)clm_calloc(gen->chans, sizeof(mus_sample_t *), "readin buffers");
      gen->ibufs[chan] = (mus_sample_t *)clm_calloc(clm_file_buffer_size, sizeof(mus_sample_t), "readin buffer");
      return((mus_any *)gen);
    }
  return(NULL);
}

Float mus_readin(mus_any *ptr)
{
  Float res;
  rdin *rd = (rdin *)ptr;
  res = mus_file2sample(ptr, rd->loc, rd->chan);
  rd->loc += rd->dir;
  return(res);
}

off_t mus_set_location(mus_any *gen, off_t loc)
{
  if ((check_gen(gen, S_setB S_mus_location)) &&
      ((gen->core)->set_location))
    return((*((gen->core)->set_location))(gen, loc));
  return((off_t)mus_error(MUS_NO_LOCATION, "can't set %s's location", mus_name(gen)));
}



/* ---------------- in-any ---------------- */

Float mus_in_any(off_t frame, int chan, mus_any *IO)
{
  if (IO) return(mus_file2sample(IO, frame, chan));
  return(0.0);
}

Float mus_ina(off_t frame, mus_any *inp) {return(mus_in_any(frame, 0, inp));}
Float mus_inb(off_t frame, mus_any *inp) {return(mus_in_any(frame, 1, inp));}



/* ---------------- file->frame ---------------- */

/* also built on file->sample */

static char *describe_file2frame(mus_any *ptr)
{
  rdin *gen = (rdin *)ptr;
  if (mus_file2frame_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "file2frame: %s", 
		 gen->file_name);
  else describe_bad_gen(ptr, "file2frame", "a");
  return(describe_buffer);
}

static mus_any_class FILE2FRAME_CLASS = {
  MUS_FILE2FRAME,
  "file2frame",
  &free_file2sample,
  &describe_file2frame,
  &inspect_rdin,
  &rdin_equalp,
  0, 0, 
  &file2sample_length, 0,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  0,
  MUS_INPUT,
  NULL,
  &file2sample_channels,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  &file_sample,
  0,
  &file2sample_file_name,
  &file2sample_end,
  0, /* location */
  0, /* set_location */
  0 /* channel */
};

mus_any *mus_make_file2frame(const char *filename)
{
  rdin *gen;
  gen = (rdin *)mus_make_file2sample(filename);
  if (gen) 
    {
      gen->core = &FILE2FRAME_CLASS;
      return((mus_any *)gen);
    }
  return(NULL);
}

int mus_file2frame_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_FILE2FRAME));}

mus_any *mus_file2frame(mus_any *ptr, off_t samp, mus_any *uf)
{
  mus_frame *f;
  rdin *gen = (rdin *)ptr;
  int i;
  if (uf == NULL) f = (mus_frame *)mus_make_empty_frame(gen->chans); else f = (mus_frame *)uf;
  for (i = 0; i < gen->chans; i++) 
    f->vals[i] = mus_file2sample(ptr, samp, i);
  return((mus_any *)f);
}


/* ---------------- sample->file ---------------- */

/* in all output functions, the assumption is that we're adding to whatever already exists */
/* also, the "end" methods need to flush the output buffer */

typedef struct {
  mus_any_class *core;
  int chan;
  off_t loc;
  char *file_name;
  int chans;
  mus_sample_t **obufs;
  off_t data_start, data_end;
  off_t out_end;
  int output_data_format;
  int output_header_type;
} rdout;

static char *inspect_rdout(mus_any *ptr)
{
  rdout *gen = (rdout *)ptr;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "rdout chan: %d, loc: " OFF_TD ", file_name: %s, chans: %d, data_start: " OFF_TD ", data_end: " OFF_TD ", out_end: " OFF_TD,
	       gen->chan, gen->loc, gen->file_name, gen->chans, gen->data_start, gen->data_end, gen->out_end);
  return(describe_buffer);
}

static char *describe_sample2file(mus_any *ptr)
{
  rdout *gen = (rdout *)ptr;
  if (mus_sample2file_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "sample2file: %s", 
		 gen->file_name);
  else describe_bad_gen(ptr, "sample2file", "a");
  return(describe_buffer);
}

static int sample2file_equalp(mus_any *p1, mus_any *p2) {return(p1 == p2);}

static int free_sample2file(mus_any *p) 
{
  rdout *ptr = (rdout *)p;
  if (ptr) 
    {
      if ((ptr->core)->end) ((*(ptr->core)->end))(p);
      FREE(ptr->file_name);
      FREE(ptr);
    }
  return(0);
}

static int sample2file_channels(mus_any *ptr) {return((int)(((rdout *)ptr)->chans));}
static int bufferlen(mus_any *ptr) {return(clm_file_buffer_size);}
static int set_bufferlen(mus_any *ptr, int len) {clm_file_buffer_size = len; return(len);}
static char *sample2file_file_name(mus_any *ptr) {return(((rdout *)ptr)->file_name);}
static Float sample_file(mus_any *ptr, off_t samp, int chan, Float val);
static int sample2file_end(mus_any *ptr);

static mus_any_class SAMPLE2FILE_CLASS = {
  MUS_SAMPLE2FILE,
  "sample2file",
  &free_sample2file,
  &describe_sample2file,
  &inspect_rdout,
  &sample2file_equalp,
  0, 0, 
  &bufferlen, &set_bufferlen, /* does this have any effect on the current gen? */
  0, 0, 0, 0,
  0, 0,
  0, 0,
  0,
  MUS_OUTPUT,
  NULL,
  &sample2file_channels,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0,
  &sample_file,
  &sample2file_file_name,
  &sample2file_end,
  0, 0, 0
};

static void flush_buffers(rdout *gen)
{
  int fd, i, j, last, hdrfrm, hdrtyp, num;
  off_t size, hdrend;
  mus_sample_t **addbufs;
  if ((gen->obufs == NULL) || (mus_file_probe(gen->file_name) == 0))
    return;
  fd = mus_sound_open_input(gen->file_name);
  if (fd == -1)
    {
      fd = mus_sound_open_output(gen->file_name, 
				 (int)sampling_rate, 
				 gen->chans, 
				 gen->output_data_format,
				 gen->output_header_type, 
				 NULL);
      if (fd == -1)
	mus_error(MUS_CANT_OPEN_FILE, 
		  "open(%s) -> %s", 
		  gen->file_name, strerror(errno));
      else
	{
	  mus_sound_write(fd, 0, gen->out_end, gen->chans, gen->obufs);
	  mus_sound_close_output(fd, 
				 (gen->out_end + 1) * 
				 gen->chans * 
				 mus_bytes_per_sample(mus_sound_data_format(gen->file_name)));	  
	}
    }
  else
    {
      hdrend = mus_sound_data_location(gen->file_name);
      hdrfrm = mus_sound_data_format(gen->file_name);
      hdrtyp = mus_sound_header_type(gen->file_name);
      size = mus_sound_frames(gen->file_name);
      addbufs = (mus_sample_t **)clm_calloc(gen->chans, sizeof(mus_sample_t *), "output buffers");
      for (i = 0; i < gen->chans; i++) 
	addbufs[i] = (mus_sample_t *)clm_calloc(clm_file_buffer_size, sizeof(mus_sample_t), "output buffer");
      mus_sound_seek_frame(fd, gen->data_start);
      num = gen->out_end - gen->data_start;
      if (num >= clm_file_buffer_size) 
	num = clm_file_buffer_size - 1;
      mus_sound_read(fd, 0, num, gen->chans, addbufs);
      mus_sound_close_input(fd);
      fd = mus_sound_reopen_output(gen->file_name, gen->chans, hdrfrm, hdrtyp, hdrend);
      last = gen->out_end - gen->data_start;
      for (j = 0; j < gen->chans; j++)
	for (i = 0; i <= last; i++)
	  addbufs[j][i] += gen->obufs[j][i];
      mus_sound_seek_frame(fd, gen->data_start);
      mus_sound_write(fd, 0, last, gen->chans, addbufs);
      if (size <= gen->out_end) size = gen->out_end + 1;
      mus_sound_close_output(fd, size * gen->chans * mus_bytes_per_sample(hdrfrm));
      for (i = 0; i < gen->chans; i++) FREE(addbufs[i]);
      FREE(addbufs);
    }
}

static Float sample_file(mus_any *ptr, off_t samp, int chan, Float val)
{
  rdout *gen = (rdout *)ptr;
  int j;
  if (chan < gen->chans)
    {
      if ((samp > gen->data_end) || 
	  (samp < gen->data_start))
	{
	  flush_buffers(gen);
	  for (j = 0; j < gen->chans; j++)
	    memset((void *)(gen->obufs[j]), 0, clm_file_buffer_size * sizeof(mus_sample_t));
	  gen->data_start = samp;
	  gen->data_end = samp + clm_file_buffer_size - 1;
	  gen->out_end = samp;
	}
      gen->obufs[chan][samp - gen->data_start] += MUS_FLOAT_TO_SAMPLE(val);
      if (samp > gen->out_end) 
	gen->out_end = samp;
    }
  /* It would be useful if this returned the new value MUS_SAMPLE_TO_FLOAT(gen->obufs[chan][samp - gen->data_start])
   *   because we could break on overflow in an instrument, or watch the overall output, but
   *   that's inconsistent with locsig (but locsig could return a frame of the updated values),
   *   and requires a second type conversion.  Perhaps a parallel output set?
   */
  return(val);
}

static int sample2file_end(mus_any *ptr)
{
  rdout *gen = (rdout *)ptr;
  int i;
  if ((gen) && (gen->obufs))
    {
      flush_buffers(gen);
      for (i = 0; i < gen->chans; i++)
	if (gen->obufs[i]) 
	  FREE(gen->obufs[i]);
      FREE(gen->obufs);
      gen->obufs = NULL;
    }
  return(0);
}

int mus_sample2file_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_SAMPLE2FILE));}

static mus_any *mus_make_sample2file_with_comment_1(const char *filename, int out_chans, int out_format, int out_type, const char *comment, int reopen)
{
  rdout *gen;
  int i, fd;
  if (filename == NULL)
    mus_error(MUS_NO_FILE_NAME_PROVIDED, S_make_sample2file " requires a file name");
  else
    {
      if (reopen)
	fd = mus_sound_reopen_output(filename, 
				     out_chans, 
				     out_format, 
				     out_type, 
				     mus_sound_data_location(filename));
      else fd = mus_sound_open_output(filename, 
				      (int)sampling_rate, 
				      out_chans, 
				      out_format, 
				      out_type, 
				      comment);
      if (fd == -1)
	mus_error(MUS_CANT_OPEN_FILE, 
		  "open(%s) -> %s", 
		  filename, strerror(errno));
      else
	{
	  gen = (rdout *)clm_calloc(1, sizeof(rdout), "output");
	  gen->core = &SAMPLE2FILE_CLASS;
	  gen->file_name = (char *)clm_calloc(strlen(filename) + 1, sizeof(char), "output filename");
	  strcpy(gen->file_name, filename);
	  gen->data_start = 0;
	  gen->data_end = clm_file_buffer_size - 1;
	  gen->out_end = 0;
	  gen->chans = out_chans;
	  gen->output_data_format = out_format;
	  gen->output_header_type = out_type;
	  gen->obufs = (mus_sample_t **)clm_calloc(gen->chans, sizeof(mus_sample_t *), "output buffers");
	  for (i = 0; i < gen->chans; i++) 
	    gen->obufs[i] = (mus_sample_t *)clm_calloc(clm_file_buffer_size, sizeof(mus_sample_t), "output buffer");
	  /* clear previous, if any */
	  if (mus_file_close(fd) != 0)
	    mus_error(MUS_CANT_CLOSE_FILE, 
		      "close(%d, %s) -> %s", 
		      fd, gen->file_name, strerror(errno));
	  return((mus_any *)gen);
	}
    }
  return(NULL);
}

mus_any *mus_continue_sample2file(const char *filename)
{
  return(mus_make_sample2file_with_comment_1(filename,
					     mus_sound_chans(filename),
					     mus_sound_data_format(filename),
					     mus_sound_header_type(filename),
					     NULL,
					     TRUE));
}

mus_any *mus_make_sample2file_with_comment(const char *filename, int out_chans, int out_format, int out_type, const char *comment)
{
  return(mus_make_sample2file_with_comment_1(filename, out_chans, out_format, out_type, comment, FALSE));
}

mus_any *mus_make_sample2file(const char *filename, int out_chans, int out_format, int out_type)
{
  return(mus_make_sample2file_with_comment_1(filename, out_chans, out_format, out_type, NULL, FALSE));
}

Float mus_sample2file(mus_any *ptr, off_t samp, int chan, Float val)
{
  return(mus_write_sample(ptr, samp, chan, val));
}

int mus_close_file(mus_any *ptr)
{
  rdout *gen = (rdout *)ptr;
  if ((mus_output_p(ptr)) && (gen->obufs)) sample2file_end(ptr);
  return(0);
}


/* ---------------- out-any ---------------- */

Float mus_out_any(off_t frame, Float val, int chan, mus_any *IO)
{
  if (IO) return(mus_sample2file(IO, frame, chan, val));
  return(0.0);
}

Float mus_outa(off_t frame, Float val, mus_any *IO) {return(mus_out_any(frame, val, 0, IO));}
Float mus_outb(off_t frame, Float val, mus_any *IO) {return(mus_out_any(frame, val, 1, IO));}
Float mus_outc(off_t frame, Float val, mus_any *IO) {return(mus_out_any(frame, val, 2, IO));}
Float mus_outd(off_t frame, Float val, mus_any *IO) {return(mus_out_any(frame, val, 3, IO));}



/* ---------------- frame->file ---------------- */

static char *describe_frame2file(mus_any *ptr)
{
  rdout *gen = (rdout *)ptr;
  if (mus_frame2file_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "frame2file: %s", 
		 gen->file_name);
  else describe_bad_gen(ptr, "frame2file", "a");
  return(describe_buffer);
}

static mus_any_class FRAME2FILE_CLASS = {
  MUS_FRAME2FILE,
  "frame2file",
  &free_sample2file,
  &describe_frame2file,
  &inspect_rdout,
  &sample2file_equalp,
  0, 0,
  &bufferlen, &set_bufferlen,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  0,
  MUS_OUTPUT,
  NULL,
  &sample2file_channels,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0,
  &sample_file,
  &sample2file_file_name,
  &sample2file_end,
  0, 0, 0
};

mus_any *mus_make_frame2file(const char *filename, int chans, int out_format, int out_type)
{
  rdout *gen;
  gen = (rdout *)mus_make_sample2file(filename, chans, out_format, out_type);
  if (gen) 
    {
      gen->core = &FRAME2FILE_CLASS;
      return((mus_any *)gen);
    }
  return(NULL);
}

int mus_frame2file_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_FRAME2FILE));}

mus_any *mus_frame2file(mus_any *ptr, off_t samp, mus_any *udata)
{
  rdout *gen = (rdout *)ptr;
  mus_frame *data = (mus_frame *)udata;
  int i, chans;
  if (data->chans == 1)
    mus_sample2file(ptr, samp, 0, data->vals[0]);
  else
    {
      chans = data->chans;
      if (gen->chans < chans) chans = gen->chans;
      for (i = 0; i < chans; i++) 
	mus_sample2file(ptr, samp, i, data->vals[i]);
    }
  return((mus_any *)data);
}



/* ---------------- locsig ---------------- */

/* always return a frame */

typedef struct {
  mus_any_class *core;
  mus_any *outn_writer;
  mus_any *revn_writer;
  mus_frame *outf, *revf;
  Float *outn;
  Float *revn;
  int chans, rev_chans, type;
  Float reverb;
} locs;

static char *inspect_locs(mus_any *ptr)
{
  locs *gen = (locs *)ptr;
  char *arr1 = NULL, *arr2 = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
	       "locs outn[%d]: %s, revn[%d]: %s",
	       gen->chans,
	       arr1 = print_array(gen->outn, gen->chans, 0),
	       gen->rev_chans,
	       arr2 = print_array(gen->revn, gen->rev_chans, 0));
  if (arr1) FREE(arr1);
  if (arr2) FREE(arr2);
  return(describe_buffer);
}

static int locsig_equalp(mus_any *p1, mus_any *p2) 
{
  locs *g1 = (locs *)p1;
  locs *g2 = (locs *)p2;
  int i;
  if (p1 == p2) return(TRUE);
  if ((g1) && (g2) &&
      (g1->core->type == g2->core->type) &&
      (g1->chans == g2->chans))
    {
      for (i = 0; i < g1->chans; i++) 
	if (g1->outn[i] != g2->outn[i]) 
	  return(FALSE);
      if (g1->revn)
	{
	  if (!(g2->revn)) return(FALSE);
	  if (g1->revn[0] != g2->revn[0]) return(FALSE);
	}
      else 
	if (g2->revn) 
	  return(FALSE);
      return(TRUE);
    }
  return(FALSE);
}

static char *describe_locsig(mus_any *ptr)
{
  #define STR_SIZE 32
  char *str;
  int i, lim = 16;
  locs *gen = (locs *)ptr;
  if (mus_locsig_p(ptr))
    {
      mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
		   "locsig: chans %d, outn: [", 
		   gen->chans);
      str = (char *)CALLOC(STR_SIZE, sizeof(char));
      if (gen->chans - 1 < lim) lim = gen->chans - 1;
      for (i = 0; i < lim; i++)
	{
	  mus_snprintf(str, STR_SIZE, "%.3f ", gen->outn[i]);
	  if ((strlen(describe_buffer) + strlen(str)) < (DESCRIBE_BUFFER_SIZE - 16))
	    strcat(describe_buffer, str);
	  else break;
	}
      if (gen->chans - 1 > lim) strcat(describe_buffer, "...");
      mus_snprintf(str, STR_SIZE, "%.3f]", gen->outn[gen->chans - 1]);
      strcat(describe_buffer, str);
      if (gen->rev_chans > 0)
	{
	  strcat(describe_buffer, ", revn: [");
	  lim = 16;
	  if (gen->rev_chans - 1 < lim) lim = gen->rev_chans - 1;
	  for (i = 0; i < lim; i++)
	    {
	      mus_snprintf(str, STR_SIZE, "%.3f ", gen->revn[i]);
	      if ((strlen(describe_buffer) + strlen(str)) < (DESCRIBE_BUFFER_SIZE - 16))
		strcat(describe_buffer, str);
	      else break;
	    }
	  if (gen->rev_chans - 1 > lim) strcat(describe_buffer, "...");
	  mus_snprintf(str, STR_SIZE, "%.3f]", gen->revn[gen->rev_chans - 1]);
	  strcat(describe_buffer, str);
	}
      FREE(str);
    }
  else describe_bad_gen(ptr, "locsig", "a");
  return(describe_buffer);
}

static int free_locsig(mus_any *p) 
{
  locs *ptr = (locs *)p;
  if (ptr) 
    {
      if (ptr->outn) FREE(ptr->outn);
      if (ptr->revn) FREE(ptr->revn);
      mus_free((mus_any *)(ptr->outf));
      if (ptr->revf) mus_free((mus_any *)(ptr->revf));
      FREE(ptr);
    }
  return(0);
}

static int locsig_length(mus_any *ptr) {return(((locs *)ptr)->chans);}
static Float *locsig_data(mus_any *ptr) {return(((locs *)ptr)->outn);}

Float mus_locsig_ref (mus_any *ptr, int chan) 
{
  locs *gen = (locs *)ptr;
  if ((ptr) && (mus_locsig_p(ptr))) 
    {
      if ((chan >= 0) && 
	  (chan < gen->chans))
	return(gen->outn[chan]);
      else mus_error(MUS_NO_SUCH_CHANNEL, 
		     "locsig_ref chan %d >= %d", 
		     chan, gen->chans);
    }
  return(0.0);
}

Float mus_locsig_set (mus_any *ptr, int chan, Float val) 
{
  locs *gen = (locs *)ptr;
  if ((ptr) && (mus_locsig_p(ptr))) 
    {
      if ((chan >= 0) && 
	  (chan < gen->chans))
	gen->outn[chan] = val;
      else mus_error(MUS_NO_SUCH_CHANNEL, 
		     "locsig_set chan %d >= %d", 
		     chan, gen->chans);
    }
  return(val);
}

Float mus_locsig_reverb_ref (mus_any *ptr, int chan) 
{
  locs *gen = (locs *)ptr;
  if ((ptr) && (mus_locsig_p(ptr))) 
    {
      if ((chan >= 0) && 
	  (chan < gen->rev_chans))
	return(gen->revn[chan]);
      else mus_error(MUS_NO_SUCH_CHANNEL, 
		     "locsig_reverb_ref chan %d, but this locsig has %d reverb chans", 
		     chan, gen->rev_chans);
    }
  return(0.0);
}

Float mus_locsig_reverb_set (mus_any *ptr, int chan, Float val) 
{
  locs *gen = (locs *)ptr;
  if ((ptr) && (mus_locsig_p(ptr))) 
    {
      if ((chan >= 0) && 
	  (chan < gen->rev_chans))
	gen->revn[chan] = val;
      else mus_error(MUS_NO_SUCH_CHANNEL, 
		     "locsig_reverb_set chan %d >= %d", 
		     chan, gen->rev_chans);
    }
  return(val);
}

static mus_any_class LOCSIG_CLASS = {
  MUS_LOCSIG,
  "locsig",
  &free_locsig,
  &describe_locsig,
  &inspect_locs,
  &locsig_equalp,
  &locsig_data, 0,
  &locsig_length,
  0,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  0,
  MUS_OUTPUT,
  NULL,
  &locsig_length,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

int mus_locsig_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_LOCSIG));}

void mus_fill_locsig(Float *arr, int chans, Float degree, Float scaler, int type)
{
  Float deg, pos, frac, degs_per_chan, ldeg, c, s;
  int left, right;
  if (chans == 1)
    arr[0] = scaler;
  else
    {
      if (degree < 0.0)
	{
	  /* sigh -- hope for the best... */
	  int m;
	  m = (int)ceil(degree / -360.0);
	  degree += (360 * m);
	}
      if (chans == 2)
	{
	  if (degree > 90.0)
	    deg = 90.0;
	  else
	    {
	      if (degree < 0.0)
		deg = 0.0;
	      else deg = degree;
	    }
	  degs_per_chan = 90.0;
	}
      else 
	{
	  deg = fmod(degree, 360.0);
	  degs_per_chan = 360.0 / chans;
	}
      pos = deg / degs_per_chan;
      left = (int)floor(pos);
      right = left + 1;
      if (right == chans) right = 0;
      frac = pos - left;
      if (type == MUS_LINEAR)
	{
	  arr[left] = scaler * (1.0 - frac);
	  arr[right] = scaler * frac;
	}
      else
	{
	  ldeg = M_PI_2 * (0.5 - frac);
	  scaler *= sqrt(2.0) / 2.0;
	  c = cos(ldeg);
	  s = sin(ldeg);
	  arr[left] = scaler * (c + s);
	  arr[right] = scaler * (c - s);
	}
    }
}

mus_any *mus_make_locsig(Float degree, Float distance, Float reverb, int chans, mus_any *output, mus_any *revput, int type)
{
  locs *gen;
  Float dist;
  if (chans <= 0)
    {
      mus_error(MUS_ARG_OUT_OF_RANGE, "chans: %d", chans);
      return(NULL);
    }
  gen = (locs *)clm_calloc(1, sizeof(locs), "locsig");
  gen->core = &LOCSIG_CLASS;
  gen->outf = (mus_frame *)mus_make_empty_frame(chans);
  gen->chans = chans;
  gen->type = type;
  gen->reverb = reverb;
  if (distance > 1.0)
    dist = 1.0 / distance;
  else dist = 1.0;
  if (output) gen->outn_writer = output;
  if (revput) 
    {
      gen->revn_writer = revput;
      gen->rev_chans = mus_channels(revput);
      if (gen->rev_chans > 0)
	{
	  gen->revn = (Float *)clm_calloc(gen->rev_chans, sizeof(Float), "locsig reverb frame");
	  gen->revf = (mus_frame *)mus_make_empty_frame(gen->rev_chans);
	  mus_fill_locsig(gen->revn, gen->rev_chans, degree, (reverb * sqrt(dist)), type);
	}
    }
  else gen->rev_chans = 0;
  gen->outn = (Float *)clm_calloc(chans, sizeof(Float), "locsig frame");
  mus_fill_locsig(gen->outn, chans, degree, dist, type);
  return((mus_any *)gen);
}

mus_any *mus_locsig(mus_any *ptr, off_t loc, Float val)
{
  locs *gen = (locs *)ptr;
  int i;
  for (i = 0; i < gen->chans; i++)
    (gen->outf)->vals[i] = val * gen->outn[i];
  for (i = 0; i < gen->rev_chans; i++)
    (gen->revf)->vals[i] = val * gen->revn[i];
  if (gen->revn_writer)
    mus_frame2file(gen->revn_writer, loc, (mus_any *)(gen->revf));
  if (gen->outn_writer)
    return(mus_frame2file(gen->outn_writer, loc, (mus_any *)(gen->outf)));
  else return((mus_any *)(gen->outf));
}

void mus_move_locsig(mus_any *ptr, Float degree, Float distance)
{
  locs *gen = (locs *)ptr;
  Float dist;
  if (distance > 1.0)
    dist = 1.0 / distance;
  else dist = 1.0;
  if (gen->rev_chans > 0)
    mus_fill_locsig(gen->revn, gen->rev_chans, degree, (gen->reverb * sqrt(dist)), gen->type);
  mus_fill_locsig(gen->outn, gen->chans, degree, dist, gen->type);
}


/* ---------------- src ---------------- */

/* sampling rate conversion */
/* taken from sweep_srate.c of Perry Cook.  To quote Perry:
 *
 * 'The conversion is performed by sinc interpolation.
 *    J. O. Smith and P. Gossett, "A Flexible Sampling-Rate Conversion Method," 
 *    Proc. of the IEEE Conference on Acoustics, Speech, and Signal Processing, San Diego, CA, March, 1984.
 * There are essentially two cases, one where the conversion factor
 * is less than one, and the sinc table is used as is yielding a sound
 * which is band limited to the 1/2 the new sampling rate (we don't
 * want to create bandwidth where there was none).  The other case
 * is where the conversion factor is greater than one and we 'warp'
 * the sinc table to make the final cutoff equal to the original sampling
 * rate /2.  Warping the sinc table is based on the similarity theorem
 * of the time and frequency domain, stretching the time domain (sinc
 * table) causes shrinking in the frequency domain.'
 *
 * we also scale the amplitude if interpolating to take into account the broadened sinc 
 *   this means that isolated pulses get scaled by 1/src, but that's a dumb special case
 */

typedef struct {
  mus_any_class *core;
  Float (*feeder)(void *arg, int direction);
  Float x;
  Float incr, width_1;
  int width, lim;
  int len;
  Float *data, *sinc_table;
  void *environ;
} sr;

static char *inspect_sr(mus_any *ptr)
{
  sr *gen = (sr *)ptr;
  char *arr = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "sr x: %f, incr: %f, width: %d, sinc table len: %d, data[%d]: %s",
	       gen->x, gen->incr, gen->width, gen->len, gen->width * 2 + 1,
	       arr = print_array(gen->data, gen->width * 2 + 1, 0));
  if (arr) FREE(arr);
  return(describe_buffer);
}

#define SRC_SINC_DENSITY 1000
#define SRC_SINC_WIDTH 10

static Float **sinc_tables = NULL;
static int *sinc_widths = NULL;
static int sincs = 0;

void mus_clear_sinc_tables(void)
{
  int i;
  if (sincs)
    {
      for (i = 0; i < sincs; i++) 
	if (sinc_tables[i]) 
	  FREE(sinc_tables[i]);
      FREE(sinc_tables);
      sinc_tables = NULL;
      FREE(sinc_widths);
      sinc_widths = NULL;
      sincs = 0;
    }
}

static Float *init_sinc_table(int width)
{
  int i, size, loc;
  Float sinc_freq, win_freq, sinc_phase, win_phase;
  for (i = 0; i < sincs; i++)
    if (sinc_widths[i] == width)
      return(sinc_tables[i]);
  if (sincs == 0)
    {
      sinc_tables = (Float **)clm_calloc(8, sizeof(Float *), "sinc tables");
      sinc_widths = (int *)clm_calloc(8, sizeof(int), "sinc tables");
      sincs = 8;
      loc = 0;
    }
  else
    {
      loc = -1;
      for (i = 0; i < sincs; i++)
	if (sinc_widths[i] == 0)
	  {
	    loc = i;
	    break;
	  }
      if (loc == -1)
	{
	  sinc_tables = (Float **)REALLOC(sinc_tables, (sincs + 8) * sizeof(Float *));
	  sinc_widths = (int *)REALLOC(sinc_widths, (sincs + 8) * sizeof(int));
	  for (i = sincs; i < (sincs + 8); i++) 
	    {
	      sinc_widths[i] = 0; 
	      sinc_tables[i] = NULL;
	    }
	  loc = sincs;
	  sincs += 8;
	}
    }
  sinc_tables[loc] = (Float *)clm_calloc(width * SRC_SINC_DENSITY + 2, sizeof(Float), "sinc table");
  sinc_widths[loc] = width;
  size = width * SRC_SINC_DENSITY;
  sinc_freq = M_PI / (Float)SRC_SINC_DENSITY;
  win_freq = M_PI / (Float)size;
  sinc_tables[loc][0] = 1.0;
  for (i = 1, sinc_phase = sinc_freq, win_phase = win_freq; i < size; i++, sinc_phase += sinc_freq, win_phase += win_freq)
    sinc_tables[loc][i] = sin(sinc_phase) * (0.5 + 0.5 * cos(win_phase)) / sinc_phase;
  return(sinc_tables[loc]);
}

int mus_src_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_SRC));}

static int free_src_gen(mus_any *srptr)
{
  sr *srp = (sr *)srptr;
  if (srp)
    {
      if (srp->data) FREE(srp->data);
      FREE(srp);
    }
  return(0);
}

static int src_equalp(mus_any *p1, mus_any *p2) {return(p1 == p2);}

static char *describe_src(mus_any *ptr)
{
  sr *gen = (sr *)ptr;
  if (mus_src_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
		 "src: width: %d, x: %.3f, incr: %.3f, sinc table len: %d",
		 gen->width, gen->x, gen->incr, gen->len);
  else describe_bad_gen(ptr, "src", "an");
  return(describe_buffer);
}

static int src_length(mus_any *ptr) {return(((sr *)ptr)->width);}
static Float run_src_gen(mus_any *srptr, Float sr_change, Float unused) {return(mus_src(srptr, sr_change, NULL));}
static void *src_environ(mus_any *rd) {return(((sr *)rd)->environ);}
static Float src_increment(mus_any *rd) {return(((sr *)rd)->incr);}
static Float src_set_increment(mus_any *rd, Float val) {((sr *)rd)->incr = val; return(val);}
static Float *src_sinc_table(mus_any *rd) {return(((sr *)rd)->sinc_table);}

static mus_any_class SRC_CLASS = {
  MUS_SRC,
  "src",
  &free_src_gen,
  &describe_src,
  &inspect_sr,
  &src_equalp,
  &src_sinc_table, 0,
  &src_length,  /* sinc width actually */
  0,
  0, 0, 0, 0,
  0, 0,
  &src_increment,
  &src_set_increment,
  &run_src_gen,
  MUS_NOT_SPECIAL,
  &src_environ,
  0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_src(Float (*input)(void *arg, int direction), Float srate, int width, void *environ)
{
  sr *srp;
  int i, wid;
  if (fabs(srate) > (Float)(1 << 16))
    mus_error(MUS_ARG_OUT_OF_RANGE, S_make_src " srate arg invalid: %f", srate);
  else
    {
      if ((width < 0) || (width > (1 << 16)))
	mus_error(MUS_ARG_OUT_OF_RANGE, S_make_src " width arg invalid: %d", width);
      else
	{
	  srp = (sr *)clm_calloc(1, sizeof(sr), "src");
	  if (width == 0) width = SRC_SINC_WIDTH;
	  if (width < (fabs(srate) * 2)) 
	    wid = (int)(ceil(fabs(srate)) * 2); 
	  else wid = width;
	  srp->core = &SRC_CLASS;
	  srp->x = 0.0;
	  srp->feeder = input;
	  srp->environ = environ;
	  srp->incr = srate;
	  srp->width = wid;
	  srp->lim = 2 * wid;
	  srp->len = wid * SRC_SINC_DENSITY;
	  srp->data = (Float *)clm_calloc(srp->lim + 1, sizeof(Float), "src table");
	  srp->sinc_table = init_sinc_table(wid);
	  for (i = wid - 1; i < srp->lim; i++) 
	    srp->data[i] = (*input)(environ, (srate >= 0.0) ? 1 : -1);
	  /* was i = 0 here but we want the incoming data centered */
	  srp->width_1 = 1.0 - wid;
	  return((mus_any *)srp);
	}
    }
  return(NULL);
}

Float mus_src(mus_any *srptr, Float sr_change, Float (*input)(void *arg, int direction))
{
  sr *srp = (sr *)srptr;
  Float sum = 0.0, x, zf, srx, factor;
  int fsx, lim, i, k, loc;
  int xi, xs, int_ok = FALSE;
  lim = srp->lim;
  srx = srp->incr + sr_change;
  if (srp->x >= 1.0)
    {
      fsx = (int)(srp->x);
      srp->x -= fsx;
      /* realign data, reset srp->x */
      if (fsx > lim)
	{
	  /* if sr_change is so extreme that the new index falls outside the data table, we need to
	   *   read forward until we reach the new data bounds
	   */
	  for (i = lim; i < fsx; i++)
	    {
	      if (input)
		(*input)(srp->environ, (srx >= 0.0) ? 1 : -1);
	      else (*(srp->feeder))(srp->environ, (srx >= 0.0) ? 1 : -1);
	    }
	  fsx = lim;
	}
#if (!HAVE_MEMMOVE)
      for (i = fsx, loc = 0; i < lim; i++, loc++) 
	srp->data[loc] = srp->data[i];
#else
      loc = lim - fsx;
      if (loc > 0)
	memmove((void *)(srp->data), (void *)(srp->data + fsx), sizeof(Float) * loc);
#endif
      for (i = loc; i < lim; i++) 
	{
	  if (input)
	    srp->data[i] = (*input)(srp->environ, (srx >= 0.0) ? 1 : -1);
	  else srp->data[i] = (*(srp->feeder))(srp->environ, (srx >= 0.0) ? 1 : -1);
	}
    }
  /* if (srx == 0.0) srx = 0.01; */ /* can't decide about this ... */
  if (srx < 0.0) srx = -srx;
  /* tedious timing tests indicate that precalculating this block in the sr_change=0 case saves no time at all */
  if (srx > 1.0) 
    {
      factor = 1.0 / srx;
      /* this is not exact since we're sampling the sinc and so on, but it's close over a wide range */
      zf = factor * (Float)SRC_SINC_DENSITY; 
      xi = (int)zf;
      int_ok = ((zf - xi) < .001);
    }
  else 
    {
      factor = 1.0;
      zf = (Float)SRC_SINC_DENSITY;
      xi = SRC_SINC_DENSITY;
      int_ok = TRUE;
    }

  if (int_ok)
    {
      xs = (int)(zf * (srp->width_1 - srp->x));
      i = 0;
      if (xs < 0)
	for (; (i < lim) && (xs < 0); i++, xs += xi)
	  sum += (srp->data[i] * srp->sinc_table[-xs]);
      for (; i < lim; i++, xs += xi)
	sum += (srp->data[i] * srp->sinc_table[xs]);
    }
  else
    {
      /* this form twice as slow because of float->int conversions */
      for (i = 0, x = zf * (srp->width_1 - srp->x); i < lim; i++, x += zf)
	{
	  /* we're moving backwards in the data array, so the sr->x field has to mimic that (hence the '1.0 - srp->x') */
	  if (x < 0) k = (int)(-x); else k = (int)x;
	  sum += (srp->data[i] * srp->sinc_table[k]);
	  /* rather than do a bounds check here, we just padded the sinc_table above with 2 extra 0's */
	}
    }
  srp->x += srx;
  return(sum * factor);
}


/* it was a cold, rainy day... */
Float mus_src_20(mus_any *srptr, Float (*input)(void *arg, int direction));
Float mus_src_20(mus_any *srptr, Float (*input)(void *arg, int direction))
{
  sr *srp = (sr *)srptr;
  Float sum;
  int lim, i, loc;
  int xi, xs;
  lim = srp->lim;
  if (srp->x > 0.0)
    {
      /* realign data, reset srp->x */
#if (!HAVE_MEMMOVE)
      for (i = 2, loc = 0; i < lim; i++, loc++) 
	srp->data[loc] = srp->data[i];
#else
      loc = lim - 2;
      memmove((void *)(srp->data), (void *)(srp->data + 2), sizeof(Float) * loc);
#endif
      for (i = loc; i < lim; i++) 
	{
	  if (input)
	    srp->data[i] = (*input)(srp->environ, 1);
	  else srp->data[i] = (*(srp->feeder))(srp->environ, 1);
	}
    }
  else srp->x = 2.0;
  xi = (int)(SRC_SINC_DENSITY / 2);
  xs = xi * (1 - srp->width);
  xi *= 2;
  sum = srp->data[srp->width - 1];
  for (i = 0; (i < lim) && (xs < 0); i += 2, xs += xi)
    sum += (srp->data[i] * srp->sinc_table[-xs]);
  for (; i < lim; i += 2, xs += xi)
    sum += (srp->data[i] * srp->sinc_table[xs]);
  return(sum * 0.5);
}

Float mus_src_05(mus_any *srptr, Float (*input)(void *arg, int direction));
Float mus_src_05(mus_any *srptr, Float (*input)(void *arg, int direction))
{
  sr *srp = (sr *)srptr;
  Float sum;
  int lim, i, loc;
  int xs;
  lim = srp->lim;
  if (srp->x >= 1.0)
    {
#if (!HAVE_MEMMOVE)
      for (i = 1, loc = 0; i < lim; i++, loc++) 
	srp->data[loc] = srp->data[i];
#else
      loc = lim - 1;
      memmove((void *)(srp->data), (void *)(srp->data + 1), sizeof(Float) * loc);
#endif
      for (i = loc; i < lim; i++) 
	{
	  if (input)
	    srp->data[i] = (*input)(srp->environ, 1);
	  else srp->data[i] = (*(srp->feeder))(srp->environ, 1);
	}
      srp->x = 0.0;
    }
  if (srp->x == 0.0)
    {
      srp->x = 0.5;
      return(srp->data[srp->width - 1]);
    }
  xs = (int)(SRC_SINC_DENSITY * (srp->width_1 - 0.5));
  for (i = 0, sum = 0.0; (i < lim) && (xs < 0); i++, xs += SRC_SINC_DENSITY)
    sum += (srp->data[i] * srp->sinc_table[-xs]);
  for (; i < lim; i++, xs += SRC_SINC_DENSITY)
    sum += (srp->data[i] * srp->sinc_table[xs]);
  srp->x += 0.5;
  return(sum);
}




/* ---------------- granulate ---------------- */

typedef struct {
  mus_any_class *core;
  Float (*rd)(void *arg, int direction);
  int s20;
  int s50;
  int rmp;
  Float amp;
  int len;
  int cur_out;
  int cur_in;
  int input_hop;
  int ctr;
  int output_hop;
  Float *data;
  Float *in_data;
  int block_len;
  int in_data_len;
  int in_data_start;
  void *environ;
} grn_info;

static char *inspect_grn_info(mus_any *ptr)
{
  grn_info *gen = (grn_info *)ptr;
  char *arr1 = NULL, *arr2 = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "grn_info s20: %d, s50: %d, rmp: %d, amp: %f, len: %d, cur_out: %d, cur_in: %d, input_hop: %d, ctr: %d, output_hop: %d, in_data_start: %d, in_data[%d]: %s, data[%d]: %s",
	       gen->s20, gen->s50, gen->rmp, gen->amp, gen->len, gen->cur_out, gen->cur_in, gen->input_hop,
	       gen->ctr, gen->output_hop, gen->in_data_start,
	       gen->in_data_len,
	       arr1 = print_array(gen->in_data, gen->in_data_len, 0),
	       gen->block_len,
	       arr2 = print_array(gen->data, gen->block_len, 0));
  if (arr1) FREE(arr1);
  if (arr2) FREE(arr2);
  return(describe_buffer);
}

int mus_granulate_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_GRANULATE));}

static int granulate_equalp(mus_any *p1, mus_any *p2) {return(p1 == p2);}

static char *describe_granulate(mus_any *ptr)
{
  grn_info *gen = (grn_info *)ptr;
  if (mus_granulate_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
		 "granulate: expansion: %.3f (%d/%d), scaler: %.3f, length: %.3f secs (%d samps), ramp: %.3f",
		 (Float)(gen->output_hop) / (Float)(gen->input_hop),
		 gen->input_hop, gen->output_hop,
		 gen->amp,
		 (Float)(gen->len) / (Float)sampling_rate, gen->len,
		 (Float)(gen->rmp) / (Float)sampling_rate);
  else describe_bad_gen(ptr, "granulate", "a");
  return(describe_buffer);
}

static int free_granulate(mus_any *ptr)
{
  grn_info *gen = (grn_info *)ptr;
  if (gen)
    {
      if (gen->data) FREE(gen->data);
      if (gen->in_data) FREE(gen->in_data);
      FREE(gen);
    }
  return(0);
}

static int grn_length(mus_any *ptr) {return(((grn_info *)ptr)->len);}
static int grn_set_length(mus_any *ptr, int val) 
{
  grn_info *gen = ((grn_info *)ptr);
  if (val < gen->block_len) gen->len = val; /* larger -> segfault */
  return(gen->len);
}
static Float grn_scaler(mus_any *ptr) {return(((grn_info *)ptr)->amp);}
static Float grn_set_scaler(mus_any *ptr, Float val) {((grn_info *)ptr)->amp = val; return(val);}
static Float grn_frequency(mus_any *ptr) {return(((Float)((grn_info *)ptr)->output_hop) / (Float)sampling_rate);}
static Float grn_set_frequency(mus_any *ptr, Float val) {((grn_info *)ptr)->output_hop = (int)((Float)sampling_rate * val); return(val);}
static void *grn_environ(mus_any *rd) {return(((grn_info *)rd)->environ);}
static Float grn_increment(mus_any *rd) {return(((Float)(((grn_info *)rd)->output_hop)) / ((Float)((grn_info *)rd)->input_hop));}
static Float grn_set_increment(mus_any *rd, Float val) {((grn_info *)rd)->input_hop = (int)(((grn_info *)rd)->output_hop / val); return(val);}
static int grn_hop(mus_any *ptr) {return(((grn_info *)ptr)->output_hop);}
static int grn_set_hop(mus_any *ptr, int val) {((grn_info *)ptr)->output_hop = val; return(val);}
static int grn_ramp(mus_any *ptr) {return(((grn_info *)ptr)->rmp);}
static int grn_set_ramp(mus_any *ptr, int val) 
{
  grn_info *gen = (grn_info *)ptr; 
  if (val < (gen->len * .5))
    gen->rmp = val;
  return(val);
}

static Float run_granulate(mus_any *ptr, Float unused1, Float unused2) {return(mus_granulate(ptr, NULL));}

static mus_any_class GRANULATE_CLASS = {
  MUS_GRANULATE,
  "granulate",
  &free_granulate,
  &describe_granulate,
  &inspect_grn_info,
  &granulate_equalp,
  0, 0,
  &grn_length,  /* segment-length */
  &grn_set_length,
  &grn_frequency, /* spd-out */
  &grn_set_frequency,
  0, 0,
  &grn_scaler, /* segment-scaler */
  &grn_set_scaler,
  &grn_increment,
  &grn_set_increment,
  &run_granulate,
  MUS_NOT_SPECIAL,
  &grn_environ,
  0,
  0, 0, 0, 0, 0, 0, 
  &grn_hop, &grn_set_hop, 
  &grn_ramp, &grn_set_ramp,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_granulate(Float (*input)(void *arg, int direction), 
			    Float expansion, Float length, Float scaler, 
			    Float hop, Float ramp, Float jitter, int max_size, void *environ)
{
  grn_info *spd;
  int outlen;
  outlen = (int)(sampling_rate * (hop + length));
  if (max_size > outlen) outlen = max_size;
  if (outlen <= 0) 
    {
      mus_error(MUS_NO_LENGTH, S_make_granulate " size is %d (hop: %f, segment-length: %f)?", outlen, hop, length);
      return(NULL);
    }
  spd = (grn_info *)clm_calloc(1, sizeof(grn_info), "granulate");
  spd->core = &GRANULATE_CLASS;
  spd->cur_out = 0;
  spd->cur_in = 0;
  spd->len = (int)(ceil(length * sampling_rate));
  spd->rmp = (int)(ramp * spd->len);
  spd->amp = scaler;
  spd->output_hop = (int)(hop * sampling_rate);
  spd->input_hop = (int)((Float)(spd->output_hop) / expansion);
  spd->s20 = (int)(jitter * sampling_rate / 20);
  spd->s50 = (int)(jitter * sampling_rate / 50);
  spd->ctr = 0;
  spd->block_len = outlen;
  spd->data = (Float *)clm_calloc(outlen, sizeof(Float), "granulate out data");
  spd->in_data_len = outlen + spd->s20 + 1;
  spd->in_data = (Float *)clm_calloc(spd->in_data_len, sizeof(Float), "granulate in data");
  spd->in_data_start = spd->in_data_len;
  spd->rd = input;
  spd->environ = environ;
  return((mus_any *)spd);
}

Float mus_granulate(mus_any *ptr, Float (*input)(void *arg, int direction))
{ 
  grn_info *spd = (grn_info *)ptr;
  int start, end, len, extra, i, j, k, steady_end, curstart;
  Float amp, incr, result;
  result = spd->data[spd->ctr];
  spd->ctr++;
  if (spd->ctr >= spd->cur_out)
    {
      start = spd->cur_out;
      end = spd->len - start;
      if (end < 0) end = 0;
      if (end > 0) 
	for (i = 0, j = start; i < end; i++, j++) 
	  spd->data[i] = spd->data[j];
      for (i = end; i < spd->block_len; i++) 
	spd->data[i] = 0.0;

      start = spd->in_data_start;
      len = spd->in_data_len;
      if (start > len)
	{
	  extra = start - len;
	  if (input)
	    for (i = 0; i < extra; i++) (*input)(spd->environ, 1);
	  else for (i = 0; i < extra; i++) (*(spd->rd))(spd->environ, 1);
	  start = len;
	}
      if (start < len)
	{
	  for (i = 0, k = start; k < len; i++, k++)
	    spd->in_data[i] = spd->in_data[k];
	}
      if (input)
	for (i = (len - start); i < len; i++) spd->in_data[i] = (*input)(spd->environ, 1);
      else for (i = (len - start); i < len; i++) spd->in_data[i] = (*(spd->rd))(spd->environ, 1);
      spd->in_data_start = spd->input_hop;

      amp = 0.0;
      incr = (Float)(spd->amp) / (Float)(spd->rmp);
      steady_end = (spd->len - spd->rmp);
      curstart = irandom(spd->s20);
      for (i = 0, j = curstart; i < spd->len; i++, j++)
	{
	  spd->data[i] += (amp * spd->in_data[j]);
	  if (i < spd->rmp) 
	    amp += incr; 
	  else 
	    if (i > steady_end) 
	      amp -= incr;
	}
      
      spd->ctr -= spd->cur_out;
      /* spd->cur_out = spd->output_hop + irandom(spd->s50); */             /* this was the original form */
      spd->cur_out = spd->output_hop + irandom(spd->s50) - (spd->s50 >> 1); /* this form suggested by Marc Lehmann */
      if (spd->cur_out < 0) spd->cur_out = 0;
    }
  return(result);
}


/* ---------------- convolve ---------------- */

/* fft and convolution of Float data in zero-based arrays
 */

#if HAVE_FFTW3
static double *rdata = NULL, *idata = NULL;
static fftw_plan rplan, iplan;
static int last_fft_size = 0;

void mus_fftw(Float *rl, int n, int dir)
{
  int i;
  if (n != last_fft_size)
    {
      if (rdata) {fftw_free(rdata); fftw_free(idata); fftw_destroy_plan(rplan); fftw_destroy_plan(iplan);}
      rdata = (double *)fftw_malloc(n * sizeof(double));
      idata = (double *)fftw_malloc(n * sizeof(double));
      rplan = fftw_plan_r2r_1d(n, rdata, idata, FFTW_R2HC, FFTW_ESTIMATE); 
      iplan = fftw_plan_r2r_1d(n, rdata, idata, FFTW_HC2R, FFTW_ESTIMATE);
      last_fft_size = n;
    }
  memset((void *)idata, 0, n * sizeof(double));
  for (i = 0; i < n; i++) {rdata[i] = rl[i];}
  if (dir != -1)
    fftw_execute(rplan);
  else fftw_execute(iplan);
  for (i = 0; i < n; i++) rl[i] = idata[i];
}
#else
#if HAVE_FFTW
static fftw_real *rdata = NULL, *idata = NULL;
static rfftw_plan rplan, iplan;
static int last_fft_size = 0;

void mus_fftw(Float *rl, int n, int dir)
{
  int i;
  if (n != last_fft_size)
    {
      if (rdata) {FREE(rdata); FREE(idata); rfftw_destroy_plan(rplan); rfftw_destroy_plan(iplan);}
      rplan = rfftw_create_plan(n, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE); /* I didn't see any improvement here from using FFTW_MEASURE */
      iplan = rfftw_create_plan(n, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
      last_fft_size = n;
      rdata = (fftw_real *)CALLOC(n, sizeof(fftw_real));
      idata = (fftw_real *)CALLOC(n, sizeof(fftw_real));
    }
  memset((void *)idata, 0, n * sizeof(fftw_real));
  /* if Float (default float) == fftw_real (default double) we could forego the data copy */
  for (i = 0; i < n; i++) {rdata[i] = rl[i];}
  if (dir != -1)
    rfftw_one(rplan, rdata, idata);
  else rfftw_one(iplan, rdata, idata);
  for (i = 0; i < n; i++) rl[i] = idata[i];
}
#endif
#endif

static void mus_scramble (Float* rl, Float* im, int n)
{
  /* bit reversal */

  int i, m, j;
  Float vr, vi;
  j = 0;
  for (i = 0; i < n; i++)
    {
      if (j > i)
	{
	  vr = rl[j];
	  vi = im[j];
	  rl[j] = rl[i];
	  im[j] = im[i];
	  rl[i] = vr;
	  im[i] = vi;
	}
      m = n >> 1;
      while ((m >= 2) && (j >= m))
	{
	  j -= m;
	  m = m >> 1;
	}
      j += m;
    }
}

void mus_fft(Float *rl, Float *im, int n, int is)
{
  /* standard fft: real part in rl, imaginary in im,
   * rl and im are zero-based.
   * see fxt/simplfft/fft.c (Joerg Arndt) 
   */
  int m, j, mh, ldm, lg, i, i2, j2, imh;
  double ur, ui, u, vr, vi, angle, c, s;
  imh = (int)(log(n + 1) / log(2.0));
  mus_scramble(rl, im, n);
  m = 2;
  ldm = 1;
  mh = n >> 1;
  angle = (M_PI * is);
  for (lg = 0; lg < imh; lg++)
    {
      c = cos(angle);
      s = sin(angle);
      ur = 1.0;
      ui = 0.0;
      for (i2 = 0; i2 < ldm; i2++)
	{
	  i = i2;
	  j = i2 + ldm;
	  for (j2 = 0; j2 < mh; j2++)
	    {
	      vr = ur * rl[j] - ui * im[j];
	      vi = ur * im[j] + ui * rl[j];
	      rl[j] = rl[i] - vr;
	      im[j] = im[i] - vi;
	      rl[i] += vr;
	      im[i] += vi;
	      i += m;
	      j += m;
	    }
	  u = ur;
	  ur = (ur * c) - (ui * s);
	  ui = (ui * c) + (u * s);
	}
      mh >>= 1;
      ldm = m;
      angle *= 0.5;
      m <<= 1;
    }
}

#if HAVE_GSL
#include <gsl/gsl_sf_bessel.h>
static double mus_bessi0(Float x)
{
  gsl_sf_result res;
  gsl_sf_bessel_I0_e(x, &res);
  return(res.val);
}
#else
static double mus_bessi0(Float x)
{ 
  Float z, denominator, numerator;
  if (x == 0.0) return(1.0);
  if (fabs(x) <= 15.0) 
    {
      z = x*x;
      numerator=(z * (z * (z * (z * (z * (z * (z * (z * (z * (z * (z * (z * (z * (z * 
										  0.210580722890567e-22 + 0.380715242345326e-19) +
									     0.479440257548300e-16) + 0.435125971262668e-13) +
								   0.300931127112960e-10) + 0.160224679395361e-7) +
							 0.654858370096785e-5) + 0.202591084143397e-2) +
					       0.463076284721000e0) + 0.754337328948189e2) +
				     0.830792541809429e4) + 0.571661130563785e6) +
			   0.216415572361227e8) + 0.356644482244025e9) +
		 0.144048298227235e10);
      denominator=(z * (z * (z - 0.307646912682801e4) +
			0.347626332405882e7) - 0.144048298227235e10);
      return(-numerator / denominator);
    } 
  return(1.0);
}
#endif

static Float sqr(Float x) {return(x*x);}

Float *mus_make_fft_window_with_window(int type, int size, Float beta, Float *window)
{
  /* mostly taken from
   *    Fredric J. Harris, "On the Use of Windows for Harmonic Analysis with the
   *    Discrete Fourier Transform," Proceedings of the IEEE, Vol. 66, No. 1,
   *    January 1978.
   *    Albert H. Nuttall, "Some Windows with Very Good Sidelobe Behaviour", 
   *    IEEE Transactions of Acoustics, Speech, and Signal Processing, Vol. ASSP-29,
   *    No. 1, February 1981, pp 84-91
   *
   * JOS had slightly different numbers for the Blackman-Harris windows.
   */
  int i, j, midn, midp1;
  Float freq, rate, sr1, angle, expn, expsum, I0beta, cx;
#if HAVE_GSL
  Float *rl, *im;
  Float pk;
#endif
  if (window == NULL) return(NULL);
  midn = size >> 1;
  midp1 = (size + 1) / 2;
  freq = TWO_PI / (Float)size;
  rate = 1.0 / (Float)midn;
  angle = 0.0;
  expn = log(2) / (Float)midn + 1.0;
  expsum = 1.0;
  switch (type)
    {
    case MUS_RECTANGULAR_WINDOW:
      for (i = 0; i < size; i++) window[i] = 1.0;
      break; 
    case MUS_HANN_WINDOW:
      for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) window[j] = (window[i] = 0.5 - 0.5 * cos(angle));
      break; 
    case MUS_WELCH_WINDOW:
      for (i = 0, j = size - 1; i <= midn; i++, j--) window[j] = (window[i] = 1.0 - sqr((Float)(i - midn) / (Float)midp1));
      break; 
    case MUS_PARZEN_WINDOW:
      for (i = 0, j = size - 1; i <= midn; i++, j--) window[j] = (window[i] = 1.0 - fabs((Float)(i - midn) / (Float)midp1));
      break; 
    case MUS_BARTLETT_WINDOW:
      for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += rate) window[j] = (window[i] = angle);
      break; 
    case MUS_HAMMING_WINDOW:
      for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) window[j] = (window[i] = 0.54 - 0.46 * cos(angle));
      break; 
    case MUS_BLACKMAN2_WINDOW: /* using Chebyshev polynomial equivalents here */
      for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) 
	{              /* (+ 0.42323 (* -0.49755 (cos a)) (* 0.07922 (cos (* a 2)))) */
	  cx = cos(angle);
	  window[j] = (window[i] = (.34401 + (cx * (-.49755 + (cx * .15844)))));
	}
      break; 
    case MUS_BLACKMAN3_WINDOW:
      for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) 
	{              /* (+ 0.35875 (* -0.48829 (cos a)) (* 0.14128 (cos (* a 2))) (* -0.01168 (cos (* a 3)))) */
	               /* (+ 0.36336 (*  0.48918 (cos a)) (* 0.13660 (cos (* a 2))) (*  0.01064 (cos (* a 3)))) is "Nuttall" window? */

	  cx = cos(angle);
	  window[j] = (window[i] = (.21747 + (cx * (-.45325 + (cx * (.28256 - (cx * .04672)))))));
	}
      break; 
    case MUS_BLACKMAN4_WINDOW:
      for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) 
	{             /* (+ 0.287333 (* -0.44716 (cos a)) (* 0.20844 (cos (* a 2))) (* -0.05190 (cos (* a 3))) (* 0.005149 (cos (* a 4)))) */
	  cx = cos(angle);
	  window[j] = (window[i] = (.084037 + (cx * (-.29145 + (cx * (.375696 + (cx * (-.20762 + (cx * .041194)))))))));
	}
      break; 
    case MUS_EXPONENTIAL_WINDOW:
      for (i = 0, j = size - 1; i <= midn; i++, j--) {window[j] = (window[i] = expsum - 1.0); expsum *= expn;}
      break;
    case MUS_KAISER_WINDOW:
      I0beta = mus_bessi0(beta); /* Harris multiplies beta by pi */
      for (i = 0, j = size - 1, angle = 1.0; i <= midn; i++, j--, angle -= rate) 
	window[j] = (window[i] = mus_bessi0(beta * sqrt(1.0 - sqr(angle))) / I0beta);
      break;
    case MUS_CAUCHY_WINDOW:
      for (i = 0, j = size - 1, angle = 1.0; i <= midn; i++, j--, angle -= rate) window[j] = (window[i] = 1.0 / (1.0 + sqr(beta * angle)));
      break;
    case MUS_POISSON_WINDOW:
      for (i = 0, j = size - 1, angle = 1.0; i <= midn; i++, j--, angle -= rate) window[j] = (window[i] = exp((-beta) * angle));
      break;
    case MUS_RIEMANN_WINDOW:
      sr1 = TWO_PI / (Float)size;
      for (i = 0, j = size - 1; i <= midn; i++, j--) 
	{
	  if (i == midn) 
	    window[j] = (window[i] = 1.0);
	  else 
	    {
	      cx = sr1 * (midn - i); /* split out because NeXT C compiler can't handle the full expression */
	      window[i] = sin(cx) / cx;
	      window[j] = window[i];
	    }
	}
      break;
    case MUS_GAUSSIAN_WINDOW:
      for (i = 0, j = size - 1, angle = 1.0; i <= midn; i++, j--, angle -= rate) window[j] = (window[i] = exp(-.5 * sqr(beta * angle)));
      break;
    case MUS_TUKEY_WINDOW:
      cx = midn * (1.0 - beta);
      for (i = 0, j = size - 1; i <= midn; i++, j--) 
	{
	  if (i >= cx) 
	    window[j] = (window[i] = 1.0);
	  else window[j] = (window[i] = .5 * (1.0 - cos(M_PI * i / cx)));
	}
      break;
    case MUS_DOLPH_CHEBYSHEV_WINDOW:
#if HAVE_GSL
      {
	gsl_complex val;
	Float den, alpha;
	freq = M_PI / (Float)size;
	if (beta < 0.2) beta = 0.2;
	alpha = GSL_REAL(gsl_complex_cosh(
			   gsl_complex_mul_real(
			     gsl_complex_arccosh_real(pow(10.0, beta)),
			     (double)(1.0 / (Float)size))));
	den = 1.0 / GSL_REAL(gsl_complex_cosh(
			       gsl_complex_mul_real(
                                 gsl_complex_arccosh_real(alpha),
				 (double)size)));
	/* den(ominator) not really needed -- we're normalizing to 1.0 */
	rl = (Float *)clm_calloc(size, sizeof(Float), "Dolph-Chebyshev buffer");
	im = (Float *)clm_calloc(size, sizeof(Float), "Dolph-Chebyshev buffer");
	for (i = 0, angle = 0.0; i < size; i++, angle += freq)
	  {
	    val = gsl_complex_mul_real(
		    gsl_complex_cos(
		      gsl_complex_mul_real(
		        gsl_complex_arccos_real(alpha * cos(angle)),
		        (double)size)),
		    den);
	    rl[i] = GSL_REAL(val);
	    im[i] = GSL_IMAG(val); /* always essentially 0.0 */
	  }
	mus_fft(rl, im, size, -1);    /* can be 1 here */
	rl[size / 2] = 0.0;
	pk = 0.0;
	for (i = 0; i < size; i++) 
	  if (pk < rl[i]) 
	    pk = rl[i];
	if ((pk != 0.0) && (pk != 1.0))
	  for (i = 0, j = size / 2; i < size; i++) 
	    {
	      window[i] = rl[j++] / pk;
	      if (j == size) j = 0;
	    }
	FREE(rl);
	FREE(im);
      }
#else
      mus_error(MUS_NO_SUCH_FFT_WINDOW, "Dolph-Chebyshev window needs the complex trig support from GSL");
#endif
      break;
    default: 
      mus_error(MUS_NO_SUCH_FFT_WINDOW, "unknown fft data window: %d", type); 
      break;
    }
  return(window);
}

Float *mus_make_fft_window(int type, int size, Float beta)
{
  return(mus_make_fft_window_with_window(type, size, beta, (Float *)clm_calloc(size, sizeof(Float), "fft window")));
}

Float *mus_spectrum(Float *rdat, Float *idat, Float *window, int n, int type)
{
  int i;
  Float maxa, todb, lowest;
  double val;
  if (window) mus_multiply_arrays(rdat, window, n);
  mus_clear_array(idat, n);
  mus_fft(rdat, idat, n, 1);
  lowest = 0.000001;
  maxa = 0.0;
  n = (int)(n * 0.5);
  for (i = 0; i < n; i++)
    {
#if (__linux__ && __PPC__)
      if (rdat[i] < lowest) rdat[i] = 0.0;
      if (idat[i] < lowest) idat[i] = 0.0;
#endif
      val = rdat[i] * rdat[i] + idat[i] * idat[i];
      if (val < lowest)
	rdat[i] = 0.001;
      else 
	{
	  rdat[i] = sqrt(val);
	  if (rdat[i] > maxa) maxa = rdat[i];
	}
    }
  if (maxa > 0.0)
    {
      maxa = 1.0 / maxa;
      if (type == 0) /* dB */
	{
	  todb = 20.0 / log(10.0);
	  for (i = 0; i < n; i++) 
	    rdat[i] = todb * log(rdat[i] * maxa);
	}
      else
	{
	  if (type == 1) /* linear, normalized */
	    for (i = 0; i < n; i++) 
	      rdat[i] *= maxa;
	}
    }
  return(rdat);
}

Float *mus_convolution (Float* rl1, Float* rl2, int n)
{
  /* convolves two real arrays.                                           */
  /* rl1 and rl2 are assumed to be set up correctly for the convolution   */
  /* (that is, rl1 (the "signal") is zero-padded by length of             */
  /* (non-zero part of) rl2 and rl2 is stored in wrap-around order)       */
  /* We treat rl2 as the imaginary part of the first fft, then do         */
  /* the split, scaling, and (complex) spectral multiply in one step.     */
  /* result in rl1                                                        */

  int j, n2, nn2;
  Float rem, rep, aim, aip, invn;

  mus_fft(rl1, rl2, n, 1);
  
  n2 = n >> 1;
  invn = 0.25 / (Float)n;
  rl1[0] = ((rl1[0] * rl2[0]) / (Float)n);
  rl2[0] = 0.0;

  for (j = 1; j <= n2; j++)
    {
      nn2 = n - j;
      rep = (rl1[j] + rl1[nn2]);
      rem = (rl1[j] - rl1[nn2]);
      aip = (rl2[j] + rl2[nn2]);
      aim = (rl2[j] - rl2[nn2]);

      rl1[j] = invn * (rep * aip + aim * rem);
      rl1[nn2] = rl1[j];
      rl2[j] = invn * (aim * aip - rep * rem);
      rl2[nn2] = -rl2[j];
    }
  
  mus_fft(rl1, rl2, n, -1);
  return(rl1);
}


typedef struct {
  mus_any_class *core;
  Float (*feeder)(void *arg, int direction);
  int fftsize, fftsize2, ctr, filtersize;
  Float *rl1, *rl2, *buf, *filter; 
  void *environ;
} conv;

static char *inspect_conv(mus_any *ptr)
{
  conv *gen = (conv *)ptr;
  char *arr1 = NULL, *arr2 = NULL, *arr3 = NULL, *arr4 = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "conv fftsize: %d, fftsize2: %d, filtersize: %d, ctr: %d, rl1: %s, rl2: %s, buf: %s, filter: %s",
	       gen->fftsize, gen->fftsize2, gen->filtersize, gen->ctr,
	       arr1 = print_array(gen->rl1, gen->fftsize, 0),
	       arr2 = print_array(gen->rl2, gen->fftsize, 0),
	       arr3 = print_array(gen->buf, gen->fftsize, 0),
	       arr4 = print_array(gen->filter, gen->filtersize, 0));
  if (arr1) FREE(arr1);
  if (arr2) FREE(arr2);
  if (arr3) FREE(arr3);
  if (arr4) FREE(arr4);
  return(describe_buffer);
}

static int convolve_equalp(mus_any *p1, mus_any *p2) {return(p1 == p2);}

static char *describe_convolve(mus_any *ptr)
{
  conv *gen = (conv *)ptr;
  if (mus_convolve_p(ptr))
    mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE, 
		 "convolve: size: %d", 
		 gen->fftsize);
  else describe_bad_gen(ptr, "convolve", "a");
  return(describe_buffer);
}

static int free_convolve(mus_any *ptr)
{
  conv *gen = (conv *)ptr;
  if (gen)
    {
      if (gen->rl1) FREE(gen->rl1);
      if (gen->rl2) FREE(gen->rl2);
      if (gen->buf) FREE(gen->buf);
      FREE(gen);
    }
  return(0);
}

static int conv_length(mus_any *ptr) {return(((conv *)ptr)->fftsize);}
static Float run_convolve(mus_any *ptr, Float unused1, Float unused2) {return(mus_convolve(ptr, NULL));}
static void *conv_environ(mus_any *rd) {return(((conv *)rd)->environ);}

static mus_any_class CONVOLVE_CLASS = {
  MUS_CONVOLVE,
  "convolve",
  &free_convolve,
  &describe_convolve,
  &inspect_conv,
  &convolve_equalp,
  0, 0,
  &conv_length,
  0,
  0, 0, 0, 0,
  0, 0,
  0, 0,
  &run_convolve,
  MUS_NOT_SPECIAL,
  &conv_environ,
  0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0
};

Float mus_convolve(mus_any *ptr, Float (*input)(void *arg, int direction))
{
  conv *gen = (conv *)ptr;
  Float result;
  int i, j;
  if (gen->ctr >= gen->fftsize2)
    {
      for (i = 0, j = gen->fftsize2; i < gen->fftsize2; i++, j++) 
	{
	  gen->buf[i] = gen->buf[j]; 
	  gen->buf[j] = 0.0;
	  if (input)
	    gen->rl1[i] = (*input)(gen->environ, 1);
	  else gen->rl1[i] = (*(gen->feeder))(gen->environ, 1);
	  gen->rl1[j] = 0.0;
	  gen->rl2[i] = 0.0;
	  gen->rl2[j] = 0.0;
	}
      for (i = 0; i < gen->filtersize; i++) gen->rl2[i] = gen->filter[i];
      mus_convolution(gen->rl1, gen->rl2, gen->fftsize);
      for (i = 0, j = gen->fftsize2; i < gen->fftsize2; i++, j++) 
	{
	  gen->buf[i] += gen->rl1[i];
	  gen->buf[j] = gen->rl1[j];
	}
      gen->ctr = 0;
    }
  result = gen->buf[gen->ctr];
  gen->ctr++;
  return(result);
}

int mus_convolve_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_CONVOLVE));}

mus_any *mus_make_convolve(Float (*input)(void *arg, int direction), Float *filter, int fftsize, int filtersize, void *environ)
{
  conv *gen = NULL;
  gen = (conv *)clm_calloc(1, sizeof(conv), "convolve");
  gen->core = &CONVOLVE_CLASS;
  gen->feeder = input;
  gen->environ = environ;
  gen->filter = filter;
  gen->filtersize = filtersize;
  gen->fftsize = fftsize;
  gen->fftsize2 = gen->fftsize / 2;
  gen->ctr = gen->fftsize2;
  gen->rl1 = (Float *)clm_calloc(fftsize, sizeof(Float), "convolve fft data");
  gen->rl2 = (Float *)clm_calloc(fftsize, sizeof(Float), "convolve fft data");
  gen->buf = (Float *)clm_calloc(fftsize, sizeof(Float), "convolve fft data");
  return((mus_any *)gen);
}

void mus_convolve_files(const char *file1, const char *file2, Float maxamp, const char *output_file)
{
  off_t file1_len, file2_len;
  int fftlen, outlen, totallen, file1_chans, file2_chans, output_chans, c1, c2;
  Float *data1, *data2, *outdat;
  mus_sample_t *samps;
  char *errmsg = NULL;
  Float maxval = 0.0;
  int i, j, k;
  file1_len = mus_sound_frames(file1);
  file2_len = mus_sound_frames(file2);
  if ((file1_len <= 0) || (file2_len <= 0)) return;
  file1_chans = mus_sound_chans(file1);
  if (file1_chans <= 0) mus_error(MUS_NO_CHANNELS, "%s chans: %d", file1, file1_chans);
  file2_chans = mus_sound_chans(file2);
  if (file2_chans <= 0) mus_error(MUS_NO_CHANNELS, "%s chans: %d", file2, file2_chans);
  output_chans = file1_chans; 
  if (file2_chans > output_chans) output_chans = file2_chans;
  fftlen = (int)(pow(2.0, (int)ceil(log(file1_len + file2_len + 1) / log(2.0))));
  outlen = file1_len + file2_len + 1;
  totallen = outlen * output_chans;
  data1 = (Float *)clm_calloc(fftlen, sizeof(Float), "convolve_files data");
  data2 = (Float *)clm_calloc(fftlen, sizeof(Float), "convolve_files data");
  if (output_chans == 1)
    {
      samps = (mus_sample_t *)clm_calloc(fftlen, sizeof(mus_sample_t), "convolve_files data");
      mus_file_to_array(file1, 0, 0, file1_len, samps); 
      for (i = 0; i < file1_len; i++) data1[i] = MUS_SAMPLE_TO_DOUBLE(samps[i]);
      mus_file_to_array(file2, 0, 0, file2_len, samps);
      for (i = 0; i < file2_len; i++) data2[i] = MUS_SAMPLE_TO_DOUBLE(samps[i]);
      mus_convolution(data1, data2, fftlen);
      for (i = 0; i < outlen; i++) 
	if (maxval < fabs(data1[i])) 
	  maxval = fabs(data1[i]);
      if (maxval > 0.0)
	{
	  maxval = maxamp / maxval;
	  for (i = 0; i < outlen; i++) data1[i] *= maxval;
	}
      for (i = 0; i < outlen; i++) samps[i] = MUS_DOUBLE_TO_SAMPLE(data1[i]);
      errmsg = mus_array_to_file_with_error(output_file, samps, outlen, mus_sound_srate(file1), 1);
      FREE(samps);
    }
  else
    {
      samps = (mus_sample_t *)clm_calloc(totallen, sizeof(mus_sample_t), "convolve_files data");
      outdat = (Float *)clm_calloc(totallen, sizeof(Float), "convolve_files data");
      c1 = 0; 
      c2 = 0;
      for (i = 0; i < output_chans; i++)
	{
	  mus_file_to_array(file1, c1, 0, file1_len, samps);
	  for (i = 0; i < file1_len; i++) data1[i] = MUS_SAMPLE_TO_DOUBLE(samps[i]);
	  mus_file_to_array(file2, c2, 0, file2_len, samps);
	  for (i = 0; i < file2_len; i++) data2[i] = MUS_SAMPLE_TO_DOUBLE(samps[i]);
	  mus_convolution(data1, data2, fftlen);
	  for (j = i, k = 0; j < totallen; j += output_chans, k++) outdat[j] = data1[k];
	  c1++; 
	  if (c1 >= file1_chans) c1 = 0;
	  c2++; 
	  if (c2 >= file2_chans) c2 = 0;
	  memset((void *)data1, 0, fftlen * sizeof(Float));
	  memset((void *)data2, 0, fftlen * sizeof(Float));
	}
      for (i = 0; i < totallen; i++) 
	if (maxval < fabs(outdat[i])) 
	  maxval = fabs(outdat[i]);
      if (maxval > 0.0)
	{
	  maxval = maxamp / maxval;
	  for (i = 0; i < totallen; i++) outdat[i] *= maxval;
	}
      for (i = 0; i < totallen; i++) samps[i] = MUS_DOUBLE_TO_SAMPLE(outdat[i]);
      errmsg = mus_array_to_file_with_error(output_file, samps, totallen, mus_sound_srate(file1), output_chans);
      FREE(samps);
      FREE(outdat);
    }
  FREE(data1);
  FREE(data2);
  if (errmsg)
    mus_error(MUS_CANT_OPEN_FILE, errmsg);
}



/* ---------------- mix files ---------------- */

/* a mixing "instrument" along the lines of the mix function in clm */
/* this is a very commonly used function, so it's worth picking out the special cases for optimization */

#define IDENTITY_MIX 0
#define IDENTITY_MONO_MIX 1
#define SCALED_MONO_MIX 2
#define SCALED_MIX 3
#define ENVELOPED_MONO_MIX 4
#define ENVELOPED_MIX 5
#define ALL_MIX 6

static int mix_type(int out_chans, int in_chans, mus_any *umx, mus_any ***envs)
{
  int i, j;
  mus_mixer *mx = (mus_mixer *)umx;
  if (envs)
    {
      if ((in_chans == 1) && (out_chans == 1)) 
	return(ENVELOPED_MONO_MIX);
      else 
	{
	  if (mx)
	    return(ALL_MIX);
	  return(ENVELOPED_MIX); 
	}
    }
  if (mx)
    {
      if ((in_chans == 1) && (out_chans == 1)) 
	{
	  if (mx->vals[0][0] == 1.0)
	    return(IDENTITY_MONO_MIX); 
	  return(SCALED_MONO_MIX);
	}
      for (i = 0; i < mx->chans; i++)
	for (j = 0; j < mx->chans; j++)
	  if (((i == j) && (mx->vals[i][i] != 1.0)) ||
	      ((i != j) && (mx->vals[i][j] != 0.0)))
	    return(SCALED_MIX);
    }
  if ((in_chans == 1) && (out_chans == 1)) 
    return(IDENTITY_MONO_MIX);
  return(IDENTITY_MIX);
}

void mus_mix_with_reader_and_writer(mus_any *outf, mus_any *inf, off_t out_start, off_t out_frames, off_t in_start, mus_any *umx, mus_any ***envs)
{
  int j = 0, k, in_chans, out_chans, mix_chans, min_chans, mixtype;
  mus_mixer *mx = (mus_mixer *)umx;
  off_t inc, outc, offi;
  mus_frame *frin, *frthru = NULL;
  out_chans = mus_channels(outf);
  if (out_chans <= 0) mus_error(MUS_NO_CHANNELS, "%s chans: %d", mus_describe(outf), out_chans);
  in_chans = mus_channels(inf);
  if (in_chans <= 0) mus_error(MUS_NO_CHANNELS, "%s chans: %d", mus_describe(inf), in_chans);
  if (out_chans > in_chans) mix_chans = out_chans; else mix_chans = in_chans;
  if (out_chans > in_chans) min_chans = in_chans; else min_chans = out_chans;
  mixtype = mix_type(out_chans, in_chans, umx, envs);
  frin = (mus_frame *)mus_make_empty_frame(mix_chans);
  frthru = (mus_frame *)mus_make_empty_frame(mix_chans);
  switch (mixtype)
    {
    case ENVELOPED_MONO_MIX:
    case ENVELOPED_MIX:
      if (umx == NULL) mx = (mus_mixer *)mus_make_identity_mixer(mix_chans); /* fall through */
    case ALL_MIX:
      /* the general case -- possible envs/scalers on every mixer cell */
      for (offi = 0, inc = in_start, outc = out_start; offi < out_frames; offi++, inc++, outc++)
	{
	  for (j = 0; j < in_chans; j++)
	    for (k = 0; k < out_chans; k++)
	      if (envs[j][k])
		mx->vals[j][k] = mus_env(envs[j][k]);
	  mus_frame2file(outf, 
			 outc, 
			 mus_frame2frame((mus_any *)mx, 
					 mus_file2frame(inf, inc, (mus_any *)frin), 
					 (mus_any *)frthru));
	}
      if (umx == NULL) mus_free((mus_any *)mx);
      break;
    case IDENTITY_MIX:
    case IDENTITY_MONO_MIX:
      for (offi = 0, inc = in_start, outc = out_start; offi < out_frames; offi++, inc++, outc++)
	mus_frame2file(outf, outc, mus_file2frame(inf, inc, (mus_any *)frin));
      break;
    case SCALED_MONO_MIX:
    case SCALED_MIX:
      for (offi = 0, inc = in_start, outc = out_start; offi < out_frames; offi++, inc++, outc++)
	mus_frame2file(outf, 
		       outc, 
		       mus_frame2frame((mus_any *)mx, 
				       mus_file2frame(inf, inc, (mus_any *)frin), 
				       (mus_any *)frthru));
      break;

    }
  mus_free((mus_any *)frin);
  mus_free((mus_any *)frthru);
}


void mus_mix(const char *outfile, const char *infile, off_t out_start, off_t out_frames, off_t in_start, mus_any *umx, mus_any ***envs)
{
  int i, j = 0, m, ofd, ifd, in_chans, out_chans, mix_chans, min_chans, mixtype;
  mus_sample_t **obufs, **ibufs;
  mus_mixer *mx = (mus_mixer *)umx;
  off_t offk, curoutframes;
  mus_any *inf, *outf;
  Float scaler;
  mus_any *e;
  out_chans = mus_sound_chans(outfile);
  if (out_chans <= 0) mus_error(MUS_NO_CHANNELS, "%s chans: %d", outfile, out_chans);
  in_chans = mus_sound_chans(infile);
  if (in_chans <= 0) mus_error(MUS_NO_CHANNELS, "%s chans: %d", infile, in_chans);
  if (out_chans > in_chans) mix_chans = out_chans; else mix_chans = in_chans;
  if (out_chans > in_chans) min_chans = in_chans; else min_chans = out_chans;
  mixtype = mix_type(out_chans, in_chans, umx, envs);
  if (mixtype == ALL_MIX)
    {
      /* the general case -- possible envs/scalers on every mixer cell */
      outf = mus_continue_sample2file(outfile);
      inf = mus_make_file2frame(infile);
      mus_mix_with_reader_and_writer(outf, inf, out_start, out_frames, in_start, umx, envs);
      mus_free((mus_any *)inf);
      mus_free((mus_any *)outf);
    }
  else
    {
      /* highly optimizable cases */
      obufs = (mus_sample_t **)clm_calloc(out_chans, sizeof(mus_sample_t *), "mix output");
      for (i = 0; i < out_chans; i++) 
	obufs[i] = (mus_sample_t *)clm_calloc(clm_file_buffer_size, sizeof(mus_sample_t), "mix output buffers");
      ibufs = (mus_sample_t **)clm_calloc(in_chans, sizeof(mus_sample_t *), "mix input");
      for (i = 0; i < in_chans; i++) 
	ibufs[i] = (mus_sample_t *)clm_calloc(clm_file_buffer_size, sizeof(mus_sample_t), "mix input buffers");
      ifd = mus_sound_open_input(infile);
      mus_sound_seek_frame(ifd, in_start);
      mus_sound_read(ifd, 0, clm_file_buffer_size - 1, in_chans, ibufs);
      ofd = mus_sound_reopen_output(outfile, 
				    out_chans, 
				    mus_sound_data_format(outfile), 
				    mus_sound_header_type(outfile), 
				    mus_sound_data_location(outfile));
      curoutframes = mus_sound_frames(outfile);
      mus_sound_seek_frame(ofd, out_start);
      mus_sound_read(ofd, 0, clm_file_buffer_size - 1, out_chans, obufs);
      mus_sound_seek_frame(ofd, out_start);
      switch (mixtype)
	{
	case IDENTITY_MONO_MIX:
	  for (offk = 0, j = 0; offk < out_frames; offk++, j++)
	    {
	      if (j == clm_file_buffer_size)
		{
		  mus_sound_write(ofd, 0, j - 1, out_chans, obufs);
		  j = 0;
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ofd, 0, clm_file_buffer_size - 1, out_chans, obufs);
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ifd, 0, clm_file_buffer_size - 1, in_chans, ibufs);
		}
	      obufs[0][j] += ibufs[0][j];
	    }
	  break;
	case IDENTITY_MIX:
	  for (offk = 0, j = 0; offk < out_frames; offk++, j++)
	    {
	      if (j == clm_file_buffer_size)
		{
		  mus_sound_write(ofd, 0, j - 1, out_chans, obufs);
		  j = 0;
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ofd, 0, clm_file_buffer_size - 1, out_chans, obufs);
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ifd, 0, clm_file_buffer_size - 1, in_chans, ibufs);
		}
	      for (i = 0; i < min_chans; i++)
		obufs[i][j] += ibufs[i][j];
	    }
	  break;
	case SCALED_MONO_MIX:
	  scaler = mx->vals[0][0];
	  for (offk = 0, j = 0; offk < out_frames; offk++, j++)
	    {
	      if (j == clm_file_buffer_size)
		{
		  mus_sound_write(ofd, 0, j - 1, out_chans, obufs);
		  j = 0;
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ofd, 0, clm_file_buffer_size - 1, out_chans, obufs);
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ifd, 0, clm_file_buffer_size - 1, in_chans, ibufs);
		}
	      obufs[0][j] += (mus_sample_t)(scaler * ibufs[0][j]);
	    }
	  break;
	case SCALED_MIX:
	  for (offk = 0, j = 0; offk < out_frames; offk++, j++)
	    {
	      if (j == clm_file_buffer_size)
		{
		  mus_sound_write(ofd, 0, j - 1, out_chans, obufs);
		  j = 0;
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ofd, 0, clm_file_buffer_size- 1 , out_chans, obufs);
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ifd, 0, clm_file_buffer_size - 1, in_chans, ibufs);
		}
	      for (i = 0; i < min_chans; i++)
		for (m = 0; m < in_chans; m++)
		  obufs[i][j] += (mus_sample_t)(ibufs[m][j] * mx->vals[m][i]);
	    }
	  break;
	case ENVELOPED_MONO_MIX:
	  e = envs[0][0];
	  for (offk = 0, j = 0; offk < out_frames; offk++, j++)
	    {
	      if (j == clm_file_buffer_size)
		{
		  mus_sound_write(ofd, 0, j - 1, out_chans, obufs);
		  j = 0;
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ofd, 0, clm_file_buffer_size - 1, out_chans, obufs);
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ifd, 0, clm_file_buffer_size - 1, in_chans, ibufs);
		}
	      obufs[0][j] += (mus_sample_t)(mus_env(e) * ibufs[0][j]);
	    }
	  break;
	case ENVELOPED_MIX:
	  e = envs[0][0];
	  for (offk = 0, j = 0; offk < out_frames; offk++, j++)
	    {
	      if (j == clm_file_buffer_size)
		{
		  mus_sound_write(ofd, 0, j - 1, out_chans, obufs);
		  j = 0;
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ofd, 0, clm_file_buffer_size - 1, out_chans, obufs);
		  mus_sound_seek_frame(ofd, out_start + offk);
		  mus_sound_read(ifd, 0, clm_file_buffer_size - 1, in_chans, ibufs);
		}
	      scaler = mus_env(e);
	      for (i = 0; i < min_chans; i++)
		obufs[i][j] += (mus_sample_t)(scaler * ibufs[i][j]);
	    }
	  break;

	}
      if (j > 0) 
	mus_sound_write(ofd, 0, j - 1, out_chans, obufs);
      if (curoutframes < (out_frames + out_start)) 
	curoutframes = out_frames + out_start;
      mus_sound_close_output(ofd, 
			     curoutframes * out_chans * mus_bytes_per_sample(mus_sound_data_format(outfile)));
      mus_sound_close_input(ifd);
      for (i = 0; i < in_chans; i++) FREE(ibufs[i]);
      FREE(ibufs);
      for (i = 0; i < out_chans; i++) FREE(obufs[i]);
      FREE(obufs);
    }
}

int mus_file2fltarray(const char *filename, int chan, off_t start, int samples, Float *array)
{
  mus_sample_t *idata;
  int i, len;
  idata = (mus_sample_t *)clm_calloc(samples, sizeof(mus_sample_t), "file2array buffer");
  len = mus_file_to_array(filename, chan, start, samples, idata);
  if (len != -1) 
    for (i = 0; i < samples; i++)
      array[i] = MUS_SAMPLE_TO_FLOAT(idata[i]);
  FREE(idata);
  return(len);
}

int mus_fltarray2file(const char *filename, Float *ddata, int len, int srate, int channels)
{
  mus_sample_t *idata;
  int i;
  char *errmsg;
  idata = (mus_sample_t *)clm_calloc(len, sizeof(mus_sample_t), "array2file buffer");
  for (i = 0; i < len; i++) 
    idata[i] = MUS_FLOAT_TO_SAMPLE(ddata[i]);
  errmsg = mus_array_to_file_with_error(filename, idata, len, srate, channels);
  FREE(idata);
  if (errmsg)
    return(mus_error(MUS_CANT_OPEN_FILE, errmsg));
  return(MUS_NO_ERROR);
}

Float mus_apply(mus_any *gen, ...)
{
  /* what about non-gen funcs such as polynomial, ring_modulate etc? */
  #define NEXT_ARG (Float)(va_arg(ap, double))
  va_list ap;
  Float f1 = 0.0, f2 = 0.0; /* force order of evaluation */
  if ((gen) && (MUS_RUN_P(gen)))
    {
      va_start(ap, gen);
      /* might want a run_args field */
      switch ((gen->core)->type)
	{
	case MUS_OSCIL:	case MUS_DELAY:	case MUS_COMB:	case MUS_NOTCH:	case MUS_ALL_PASS: case MUS_WAVESHAPE:
	  f1 = NEXT_ARG; 
	  f2 = NEXT_ARG; 
	  break;

	case MUS_SUM_OF_COSINES: case MUS_TABLE_LOOKUP: case MUS_SQUARE_WAVE: case MUS_SAWTOOTH_WAVE:
	case MUS_TRIANGLE_WAVE: case MUS_PULSE_TRAIN: case MUS_RAND: case MUS_RAND_INTERP:  
	case MUS_ASYMMETRIC_FM:	case MUS_ONE_ZERO: case MUS_ONE_POLE: case MUS_TWO_ZERO: case MUS_TWO_POLE:     
	case MUS_FORMANT: case MUS_SRC: case MUS_SINE_SUMMATION: case MUS_WAVE_TRAIN:    
	case MUS_FILTER: case MUS_FIR_FILTER: case MUS_IIR_FILTER: 
	  f1 = NEXT_ARG; 
	  break;

	case MUS_READIN: case MUS_CONVOLVE: case MUS_ENV: case MUS_GRANULATE:
	  break;
	}
      va_end(ap);
      return(MUS_RUN(gen, f1, f2));
    }
  return(0.0);
}

Float mus_bank(mus_any **gens, Float *scalers, Float *arg1, Float *arg2, int size)
{
  int i;
  Float sum = 0.0;
  if (arg1)
    {
      if (arg2)
	{
	  for (i = 0; i < size; i++) 
	    if (gens[i]) 
	      sum += (scalers[i] * (MUS_RUN(gens[i], arg1[i], arg2[i])));
	}
      else 
	{
	  for (i = 0; i < size; i++) 
	    if (gens[i]) 
	      sum += (scalers[i] * (MUS_RUN(gens[i], arg1[i], 0.0)));
	}
    }
  else 
    {
      for (i = 0; i < size; i++) 
	if (gens[i]) 
	  sum += (scalers[i] * (MUS_RUN(gens[i], 0.0, 0.0)));
    }
  return(sum);
}



/* ---------------- phase-vocoder ---------------- */

typedef struct {
  mus_any_class *core;
  Float pitch;
  Float (*input)(void *arg, int direction);
  void *environ;
  int (*analyze)(void *arg, Float (*input)(void *arg1, int direction));
  int (*edit)(void *arg);
  Float (*synthesize)(void *arg);
  int outctr, interp, filptr, N, D;
  Float *win, *ampinc, *amps, *freqs, *phases, *phaseinc, *lastphase, *in_data;
} pv_info;

static char *inspect_pv_info(mus_any *ptr)
{
  pv_info *gen = (pv_info *)ptr;
  char *arr1 = NULL, *arr2 = NULL, *arr3 = NULL;
  mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
	       "pv_info outctr: %d, interp: %d, filptr: %d, N: %d, D: %d, in_data: %s, amps: %s, freqs: %s",
	       gen->outctr, gen->interp, gen->filptr, gen->N, gen->D,
	       arr1 = print_array(gen->in_data, gen->N, 0),
	       arr2 = print_array(gen->amps, gen->N / 2, 0),
	       arr3 = print_array(gen->freqs, gen->N, 0));
  if (arr1) FREE(arr1);
  if (arr2) FREE(arr2);
  if (arr3) FREE(arr3);
  return(describe_buffer);
}

int mus_phase_vocoder_p(mus_any *ptr) {return((ptr) && ((ptr->core)->type == MUS_PHASE_VOCODER));}

static int phase_vocoder_equalp(mus_any *p1, mus_any *p2) {return(p1 == p2);}

static char *describe_phase_vocoder(mus_any *ptr)
{
  char *arr = NULL;
  pv_info *gen = (pv_info *)ptr;
  if (mus_phase_vocoder_p(ptr))
    {
      mus_snprintf(describe_buffer, DESCRIBE_BUFFER_SIZE,
		   "phase_vocoder: outctr: %d, interp: %d, filptr: %d, N: %d, D: %d, in_data: %s",
		   gen->outctr, gen->interp, gen->filptr, gen->N, gen->D,
		   arr = print_array(gen->in_data, gen->N, 0));
      if (arr) FREE(arr);
    }
  else describe_bad_gen(ptr, "phase_vocoder", "a");
  return(describe_buffer);
}

static int free_phase_vocoder(mus_any *ptr)
{
  pv_info *gen = (pv_info *)ptr;
  if (gen)
    {
      if (gen->in_data) FREE(gen->in_data);
      if (gen->amps) FREE(gen->amps);
      if (gen->freqs) FREE(gen->freqs);
      if (gen->phases) FREE(gen->phases);
      if (gen->win) FREE(gen->win);
      if (gen->phaseinc) FREE(gen->phaseinc);
      if (gen->lastphase) FREE(gen->lastphase);
      if (gen->ampinc) FREE(gen->ampinc);
      FREE(gen);
    }
  return(0);
}

static int pv_length(mus_any *ptr) {return(((pv_info *)ptr)->N);}
static int pv_set_length(mus_any *ptr, int val) {((pv_info *)ptr)->N = val; return(val);}
static int pv_hop(mus_any *ptr) {return(((pv_info *)ptr)->D);}
static int pv_set_hop(mus_any *ptr, int val) {((pv_info *)ptr)->D = val; return(val);}
static Float pv_frequency(mus_any *ptr) {return(((pv_info *)ptr)->pitch);}
static Float pv_set_frequency(mus_any *ptr, Float val) {((pv_info *)ptr)->pitch = val; return(val);}
static void *pv_environ(mus_any *rd) {return(((pv_info *)rd)->environ);}

Float *mus_phase_vocoder_amp_increments(mus_any *ptr) {return(((pv_info *)ptr)->ampinc);}
Float *mus_phase_vocoder_amps(mus_any *ptr) {return(((pv_info *)ptr)->amps);}
Float *mus_phase_vocoder_freqs(mus_any *ptr) {return(((pv_info *)ptr)->freqs);}
Float *mus_phase_vocoder_phases(mus_any *ptr) {return(((pv_info *)ptr)->phases);}
Float *mus_phase_vocoder_phase_increments(mus_any *ptr) {return(((pv_info *)ptr)->phaseinc);}
Float *mus_phase_vocoder_previous_phases(mus_any *ptr) {return(((pv_info *)ptr)->lastphase);}
int mus_phase_vocoder_outctr(mus_any *ptr) {return(((pv_info *)ptr)->outctr);}
int mus_phase_vocoder_set_outctr(mus_any *ptr, int val) {((pv_info *)ptr)->outctr = val; return(val);}
static Float *phase_vocoder_data(mus_any *ptr) {return(((pv_info *)ptr)->in_data);}
static Float run_phase_vocoder(mus_any *ptr, Float unused1, Float unused2) {return(mus_phase_vocoder(ptr, NULL));}
static Float pv_increment(mus_any *rd) {return((Float)(((pv_info *)rd)->interp));}
static Float pv_set_increment(mus_any *rd, Float val) {((pv_info *)rd)->interp = (int)val; return(val);}

static mus_any_class PHASE_VOCODER_CLASS = {
  MUS_PHASE_VOCODER,
  "phase_vocoder",
  &free_phase_vocoder,
  &describe_phase_vocoder,
  &inspect_pv_info,
  &phase_vocoder_equalp,
  &phase_vocoder_data, 0,
  &pv_length,
  &pv_set_length,
  &pv_frequency,
  &pv_set_frequency,
  0, 0,
  0, 0,
  &pv_increment,
  &pv_set_increment,
  &run_phase_vocoder,
  MUS_NOT_SPECIAL,
  &pv_environ,
  0,
  0, 0, 0, 0, 0, 0, 
  &pv_hop, &pv_set_hop, 
  0, 0,
  0, 0, 0, 0, 0, 0, 0
};

mus_any *mus_make_phase_vocoder(Float (*input)(void *arg, int direction), 
				int fftsize, int overlap, int interp,
				Float pitch,
				int (*analyze)(void *arg, Float (*input)(void *arg1, int direction)),
				int (*edit)(void *arg), 
				Float (*synthesize)(void *arg), 
				void *environ)
{
  /* order of args is trying to match src, granulate etc
   *   the inclusion of pitch and interp provides built-in time/pitch scaling which is 99% of phase-vocoder use
   */
  pv_info *pv;
  int N2, D, i;
  Float scl;
  N2 = (int)(fftsize / 2);
  if (N2 == 0) return(NULL);
  D = fftsize / overlap;
  if (D == 0) return(NULL);
  pv = (pv_info *)clm_calloc(1, sizeof(pv_info), "phase_vocoder");
  pv->core = &PHASE_VOCODER_CLASS;
  pv->N = fftsize;
  pv->D = D;
  pv->interp = interp;
  pv->outctr = interp;
  pv->filptr = 0;
  pv->pitch = pitch;
  pv->ampinc = (Float *)clm_calloc(fftsize, sizeof(Float), "pvoc ampinc");
  pv->freqs = (Float *)clm_calloc(fftsize, sizeof(Float), "pvoc freqs");
  pv->amps = (Float *)clm_calloc(N2, sizeof(Float), "pvoc amps");
  pv->phases = (Float *)clm_calloc(N2, sizeof(Float), "pvoc phases");
  pv->lastphase = (Float *)clm_calloc(N2, sizeof(Float), "pvoc lastphase");
  pv->phaseinc = (Float *)clm_calloc(N2, sizeof(Float), "pvoc phaseinc");
  pv->in_data = NULL;
  pv->input = input;
  pv->environ = environ;
  pv->analyze = analyze;
  pv->edit = edit;
  pv->synthesize = synthesize;
  pv->win = mus_make_fft_window(MUS_HAMMING_WINDOW, fftsize, 0.0);
  scl = 2.0 / (0.54 * (Float)fftsize);
  if (pv->win) /* clm2xen traps errors for later reporting (to clean up local allocation),
		*   so clm_calloc might return NULL in all the cases here
		*/
    for (i = 0; i < fftsize; i++) 
      pv->win[i] *= scl;
  return((mus_any *)pv);
}

Float mus_phase_vocoder(mus_any *ptr, Float (*input)(void *arg, int direction))
{
  pv_info *pv = (pv_info *)ptr;
  int N2, i, j, buf;
  Float pscl, kscl, ks, diff, scl;
  N2 = pv->N / 2;
  if (pv->outctr >= pv->interp)
    {
      if ((pv->analyze == NULL) || 
	  ((*(pv->analyze))(pv->environ, input) != 0))
	{
	  mus_clear_array(pv->freqs, pv->N);
	  pv->outctr = 0;
	  if (pv->in_data == NULL)
	    {
	      pv->in_data = (Float *)clm_calloc(pv->N, sizeof(Float), "pvoc indata");
	      if (input)
		for (i = 0; i < pv->N; i++) pv->in_data[i] = (*input)(pv->environ, 1);
	      else for (i = 0; i < pv->N; i++) pv->in_data[i] = (*(pv->input))(pv->environ, 1);
	    }
	  else
	    {
	      for (i = 0, j = pv->D; j < pv->N; i++, j++) pv->in_data[i] = pv->in_data[j];
	      if (input)
		for (i = pv->N - pv->D; i < pv->N; i++) pv->in_data[i] = (*input)(pv->environ, 1);
	      else for (i = pv->N - pv->D; i < pv->N; i++) pv->in_data[i] = (*(pv->input))(pv->environ, 1);
	    }
	  buf = pv->filptr % pv->N;
	  for (i = 0; i < pv->N; i++)
	    {
	      pv->ampinc[buf++] = pv->win[i] * pv->in_data[i];
	      if (buf >= pv->N) buf = 0;
	    }
	  pv->filptr += pv->D;
#if HAVE_FFTW || HAVE_FFTW3
	  /* actually this is not faster than the mus_fft case -- other processing swamps the fft here */
	  {
	    double temp;
	    mus_fftw(pv->ampinc, pv->N, 1);
	    pv->ampinc[0] = fabs(pv->ampinc[0]);
	    pv->ampinc[N2] = fabs(pv->ampinc[N2]);
	    for (i = 1, j = pv->N - 1; i < N2; i++, j--)
	      {
		pv->freqs[i] = atan2(pv->ampinc[j], pv->ampinc[i]);
		temp = pv->ampinc[i] * pv->ampinc[i] + pv->ampinc[j] * pv->ampinc[j];
		if (temp < .00000001) 
		  pv->ampinc[i] = 0.0;
		else pv->ampinc[i] = sqrt(temp);
	      }
	  }
#else
	  mus_fft(pv->ampinc, pv->freqs, pv->N, 1);
	  mus_rectangular2polar(pv->ampinc, pv->freqs, N2);
#endif
	}
      
      if ((pv->edit == NULL) || 
	  ((*(pv->edit))(pv->environ) != 0))
	{
	  pscl = 1.0 / (Float)(pv->D);
	  kscl = TWO_PI / (Float)(pv->N);
	  for (i = 0, ks = 0.0; i < N2; i++, ks += kscl)
	    {
	      diff = pv->freqs[i] - pv->lastphase[i];
	      pv->lastphase[i] = pv->freqs[i];
	      while (diff > M_PI) diff -= TWO_PI;
	      while (diff < -M_PI) diff += TWO_PI;
	      pv->freqs[i] = pv->pitch * (diff * pscl + ks);
	    }
	}
      
      scl = 1.0 / (Float)(pv->interp);
      for (i = 0; i < N2; i++)
	{
	  pv->ampinc[i] = scl * (pv->ampinc[i] - pv->amps[i]);
	  pv->freqs[i] = scl * (pv->freqs[i] - pv->phaseinc[i]);
	}
    }
  
  pv->outctr++;
  if (pv->synthesize)
    return((*(pv->synthesize))(pv->environ));
  else
    {
      for (i = 0; i < N2; i++)
	{
	  pv->amps[i] += pv->ampinc[i];
	  pv->phaseinc[i] += pv->freqs[i];
	  pv->phases[i] += pv->phaseinc[i];
	}
      return(mus_sum_of_sines(pv->amps, pv->phases, N2));
    }
}

void init_mus_module(void)
{
#if WITH_SINE_TABLE
  init_sine();
#endif
}

