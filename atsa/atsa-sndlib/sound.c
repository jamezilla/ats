/* sound.c */

#if defined(HAVE_CONFIG_H)
  #include <config.h>
#endif

#if USE_SND
  #include "snd.h"
#else
  #define PRINT_BUFFER_SIZE 512
  #define LABEL_BUFFER_SIZE 64
#endif

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>

#if (defined(HAVE_LIBC_H) && (!defined(HAVE_UNISTD_H)))
  #include <libc.h>
#else
  #if (!(defined(_MSC_VER))) && (!(defined(MPW_C)))
    #include <unistd.h>
  #endif
  #if (!defined(HAVE_CONFIG_H)) || HAVE_STRING_H
    #include <string.h>
  #endif
#endif

#include "sndlib.h"
#include "sndlib-strings.h"

#if MACOS
  #ifdef MPW_C
    #define time_t long
  #else
    #include <time.h>
    #include <stat.h>
  #endif
  #define off_t long
#else
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <time.h>
#endif

#include <stdarg.h>

static mus_error_handler_t *mus_error_handler = NULL;

mus_error_handler_t *mus_error_set_handler(mus_error_handler_t *new_error_handler) 
{
  mus_error_handler_t *old_handler;
  old_handler = mus_error_handler;
  mus_error_handler = new_error_handler;
  return(old_handler);
}

#define MUS_ERROR_BUFFER_SIZE 1024
static char *mus_error_buffer = NULL;

int mus_error(int error, const char *format, ...)
{
#if HAVE_VPRINTF
  va_list ap;
#endif
  if (format == NULL) return(MUS_ERROR); /* else bus error in Mac OSX */
#if HAVE_VPRINTF
  if (mus_error_buffer == NULL)
    mus_error_buffer = (char *)CALLOC(MUS_ERROR_BUFFER_SIZE, sizeof(char));
  va_start(ap, format);
#if HAVE_VSNPRINTF
  vsnprintf(mus_error_buffer, MUS_ERROR_BUFFER_SIZE, format, ap);
#else
  vsprintf(mus_error_buffer, format, ap);
#endif
  va_end(ap);
  if (mus_error_handler)
    (*mus_error_handler)(error, mus_error_buffer);
  else 
    {
      fprintf(stderr, mus_error_buffer);
      fputc('\n', stderr);
    }
#else
  if (error == 0) /* this case mainly for CLM */
    fprintf(stderr, format);
  else fprintf(stderr, "error: %d %s\n", error, format);
#endif
  return(MUS_ERROR);
}

static mus_print_handler_t *mus_print_handler = NULL;

mus_print_handler_t *mus_print_set_handler(mus_print_handler_t *new_print_handler) 
{
  mus_print_handler_t *old_handler;
  old_handler = mus_print_handler;
  mus_print_handler = new_print_handler;
  return(old_handler);
}

void mus_print(const char *format, ...)
{
#if HAVE_VPRINTF
  va_list ap;
  if (mus_error_buffer == NULL)
    mus_error_buffer = (char *)CALLOC(MUS_ERROR_BUFFER_SIZE, sizeof(char));
  if (mus_print_handler)
    {
      va_start(ap, format);
#if HAVE_VSNPRINTF
      vsnprintf(mus_error_buffer, MUS_ERROR_BUFFER_SIZE, format, ap);
#else
      vsprintf(mus_error_buffer, format, ap);
#endif
      va_end(ap);
      (*mus_print_handler)(mus_error_buffer);
    }
  else
    {
      va_start(ap, format);
      vfprintf(stdout, format, ap);
      va_end(ap);
    }
#endif
}

static const char *mus_initial_error_names[] = {
  "no error", "no frequency method", "no phase method", "null gen arg to method", "no length method",
  "no free method", "no describe method", "no data method", "no scaler method",
  "memory allocation failed", "unstable two pole error",
  "can't open file", "no sample input", "no sample output",
  "no such channel", "no file name provided", "no location method", "no channel method",
  "no such fft window", "unsupported data format", "header read failed",
  "unsupported header type", "file descriptors not initialized", "not a sound file", "file closed", "write error",
  "bogus free", "buffer overflow", "buffer underflow", "file overflow",
  "header write failed", "can't open temp file", "interrupted", "bad envelope",

  "audio channels not available", "audio srate not available", "audio format not available",
  "no audio input available", "audio configuration not available", 
  "no audio lines available", "audio write error", "audio size not available", "audio device not available",
  "can't close audio", "can't open audio", "audio read error", "audio amp not available",
  "can't write audio", "can't read audio", "no audio read permission", 
  "can't close file", "arg out of range",

  "midi open error", "midi read error", "midi write error", "midi close error", "midi init error", "midi misc error",

  "no channels method", "no hop method", "no width method", "no file-name method", "no ramp method", "no run method",
  "no increment method", "no b2 method", "no inspect method", "no offset method",
  "no x1 method", "no x2 method", "no y1 method", "no y2 method"
};

static char **mus_error_names = NULL;
static int mus_error_names_size = 0;

static int mus_error_tag = MUS_INITIAL_ERROR_TAG;
int mus_make_error(char *error_name) 
{
  int new_error, err, len, i;
  new_error = mus_error_tag++;
  err = new_error - MUS_INITIAL_ERROR_TAG;
  if (error_name)
    {
      if (err >= mus_error_names_size)
	{
	  if (mus_error_names_size == 0)
	    {
	      mus_error_names_size = 8;
	      mus_error_names = (char **)CALLOC(mus_error_names_size, sizeof(char *));
	    }
	  else
	    {
	      len = mus_error_names_size;
	      mus_error_names_size += 8;
	      mus_error_names = (char **)REALLOC(mus_error_names, mus_error_names_size * sizeof(char *));
	      for (i = len; i < mus_error_names_size; i++) mus_error_names[i] = NULL;
	    }
	}
      len = strlen(error_name);
      mus_error_names[err] = (char *)CALLOC(len + 1, sizeof(char));
      strcpy(mus_error_names[err], error_name);
    }
  return(new_error);
}

const char *mus_error_to_string(int err)
{
  if (err < MUS_INITIAL_ERROR_TAG)
    return(mus_initial_error_names[err]);
  else
    {
      err -= MUS_INITIAL_ERROR_TAG;
      if ((mus_error_names) && (err < mus_error_names_size))
	return(mus_error_names[err]);
      else return("unknown mus error");
    }
}

static void default_mus_error(int type, char *msg)
{
  /* default error handler simply prints the error message */
  fprintf(stderr, msg);
}



#ifndef MPW_C
static time_t local_file_write_date(const char *filename)
{
  struct stat statbuf;
  int err;
  err = stat(filename, &statbuf);
  if (err < 0) return(err);
  return((time_t)(statbuf.st_mtime));
}
#else
static int local_file_write_date(const char *filename) {return(1);}
#endif

static int sndlib_initialized = FALSE;

int mus_sound_initialize(void)
{
  int err = MUS_NO_ERROR;
  if (!sndlib_initialized)
    {
      sndlib_initialized = TRUE;
      mus_error_handler = default_mus_error;
      err = mus_header_initialize();
         /* NOTE: */
	 /* mus_audio_initialize reference commented out for atsa JP */
/*       if (err == MUS_NO_ERROR)  */
/* 	err = mus_audio_initialize(); */
      return(err);
    }
  return(MUS_NO_ERROR);
}

int mus_sample_bits(void)
{
  /* this to check for inconsistent loads */
#if SNDLIB_USE_FLOATS
  return(0);
#else
  return(MUS_SAMPLE_BITS);
#endif
}

typedef struct {
  char *file_name;  /* full path -- everything is keyed to this name */
  int table_pos;
  int *aux_comment_start, *aux_comment_end;
  int *loop_modes, *loop_starts, *loop_ends;
  int markers, base_detune, base_note;
  int *marker_ids, *marker_positions;
  off_t samples, true_file_length;
  off_t data_location;
  int srate, chans, header_type, data_format, original_sound_format, datum_size; 
  int comment_start, comment_end, type_specifier, bits_per_sample, fact_samples, block_align;
  time_t write_date;
  mus_sample_t *maxamps;
  off_t *maxtimes;
} sound_file;

static int sound_table_size = 0;
static sound_file **sound_table = NULL;
static sound_file *previous_sf = NULL; /* memoized search */
static int previous_freed_sf = -1;

static void free_sound_file(sound_file *sf)
{
  previous_sf = NULL;
  if (sf)
    {
      sound_table[sf->table_pos] = NULL;
      previous_freed_sf = sf->table_pos;
      if (sf->aux_comment_start) FREE(sf->aux_comment_start);
      if (sf->aux_comment_end) FREE(sf->aux_comment_end);
      if (sf->file_name) FREE(sf->file_name);
      if (sf->loop_modes) FREE(sf->loop_modes);
      if (sf->loop_starts) FREE(sf->loop_starts);
      if (sf->loop_ends) FREE(sf->loop_ends);
      if (sf->marker_ids) FREE(sf->marker_ids);
      if (sf->marker_positions) FREE(sf->marker_positions);
      if (sf->maxamps) FREE(sf->maxamps);
      if (sf->maxtimes) FREE(sf->maxtimes);
      FREE(sf);
    }
}

static sound_file *add_to_sound_table(const char *name)
{
  int i, pos;
  pos = previous_freed_sf;
  if (pos == -1)
    {
      for (i = 0; i < sound_table_size; i++)
	if (sound_table[i] == NULL) 
	  {
	    pos = i;
	    break;
	  }
      if (pos == -1)
	{
	  pos = sound_table_size;
	  sound_table_size += 16;
	  if (sound_table == NULL)
	    sound_table = (sound_file **)CALLOC(sound_table_size, sizeof(sound_file *));
	  else 
	    {
	      sound_table = (sound_file **)REALLOC(sound_table, sound_table_size * sizeof(sound_file *));
	      for (i = pos; i < sound_table_size; i++) sound_table[i] = NULL;
	    }
	}
    }
  else previous_freed_sf = -1;
  sound_table[pos] = (sound_file *)CALLOC(1, sizeof(sound_file));
  sound_table[pos]->table_pos = pos;
  sound_table[pos]->file_name = (char *)CALLOC(strlen(name) + 1, sizeof(char));
  strcpy(sound_table[pos]->file_name, name);
  return(sound_table[pos]);
}

int mus_sound_prune(void)
{
  int i, pruned = 0;
  for (i = 0; i < sound_table_size; i++)
    if ((sound_table[i]) && 
	(!(mus_file_probe(sound_table[i]->file_name))))
      {
	free_sound_file(sound_table[i]);
	sound_table[i] = NULL;
	pruned++;
      }
  return(pruned);
}

int mus_sound_forget(const char *name)
{
  int i, len, free_name = FALSE;
  char *short_name = NULL;
  if (name == NULL) return(MUS_ERROR);
  if (name[0] == '/')
    {
      len = strlen(name);
      for (i = 0; i < len; i++)
	if (name[i] == '/')
	  short_name = (char *)(name + i + 1);
    }
  else
    {
      short_name = mus_expand_filename((char *)name);
      free_name = TRUE;
    }
  previous_sf = NULL;
  if (name)
    for (i = 0; i < sound_table_size; i++)
      if ((sound_table[i]) &&
	  ((strcmp(name, sound_table[i]->file_name) == 0) ||
	   ((short_name) && 
	    (strcmp(short_name, sound_table[i]->file_name) == 0))))
	{
	  free_sound_file(sound_table[i]);
	  sound_table[i] = NULL;
	}
#if DEBUG_MEMORY || (defined(MPW_C)) || (!HAVE_STRDUP)
  if (free_name) FREE(short_name); /* copy_string=MALLOC(snd-utils.c) in io.c mus_expand_filename or strdup=MALLOC in MPW */
#else
  if (free_name) free(short_name); /* strdup=malloc, presumably */
#endif
  return(MUS_NO_ERROR);
}

static sound_file *check_write_date(const char *name, sound_file *sf)
{
  int chan;
  off_t data_size;
  time_t date;
  if (sf)
    {
      date = local_file_write_date(name);
      if (date == sf->write_date)
	return(sf);
      else 
	{
	  if ((sf->header_type == MUS_RAW) && (mus_header_no_header(name)))
	    {
	      /* sound has changed since we last read it, but it has no header, so
	       * the only sensible thing to check is the new length (i.e. caller
	       * has set other fields by hand)
	       */
	      sf->write_date = date;
	      chan = mus_file_open_read(name);
	      data_size = lseek(chan, 0L, SEEK_END);
	      sf->true_file_length = data_size;
	      sf->samples = mus_bytes_to_samples(sf->data_format, data_size);
	      CLOSE(chan);  
	      return(sf);
	    }
	  /* otherwise our data base is out-of-date, so clear it out */
	  free_sound_file(sf);
	}
    }
  return(NULL);
}

static sound_file *find_sound_file(const char *name)
{
  int i;
  /* perhaps we already have the needed data... */
  if ((previous_sf) &&
      (strcmp(previous_sf->file_name, name) == 0) &&
      (previous_sf->write_date == local_file_write_date(name)))
    return(previous_sf);
  if (name)
    for (i = 0; i < sound_table_size; i++)
      if ((sound_table[i]) &&
	  (strcmp(name, sound_table[i]->file_name) == 0))
	{
	  previous_sf = check_write_date(name, sound_table[i]);
	  return(previous_sf);
	}
  return(NULL);
}

static void display_sound_file_entry(FILE *fp, const char *name, sound_file *sf)
{
  int i, lim;
  time_t date;
  char timestr[64];
  char *comment;
#ifndef MPW_C
  date = sf->write_date;
  if (date != 0)
    {
#if (!defined(HAVE_CONFIG_H)) || HAVE_STRFTIME
      strftime(timestr, 64, "%a %d-%b-%Y %H:%M:%S", localtime(&date));
#else
      sprintf(timestr, "%d", date);
#endif
    }
  else sprintf(timestr, "(date cleared)");
#else
  sprintf(timestr, "(date unknown)");
#endif
  fprintf(fp, "  %s: %s, chans: %d, srate: %d, type: %s, format: %s, samps: " OFF_TD,
	  name,
	  timestr,
	  sf->chans,
	  sf->srate,
	  mus_header_type_name(sf->header_type),
	  mus_data_format_name(sf->data_format),
	  sf->samples);
  if (sf->loop_modes)
    {
      if (sf->loop_modes[0] != 0)
	fprintf(fp, ", loop mode %d: %d to %d", sf->loop_modes[0], sf->loop_starts[0], sf->loop_ends[0]);
      if (sf->loop_modes[1] != 0)
	fprintf(fp, ", loop mode %d: %d to %d, ", sf->loop_modes[1], sf->loop_starts[1], sf->loop_ends[1]);
      fprintf(fp, ", base: %d, detune: %d", sf->base_note, sf->base_detune);
    }
  if (sf->maxamps)
    {
      lim = sf->chans;
      if (lim > 0)
	{
	  if (lim > 64) 
	    lim = 64;
	  for (i = 0; i < lim; i++)
	    {
	      if (i > 1) fprintf(fp, ", ");
	      fprintf(fp, " %.3f at %.3f ",
		      MUS_SAMPLE_TO_FLOAT(sf->maxamps[i]),
		      (sf->srate > 0.0) ? (float)((double)(sf->maxtimes[i]) / (double)(sf->srate)) : (float)(sf->maxtimes[i]));
	    }
	}
    }
  if (mus_file_probe(name))
    {
      comment = mus_sound_comment(name);
      if (comment)
	{
	  fprintf(fp, "\n      comment: %s", comment);
	  FREE(comment);
	}
    }
  else fprintf(fp, " [defunct]");
  fprintf(fp, "\n");
}

void mus_sound_report_cache(FILE *fp)
{
  sound_file *sf;
  int entries = 0;
  int i;
  fprintf(fp, "sound table:\n");
  for (i = 0; i < sound_table_size; i++)
    {
      sf = sound_table[i];
      if (sf) 
	{
	  display_sound_file_entry(fp, sf->file_name, sf);
	  entries++;
	}
    }
  fprintf(fp, "\nentries: %d\n", entries); 
  fflush(fp);
}

static sound_file *fill_sf_record(const char *name, sound_file *sf)
{
  int i;
  sf->data_location = mus_header_data_location();
  sf->samples = mus_header_samples();
  sf->data_format = mus_header_format();
  sf->srate = mus_header_srate();
  /* if (sf->srate < 0) sf->srate = 0; */
  sf->chans = mus_header_chans();
  /* if (sf->chans < 0) sf->chans = 0; */
  sf->datum_size = mus_bytes_per_sample(sf->data_format);
  sf->header_type = mus_header_type();
  sf->original_sound_format = mus_header_original_format();
  sf->true_file_length = mus_header_true_length();
  sf->comment_start = mus_header_comment_start();
  sf->comment_end = mus_header_comment_end();
  if (((sf->header_type == MUS_AIFC) || 
       (sf->header_type == MUS_AIFF) || 
       (sf->header_type == MUS_RIFF)) &&
      (mus_header_aux_comment_start(0) != 0))

    {
      sf->aux_comment_start = (int *)CALLOC(4, sizeof(int));
      sf->aux_comment_end = (int *)CALLOC(4, sizeof(int));
      for (i = 0; i < 4; i++)
	{
	  sf->aux_comment_start[i] = mus_header_aux_comment_start(i);
	  sf->aux_comment_end[i] = mus_header_aux_comment_end(i);
	}
    }
  sf->type_specifier = mus_header_type_specifier();
  sf->bits_per_sample = mus_header_bits_per_sample();
  sf->fact_samples = mus_header_fact_samples();
  sf->block_align = mus_header_block_align();
  sf->write_date = local_file_write_date(name);
  if (mus_header_loop_mode(0) > 0)
    {
      sf->loop_modes = (int *)CALLOC(2, sizeof(int));
      sf->loop_starts = (int *)CALLOC(2, sizeof(int));
      sf->loop_ends = (int *)CALLOC(2, sizeof(int));
      for (i = 0; i < 2; i++)
	{
	  sf->loop_modes[i] = mus_header_loop_mode(i);
	  if ((sf->header_type == MUS_AIFF) || 
	      (sf->header_type == MUS_AIFC))
	    {
	      sf->loop_starts[i] = mus_header_mark_position(mus_header_loop_start(i)); 
	      sf->loop_ends[i] = mus_header_mark_position(mus_header_loop_end(i));
	    }
	  else
	    {
	      sf->loop_starts[i] = mus_header_loop_start(i); 
	      sf->loop_ends[i] = mus_header_loop_end(i);
	    }
	}
      sf->base_detune = mus_header_base_detune();
      sf->base_note = mus_header_base_note();
    }
  /* aux comments */
  previous_sf = sf;
  return(sf);
}

static sound_file *read_sound_file_header_with_fd(int fd, const char *arg)
{
  mus_sound_initialize();
  if ((mus_header_read_with_fd(fd)) == MUS_ERROR) return(NULL);
  return(fill_sf_record(arg, add_to_sound_table(arg)));
}

static sound_file *read_sound_file_header_with_name(const char *name)
{
  mus_sound_initialize();
  if (mus_header_read(name) != MUS_ERROR)
    return(fill_sf_record(name, add_to_sound_table(name)));
  return(NULL);
}

static sound_file *getsf(const char *arg) 
{
  sound_file *sf = NULL;
  if (arg == NULL) return(NULL);
  if ((sf = find_sound_file(arg)) == NULL)
    return(read_sound_file_header_with_name(arg));
  return(sf);
}

#define MUS_SF(Filename, Expression) \
  sound_file *sf; \
  sf = getsf(Filename); \
  if (sf) return(Expression); \
  return(MUS_ERROR)

off_t mus_sound_samples (const char *arg)       {MUS_SF(arg, sf->samples);}
off_t mus_sound_frames (const char *arg)        {MUS_SF(arg, (sf->chans > 0) ? (sf->samples / sf->chans) : 0);}
int mus_sound_datum_size (const char *arg)      {MUS_SF(arg, sf->datum_size);}
off_t mus_sound_data_location (const char *arg) {MUS_SF(arg, sf->data_location);}
int mus_sound_chans (const char *arg)           {MUS_SF(arg, sf->chans);}
int mus_sound_srate (const char *arg)           {MUS_SF(arg, sf->srate);}
int mus_sound_header_type (const char *arg)     {MUS_SF(arg, sf->header_type);}
int mus_sound_data_format (const char *arg)     {MUS_SF(arg, sf->data_format);}
int mus_sound_original_format (const char *arg) {MUS_SF(arg, sf->original_sound_format);}
int mus_sound_comment_start (const char *arg)   {MUS_SF(arg, sf->comment_start);}
int mus_sound_comment_end (const char *arg)     {MUS_SF(arg, sf->comment_end);}
off_t mus_sound_length (const char *arg)        {MUS_SF(arg, sf->true_file_length);}
int mus_sound_fact_samples (const char *arg)    {MUS_SF(arg, sf->fact_samples);}
int mus_sound_write_date (const char *arg)      {MUS_SF(arg, (int)(sf->write_date));}
int mus_sound_type_specifier (const char *arg)  {MUS_SF(arg, sf->type_specifier);}
int mus_sound_align (const char *arg)           {MUS_SF(arg, sf->block_align);}
int mus_sound_bits_per_sample (const char *arg) {MUS_SF(arg, sf->bits_per_sample);}

float mus_sound_duration(const char *arg) 
{
  float val = -1.0;
  sound_file *sf; 
  sf = getsf(arg); 
  if (sf) 
    {
      if ((sf->chans > 0) && (sf->srate > 0))
	val = (float)((double)(sf->samples) / ((float)(sf->chans) * (float)(sf->srate)));
      else val = 0.0;
    }
  return(val);
}

int *mus_sound_loop_info(const char *arg)
{
  sound_file *sf; 
  int *info;
  sf = getsf(arg); 
  if ((sf) && (sf->loop_modes))
    {
      info = (int *)CALLOC(MUS_LOOP_INFO_SIZE, sizeof(int));
      if (sf->loop_modes[0] != 0)
	{
	  info[0] = sf->loop_starts[0];
	  info[1] = sf->loop_ends[0];
	  info[6] = sf->loop_modes[0];
	}
      if (sf->loop_modes[1] != 0)
	{
	  info[2] = sf->loop_starts[1];
	  info[3] = sf->loop_ends[1];
	  info[7] = sf->loop_modes[1];
	}
      info[4] = sf->base_note;
      info[5] = sf->base_detune;
      return(info);
    }
  else return(NULL);
}

void mus_sound_set_full_loop_info(const char *arg, int *loop)
{
  sound_file *sf; 
  sf = getsf(arg); 
  if (sf)
    {
      if (sf->loop_modes == NULL)
	{
	  sf->loop_modes = (int *)CALLOC(2, sizeof(int));
	  sf->loop_starts = (int *)CALLOC(2, sizeof(int));
	  sf->loop_ends = (int *)CALLOC(2, sizeof(int));
	}
      sf->loop_modes[0] = loop[6]; 
      if (loop[6] != 0)
	{
	  sf->loop_starts[0] = loop[0];
	  sf->loop_ends[0] = loop[1];
	}
      else
	{
	  sf->loop_starts[0] = 0;
	  sf->loop_ends[0] = 0;
	}
      sf->loop_modes[1] = loop[7];
      if (loop[7] != 0)
	{
	  sf->loop_starts[1] = loop[2];
	  sf->loop_ends[1] = loop[3];
	}
      else
	{
	  sf->loop_starts[1] = 0;
	  sf->loop_ends[1] = 0;
	}
      sf->base_note = loop[4];
      sf->base_detune = loop[5];
    }
}

char *mus_sound_comment(const char *name)
{
  int start, end, fd, len, full_len;
  char *sc = NULL, *auxcom;
  sound_file *sf = NULL;
  sf = getsf(name); 
  if (sf == NULL) return(NULL);
  start = mus_sound_comment_start(name);
  end = mus_sound_comment_end(name);
  if (end == 0) 
    {
      if (sf->aux_comment_start)
	{
	  if (mus_sound_header_type(name) == MUS_RIFF) 
	    return(mus_header_riff_aux_comment(name, 
					       sf->aux_comment_start, 
					       sf->aux_comment_end));
	  if ((mus_sound_header_type(name) == MUS_AIFF) || 
	      (mus_sound_header_type(name) == MUS_AIFC)) 
	    return(mus_header_aiff_aux_comment(name, 
					       sf->aux_comment_start, 
					       sf->aux_comment_end));
	}
      return(NULL);
    }
  len = end - start + 1;
  if (len > 0)
    {
      /* open and get the comment */
      fd = mus_file_open_read(name);
      if (fd == -1) return(NULL);
      lseek(fd, start, SEEK_SET);
      sc = (char *)CALLOC(len + 1, sizeof(char));
      read(fd, sc, len);
      CLOSE(fd);
      if (((mus_sound_header_type(name) == MUS_AIFF) || 
	   (mus_sound_header_type(name) == MUS_AIFC)) &&
	  (sf->aux_comment_start))
	{
	  auxcom = mus_header_aiff_aux_comment(name, 
					       sf->aux_comment_start, 
					       sf->aux_comment_end);
	  if (auxcom)
	    {
	      full_len = strlen(auxcom) + strlen(sc) + 2;
	      sc = (char *)REALLOC(sc, full_len * sizeof(char));
	      strcat(sc, "\n");
	      strcat(sc, auxcom);
	    }
	}
    }
  return(sc);
}

int mus_sound_open_input (const char *arg) 
{
  int fd;
  sound_file *sf = NULL;
  mus_sound_initialize();
  fd = mus_file_open_read(arg);
  if (fd != -1)
    {
      if ((sf = find_sound_file(arg)) == NULL)
	sf = read_sound_file_header_with_fd(fd, arg);
      if (sf == NULL)
	{
	  CLOSE(fd);
	  fd = -1;
	}
    }
  if (sf) 
    {
      mus_file_open_descriptors(fd, arg, sf->data_format, sf->datum_size, sf->data_location, sf->chans, sf->header_type);
      lseek(fd, sf->data_location, SEEK_SET);
    }
  else mus_error(MUS_CANT_OPEN_FILE, S_mus_sound_open_input " can't open %s: %s", arg, strerror(errno));
  return(fd);
}

int mus_sound_open_output (const char *arg, int srate, int chans, int data_format, int header_type, const char *comment)
{
  int fd = MUS_ERROR, err, comlen = 0;
  if (comment) comlen = strlen(comment);
  mus_sound_initialize();
  mus_sound_forget(arg);
  err = mus_header_write(arg, header_type, srate, chans, 0, 0, data_format, comment, comlen);
  if (err != MUS_ERROR)
    {
      fd = mus_file_open_write(arg);
      if (fd != -1)
	mus_file_open_descriptors(fd,
				  arg,
				  data_format,
				  mus_bytes_per_sample(data_format),
				  mus_header_data_location(),
				  chans,
				  header_type);
    }
  return(fd);
}

int mus_sound_reopen_output(const char *arg, int chans, int format, int type, off_t data_loc)
{
  int fd;
  mus_sound_initialize();
  fd = mus_file_reopen_write(arg);
  if (fd != -1)
    mus_file_open_descriptors(fd,
			      arg,
			      format,
			      mus_bytes_per_sample(format),
			      data_loc,
			      chans,
			      type);
  return(fd);
}

int mus_sound_close_input (int fd) 
{
  return(mus_file_close(fd)); /* this closes the clm file descriptors */
}

int mus_sound_close_output (int fd, off_t bytes_of_data) 
{
  char *name;
  name = mus_file_fd_name(fd);
  if (name)
    {
      mus_sound_forget(name);
      mus_header_update_with_fd(fd, mus_file_header_type(fd), bytes_of_data);
      return(mus_file_close(fd));
    }
  return(MUS_ERROR);
}

int mus_sound_read (int fd, int beg, int end, int chans, mus_sample_t **bufs) 
{
  return(mus_file_read(fd, beg, end, chans, bufs));
}

int mus_sound_write (int tfd, int beg, int end, int chans, mus_sample_t **bufs) 
{
  return(mus_file_write(tfd, beg, end, chans, bufs));
}

off_t mus_sound_seek_frame(int tfd, off_t frame)
{
  return(mus_file_seek_frame(tfd, frame));
}

enum {SF_CHANS, SF_SRATE, SF_TYPE, SF_FORMAT, SF_LOCATION, SF_SIZE};

static int mus_sound_set_field(const char *arg, int field, int val)
{
  sound_file *sf; 
  sf = getsf(arg); 
  if (sf) 
    {
      switch (field)
	{
	case SF_CHANS:    sf->chans = val; break;
	case SF_SRATE:    sf->srate = val; break;
	case SF_TYPE:     sf->header_type = val; break;
	case SF_FORMAT:   sf->data_format = val; sf->datum_size = mus_bytes_per_sample(val); break;
	default: return(MUS_ERROR); break;
	}
      return(MUS_NO_ERROR);
    }
  return(MUS_ERROR);
}

static int mus_sound_set_off_t_field(const char *arg, int field, off_t val)
{
  sound_file *sf; 
  sf = getsf(arg); 
  if (sf) 
    {
      switch (field)
	{
	case SF_SIZE:     sf->samples = val; break;
	case SF_LOCATION: sf->data_location = val; break;
	default: return(MUS_ERROR); break;
	}
      return(MUS_NO_ERROR);
    }
  return(MUS_ERROR);
}

int mus_sound_set_chans(const char *arg, int val) {return(mus_sound_set_field(arg, SF_CHANS, val));}
int mus_sound_set_srate(const char *arg, int val) {return(mus_sound_set_field(arg, SF_SRATE, val));}
int mus_sound_set_header_type(const char *arg, int val) {return(mus_sound_set_field(arg, SF_TYPE, val));}
int mus_sound_set_data_format(const char *arg, int val) {return(mus_sound_set_field(arg, SF_FORMAT, val));}
int mus_sound_set_data_location(const char *arg, off_t val) {return(mus_sound_set_off_t_field(arg, SF_LOCATION, val));}
int mus_sound_set_samples(const char *arg, off_t val) {return(mus_sound_set_off_t_field(arg, SF_SIZE, val));}

int mus_sound_override_header(const char *arg, int srate, int chans, int format, int type, off_t location, off_t size)
{
  sound_file *sf; 
  /* perhaps once a header has been over-ridden, we should not reset the relevant fields upon re-read? */
  sf = getsf(arg); 
  if (sf)
    {
      if (location != -1) sf->data_location = location;
      if (size != -1) sf->samples = size;
      if (format != -1) 
	{
	  sf->data_format = format;
	  sf->datum_size = mus_bytes_per_sample(format);
	}
      if (srate != -1) sf->srate = srate;
      if (chans != -1) sf->chans = chans;
      if (type != -1) sf->header_type = type;
      return(MUS_NO_ERROR);
    }
  return(MUS_ERROR);
}

off_t mus_sound_maxamp(const char *ifile, mus_sample_t *vals)
{
  /* for CLM (ffi.lisp) */
  mus_sample_t *mvals;
  off_t *mtimes;
  int chans, i, j;
  off_t frames;
  chans = mus_sound_chans(ifile);
  mvals = (mus_sample_t *)CALLOC(chans, sizeof(mus_sample_t));
  mtimes = (off_t *)CALLOC(chans, sizeof(off_t));
  frames = mus_sound_maxamps(ifile, chans, mvals, mtimes);
  for (i = 0, j = 0; i < chans; i++, j += 2)
    {
      vals[j] = (mus_sample_t)(mtimes[i]);
      vals[j + 1] = mvals[i];
    }
  FREE(mvals);
  FREE(mtimes);
  return(frames);
}

int mus_sound_maxamp_exists(const char *ifile)
{
  sound_file *sf; 
  int val = 0;
  sf = getsf(ifile); 
  val = ((sf) && (sf->maxamps));
  return(val);
}

off_t mus_sound_maxamps(const char *ifile, int chans, mus_sample_t *vals, off_t *times)
{
  int ifd, ichans, chn, j;
  unsigned int i, bufnum, curframes;
  off_t n, frames;
  mus_sample_t abs_samp;
  mus_sample_t *buffer, *samp;
  off_t *time;
  mus_sample_t **ibufs;
  sound_file *sf; 
  sf = getsf(ifile); 
  if (sf->chans <= 0) return(MUS_ERROR);
  if ((sf) && (sf->maxamps))
    {
      if (chans > sf->chans) ichans = sf->chans; else ichans = chans;
      for (chn = 0; chn < ichans; chn++)
	{
	  times[chn] = sf->maxtimes[chn];
	  vals[chn] = sf->maxamps[chn];
	}
      frames = sf->samples / sf->chans;
      return(frames);
    }
  ifd = mus_sound_open_input(ifile);
  if (ifd == MUS_ERROR) return(MUS_ERROR);
  ichans = mus_sound_chans(ifile);
  frames = mus_sound_frames(ifile);
  if (frames == 0) 
    {
      mus_sound_close_input(ifd);
      return(0);
    }
  mus_sound_seek_frame(ifd, 0);
  ibufs = (mus_sample_t **)CALLOC(ichans, sizeof(mus_sample_t *));
  bufnum = 8192;
  for (j = 0; j < ichans; j++) 
    ibufs[j] = (mus_sample_t *)CALLOC(bufnum, sizeof(mus_sample_t));
  time = (off_t *)CALLOC(ichans, sizeof(off_t));
  samp = (mus_sample_t *)CALLOC(ichans, sizeof(mus_sample_t));
  for (n = 0; n < frames; n += bufnum)
    {
      if ((n + bufnum) < frames) 
	curframes = bufnum; 
      else curframes = (frames - n);
      mus_sound_read(ifd, 0, curframes - 1, ichans, ibufs);
      for (chn = 0; chn < ichans; chn++)
	{
	  buffer = (mus_sample_t *)(ibufs[chn]);
	  for (i = 0; i < curframes; i++) 
	    {
	      abs_samp = mus_sample_abs(buffer[i]);
	      if (abs_samp > samp[chn])
		{
		  time[chn] = i + n; 
		  samp[chn] = abs_samp;
		}
	    }
	}
    }
  mus_sound_close_input(ifd);
  mus_sound_set_maxamps(ifile, ichans, samp, time); /* save the complete set */
  if (ichans > chans) ichans = chans;
  for (chn = 0; chn < ichans; chn++)
    {
      times[chn] = time[chn];
      vals[chn] = samp[chn];
    }
  FREE(time);
  FREE(samp);
  for (j = 0; j < ichans; j++) FREE(ibufs[j]);
  FREE(ibufs);
  return(frames);
}

int mus_sound_set_maxamps(const char *ifile, int chans, mus_sample_t *vals, off_t *times)
{
  int i, ichans = 0;
  sound_file *sf; 
  sf = getsf(ifile); 
  if (sf)
    {
      if (sf->maxamps)
	{
	  if (chans > sf->chans) ichans = sf->chans; else ichans = chans;
	  for (i = 0; i < ichans; i++)
	    {
	      sf->maxtimes[i] = times[i];
	      sf->maxamps[i] = vals[i];
	    }
	}
      else
	{
	  ichans = mus_sound_chans(ifile);
	  if (sf->maxamps == NULL) 
	    {
	      sf->maxamps = (mus_sample_t *)CALLOC(ichans, sizeof(mus_sample_t));
	      sf->maxtimes = (off_t *)CALLOC(ichans, sizeof(off_t));
	    }
	  if (ichans > chans) ichans = chans;
	  for (i = 0; i < ichans; i++)
	    {
	      sf->maxtimes[i] = times[i];
	      sf->maxamps[i] = vals[i];
	    }
	}
      return(MUS_NO_ERROR);
    }
  return(MUS_ERROR);
}

int mus_file_to_array(const char *filename, int chan, int start, int samples, mus_sample_t *array)
{
  int ifd, chans, total_read;
  mus_sample_t **bufs;
  ifd = mus_sound_open_input(filename);
  if (ifd == MUS_ERROR) return(MUS_ERROR);
  chans = mus_sound_chans(filename);
  if (chan >= chans) 
    {
      mus_sound_close_input(ifd);      
      return(mus_error(MUS_NO_SUCH_CHANNEL, "mus_file_to_array can't read %s channel %d (file has %d chans)", filename, chan, chans));
    }
  bufs = (mus_sample_t **)CALLOC(chans, sizeof(mus_sample_t *));
  bufs[chan] = array;
  mus_sound_seek_frame(ifd, start);
  total_read = mus_file_read_any(ifd, 0, chans, samples, bufs, (mus_sample_t *)bufs);
  mus_sound_close_input(ifd);
  FREE(bufs);
  return(total_read);
}

char *mus_array_to_file_with_error(const char *filename, mus_sample_t *ddata, int len, int srate, int channels)
{
  /* put ddata into a sound file, taking byte order into account */
  /* assume ddata is interleaved already if more than one channel */
  int fd, err = MUS_NO_ERROR;
  mus_sample_t *bufs[1];
  mus_sound_forget(filename);
  fd = mus_file_create(filename);
  if (fd == -1) 
    return("mus_array_to_file can't create output file");
  err = mus_file_open_descriptors(fd, filename,
				  MUS_OUT_FORMAT,
				  mus_bytes_per_sample(MUS_OUT_FORMAT),
				  28, channels, MUS_NEXT);
  if (err != MUS_ERROR)
    {
      err = mus_header_write_next_header(fd, srate, channels, 28, len * sizeof(mus_sample_t), MUS_OUT_FORMAT, NULL, 0);
      if (err != MUS_ERROR)
	{
	  bufs[0] = ddata;
	  err = mus_file_write(fd, 0, len - 1, 1, bufs);
	}
    }
  mus_file_close(fd);
  if (err == MUS_ERROR)
    return("mus_array_to_file write error");
  return(NULL);
}

int mus_array_to_file(const char *filename, mus_sample_t *ddata, int len, int srate, int channels)
{
  char *errmsg;
  errmsg = mus_array_to_file_with_error(filename, ddata, len, srate, channels);
  if (errmsg)
    return(mus_error(MUS_CANT_OPEN_FILE, errmsg));
  return(MUS_NO_ERROR);
}
