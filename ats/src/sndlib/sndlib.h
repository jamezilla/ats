#ifndef SNDLIB_H
#define SNDLIB_H

#define SNDLIB_VERSION 18
#define SNDLIB_REVISION 6
#define SNDLIB_DATE "6-June-03"

/* try to figure out what type of machine (and in worst case, what OS) we're running on */

#if defined(HAVE_CONFIG_H)
  #include <config.h>
  #if (!defined(WORDS_BIGENDIAN))
     #define MUS_LITTLE_ENDIAN 1
  #endif
#else
  #ifdef __LITTLE_ENDIAN__
    #define MUS_LITTLE_ENDIAN 1
  #else
    #ifdef BYTE_ORDER
      #if (BYTE_ORDER == LITTLE_ENDIAN)
        #define MUS_LITTLE_ENDIAN 1
      #else
        #if __INTEL__
          #define MUS_LITTLE_ENDIAN 1
        #endif
      #endif
    #endif
  #endif
  #if USE_SND && (!(defined(_FILE_OFFSET_BITS)))
    #define _FILE_OFFSET_BITS 64
  #endif
#endif

#if defined(ALPHA) || defined(WINDOZE) || defined(__alpha__)
  #define MUS_LITTLE_ENDIAN 1
#endif

#if (!(defined(MACOS))) && (defined(MPW_C) || defined(macintosh) || defined(__MRC__))
  #define MACOS 1
  #include <MacMemory.h>
  #include <TextUtils.h>
  #include <Gestalt.h>
#endif

#ifndef TRUE
  #define TRUE 1
#endif
#ifndef FALSE
  #define FALSE 0
#endif

#if defined(macosx)
  #define MAC_OSX 1
#endif

#if (!defined(SGI)) && (!defined(LINUX)) && (!defined(MACOS)) && (!defined(SUN)) && (!defined(UW2)) && (!defined(SCO5)) && (!defined(ALPHA)) && (!defined(WINDOZE)) && (!defined(MAC_OSX))
  #if defined(__dest_os)
    /* we're in Metrowerks Land */
    #if (__dest_os == __mac_os)
      #define MACOS 1
    #endif
  #else
    #if macintosh
      #define MACOS 1
    #else
      #if (__WINDOWS__) || (__NT__) || (_WIN32) || (__CYGWIN__)
        #define WINDOZE 1
        #define MUS_LITTLE_ENDIAN 1
      #else
        #ifdef __alpha__
          #define ALPHA 1
          #define MUS_LITTLE_ENDIAN 1
        #endif
      #endif
    #endif
  #endif
#endif  

#if MACOS
  #define off_t long
#else
  #include <sys/types.h>
#endif

#if (defined(SIZEOF_OFF_T) && (SIZEOF_OFF_T > 4)) || (defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64))
  #define OFF_TD "%lld"
#else
  #define OFF_TD "%d"
#endif

#ifndef MUS_LITTLE_ENDIAN
  #define MUS_LITTLE_ENDIAN 0
#endif

#ifndef __GNUC__
  #ifndef __FUNCTION__
    #define __FUNCTION__ ""
  #endif
#endif

/* this block needed because not everyone uses configure, and those who don't often have no clue what audio system they're using */
/*   so, if nothing is set but we're on a system that looks linux-like and we can find the OSS headers, use OSS */

#ifndef ESD
#ifndef HAVE_OSS
#ifndef HAVE_ALSA
  #if defined(LINUX) || defined(SCO5) || defined(UW2) || defined(HAVE_SOUNDCARD_H) || defined(HAVE_SYS_SOUNDCARD_H) || defined(HAVE_MACHINE_SOUNDCARD_H) || defined(USR_LIB_OSS) || defined(USR_LOCAL_LIB_OSS) || defined(OPT_OSS) || defined(VAR_LIB_OSS) || defined(__FreeBSD__) || defined(__bsdi__)
    #define HAVE_OSS 1
  #else
    #define HAVE_OSS 0
  #endif
#else
  /* this branch may be obsolete with Fernando's new OSS+Alsa code -- need to test it */
  #define HAVE_OSS 0
#endif
#endif
#endif

#if (!defined(M_PI))
  #define M_PI 3.14159265358979323846264338327
  #define M_PI_2 (M_PI / 2.0)
#endif

#define TWO_PI (2.0 * M_PI)

#define POWER_OF_2_P(x)	((((x) - 1) & (x)) == 0)
/* from sys/param.h */

#ifndef SEEK_SET
  #define SEEK_SET 0
  #define SEEK_END 2
#endif

#if (!SNDLIB_USE_FLOATS)
  #define mus_sample_t int
  #ifndef MUS_SAMPLE_BITS
    #define MUS_SAMPLE_BITS 24
  #endif
  #define MUS_SAMPLE_0 0
  #define MUS_BYTE_TO_SAMPLE(n) ((mus_sample_t)((n) << (MUS_SAMPLE_BITS - 8)))
  #define MUS_SAMPLE_TO_BYTE(n) ((n) >> (MUS_SAMPLE_BITS - 8))
  #define MUS_SHORT_TO_SAMPLE(n) ((mus_sample_t)((n) << (MUS_SAMPLE_BITS - 16)))
  #define MUS_SAMPLE_TO_SHORT(n) ((short)((n) >> (MUS_SAMPLE_BITS - 16)))
  #if (MUS_SAMPLE_BITS < 24)
    #define MUS_INT24_TO_SAMPLE(n) ((mus_sample_t)((n) >> (24 - MUS_SAMPLE_BITS)))
    #define MUS_SAMPLE_TO_INT24(n) ((int)((n) << (24 - MUS_SAMPLE_BITS)))
  #else
    #define MUS_INT24_TO_SAMPLE(n) ((mus_sample_t)((n) << (MUS_SAMPLE_BITS - 24)))
    #define MUS_SAMPLE_TO_INT24(n) ((int)((n) >> (MUS_SAMPLE_BITS - 24)))
  #endif
  #define MUS_INT_TO_SAMPLE(n) ((mus_sample_t)(n))
  #define MUS_SAMPLE_TO_INT(n) ((int)(n))
  /* these are for direct read/write (no cross-image assumption is made about 32 bit int scaling) */
  #define MUS_FLOAT_TO_FIX ((MUS_SAMPLE_BITS < 32) ? (1 << (MUS_SAMPLE_BITS - 1)) : 0x7fffffff)
  #define MUS_FIX_TO_FLOAT (1.0 / (float)(MUS_FLOAT_TO_FIX))
  #define MUS_FLOAT_TO_SAMPLE(n) ((mus_sample_t)((n) * MUS_FLOAT_TO_FIX))
  #define MUS_SAMPLE_TO_FLOAT(n) ((float)((n) * MUS_FIX_TO_FLOAT))
  #define MUS_DOUBLE_TO_SAMPLE(n) ((mus_sample_t)((n) * MUS_FLOAT_TO_FIX))
  #define MUS_SAMPLE_TO_DOUBLE(n) ((double)((n) * MUS_FIX_TO_FLOAT))
  #define MUS_SAMPLE_MAX ((mus_sample_t)((MUS_SAMPLE_BITS < 32) ? (MUS_FLOAT_TO_FIX - 1) : 0x7fffffff))
  #define MUS_SAMPLE_MIN ((mus_sample_t)((MUS_SAMPLE_BITS < 32) ? (-(MUS_FLOAT_TO_FIX)) : -0x7fffffff))
  #define mus_sample_abs(Sample) abs(Sample)
#else
  /* this could use Float throughout and reflect the Float = double choice elsewhere */
  #define mus_sample_t float
  #ifndef MUS_SAMPLE_BITS
    #define MUS_SAMPLE_BITS 24
  #endif
  #define MUS_SAMPLE_0 0.0
  #define MUS_BYTE_TO_SAMPLE(n) ((mus_sample_t)((float)(n) / (float)(1 << 7)))
  #define MUS_SHORT_TO_SAMPLE(n) ((mus_sample_t)((float)(n) / (float)(1 << 15)))
  #define MUS_INT_TO_SAMPLE(n) ((mus_sample_t)((float)(n) / (float)(1 << (MUS_SAMPLE_BITS - 1))))
  #define MUS_INT24_TO_SAMPLE(n) ((mus_sample_t)((float)(n) / (float)(1 << 23)))
  #define MUS_FLOAT_TO_FIX 1.0
  #define MUS_FIX_TO_FLOAT 1.0
  #define MUS_FLOAT_TO_SAMPLE(n) ((mus_sample_t)(n))
  #define MUS_DOUBLE_TO_SAMPLE(n) ((mus_sample_t)(n))
  #define MUS_SAMPLE_TO_FLOAT(n) ((float)(n))
  #define MUS_SAMPLE_TO_DOUBLE(n) ((double)(n))
  #define MUS_SAMPLE_TO_INT(n) ((int)((n) * (1 << (MUS_SAMPLE_BITS - 1))))
  #define MUS_SAMPLE_TO_INT24(n) ((int)((n) * (1 << 23)))
  #define MUS_SAMPLE_TO_SHORT(n) ((short)((n) * (1 << 15)))
  #define MUS_SAMPLE_TO_BYTE(n) ((char)((n) * (1 << 7)))
  #define MUS_SAMPLE_MAX 0.99999
  #define MUS_SAMPLE_MIN (-1.0)
  #define mus_sample_abs(Sample) fabs(Sample)
#endif

#define MUS_DAC_CHANNEL 252525
#define MUS_DAC_REVERB 252520

#define MUS_UNSUPPORTED -1
enum {MUS_NEXT, MUS_AIFC, MUS_RIFF, MUS_BICSF, MUS_NIST, MUS_INRS, MUS_ESPS, MUS_SVX, MUS_VOC, MUS_SNDT, MUS_RAW,
      MUS_SMP, MUS_AVR, MUS_IRCAM, MUS_SD1, MUS_SPPACK, MUS_MUS10, MUS_HCOM, MUS_PSION, MUS_MAUD,
      MUS_IEEE, MUS_MATLAB, MUS_ADC, MUS_MIDI, MUS_SOUNDFONT, MUS_GRAVIS, MUS_COMDISCO, MUS_GOLDWAVE, MUS_SRFS,
      MUS_MIDI_SAMPLE_DUMP, MUS_DIAMONDWARE, MUS_ADF, MUS_SBSTUDIOII, MUS_DELUSION,
      MUS_FARANDOLE, MUS_SAMPLE_DUMP, MUS_ULTRATRACKER, MUS_YAMAHA_SY85, MUS_YAMAHA_TX16W, MUS_DIGIPLAYER,
      MUS_COVOX, MUS_SPL, MUS_AVI, MUS_OMF, MUS_QUICKTIME, MUS_ASF, MUS_YAMAHA_SY99, MUS_KURZWEIL_2000,
      MUS_AIFF, MUS_PAF, MUS_CSL, MUS_FILE_SAMP, MUS_PVF, MUS_SOUNDFORGE, MUS_TWINVQ, MUS_AKAI4,
      MUS_IMPULSETRACKER, MUS_KORG, MUS_MAUI};

#define MUS_HEADER_TYPE_OK(n) (((n) > MUS_UNSUPPORTED) && ((n) <= MUS_MAUI))

enum {MUS_UNKNOWN, MUS_BSHORT, MUS_MULAW, MUS_BYTE, MUS_BFLOAT, MUS_BINT, MUS_ALAW, MUS_UBYTE, MUS_B24INT,
      MUS_BDOUBLE, MUS_LSHORT, MUS_LINT, MUS_LFLOAT, MUS_LDOUBLE, MUS_UBSHORT, MUS_ULSHORT, MUS_L24INT,
      MUS_BINTN, MUS_LINTN, MUS_BFLOAT_UNSCALED, MUS_LFLOAT_UNSCALED, MUS_BDOUBLE_UNSCALED, MUS_LDOUBLE_UNSCALED};

/* MUS_LINTN and MUS_BINTN refer to 32 bit ints with 31 bits of "fraction" -- the data is "left justified" */
/* "unscaled" means the float value is used directly (i.e. not as -1.0 to 1.0, but (probably) -32768.0 to 32768.0) */

#define MUS_DATA_FORMAT_OK(n) (((n) > MUS_UNKNOWN) && ((n) <= MUS_LDOUBLE_UNSCALED))
#define MUS_LAST_DATA_FORMAT MUS_LDOUBLE_UNSCALED

#if MAC_OSX
  #define MUS_COMPATIBLE_FORMAT MUS_BFLOAT
#else
  #if MUS_LITTLE_ENDIAN
    #define MUS_COMPATIBLE_FORMAT MUS_LSHORT
  #else
    #define MUS_COMPATIBLE_FORMAT MUS_BSHORT
  #endif
#endif

#if MUS_LITTLE_ENDIAN
  #if SNDLIB_USE_FLOATS
    #define MUS_OUT_FORMAT MUS_LFLOAT
  #else
    #define MUS_OUT_FORMAT MUS_LINT
  #endif
#else
  #if SNDLIB_USE_FLOATS
    #define MUS_OUT_FORMAT MUS_BFLOAT
  #else
    #define MUS_OUT_FORMAT MUS_BINT
  #endif
#endif


#define MUS_NIST_SHORTPACK 2
#define MUS_AIFF_IMA_ADPCM 99

#define MUS_AUDIO_PACK_SYSTEM(n) ((n) << 16)
#define MUS_AUDIO_SYSTEM(n) (((n) >> 16) & 0xffff)
#define MUS_AUDIO_DEVICE(n) ((n) & 0xffff)

enum {MUS_AUDIO_DEFAULT, MUS_AUDIO_DUPLEX_DEFAULT, MUS_AUDIO_ADAT_IN, MUS_AUDIO_AES_IN, MUS_AUDIO_LINE_OUT,
      MUS_AUDIO_LINE_IN, MUS_AUDIO_MICROPHONE, MUS_AUDIO_SPEAKERS, MUS_AUDIO_DIGITAL_IN, MUS_AUDIO_DIGITAL_OUT,
      MUS_AUDIO_DAC_OUT, MUS_AUDIO_ADAT_OUT, MUS_AUDIO_AES_OUT, MUS_AUDIO_DAC_FILTER, MUS_AUDIO_MIXER,
      MUS_AUDIO_LINE1, MUS_AUDIO_LINE2, MUS_AUDIO_LINE3, MUS_AUDIO_AUX_INPUT, MUS_AUDIO_CD,
      MUS_AUDIO_AUX_OUTPUT, MUS_AUDIO_SPDIF_IN, MUS_AUDIO_SPDIF_OUT, MUS_AUDIO_AMP, MUS_AUDIO_SRATE,
      MUS_AUDIO_CHANNEL, MUS_AUDIO_FORMAT, MUS_AUDIO_IMIX, MUS_AUDIO_IGAIN, MUS_AUDIO_RECLEV,
      MUS_AUDIO_PCM, MUS_AUDIO_PCM2, MUS_AUDIO_OGAIN, MUS_AUDIO_LINE, MUS_AUDIO_SYNTH,
      MUS_AUDIO_BASS, MUS_AUDIO_TREBLE, MUS_AUDIO_PORT, MUS_AUDIO_SAMPLES_PER_CHANNEL,
      MUS_AUDIO_DIRECTION
};
/* Snd's recorder uses MUS_AUDIO_DIRECTION to find the size of this list */

#define MUS_AUDIO_DEVICE_OK(a) (((a) >= MUS_AUDIO_DEFAULT) && ((a) <= MUS_AUDIO_DIRECTION))

#define MUS_ERROR -1

enum {MUS_NO_ERROR, MUS_NO_FREQUENCY, MUS_NO_PHASE, MUS_NO_GEN, MUS_NO_LENGTH,
      MUS_NO_FREE, MUS_NO_DESCRIBE, MUS_NO_DATA, MUS_NO_SCALER,
      MUS_MEMORY_ALLOCATION_FAILED, MUS_UNSTABLE_TWO_POLE_ERROR,
      MUS_CANT_OPEN_FILE, MUS_NO_SAMPLE_INPUT, MUS_NO_SAMPLE_OUTPUT,
      MUS_NO_SUCH_CHANNEL, MUS_NO_FILE_NAME_PROVIDED, MUS_NO_LOCATION, MUS_NO_CHANNEL,
      MUS_NO_SUCH_FFT_WINDOW, MUS_UNSUPPORTED_DATA_FORMAT, MUS_HEADER_READ_FAILED,
      MUS_UNSUPPORTED_HEADER_TYPE,
      MUS_FILE_DESCRIPTORS_NOT_INITIALIZED, MUS_NOT_A_SOUND_FILE, MUS_FILE_CLOSED, MUS_WRITE_ERROR,
      MUS_BOGUS_FREE, MUS_BUFFER_OVERFLOW, MUS_BUFFER_UNDERFLOW, MUS_FILE_OVERFLOW,
      MUS_HEADER_WRITE_FAILED, MUS_CANT_OPEN_TEMP_FILE, MUS_INTERRUPTED, MUS_BAD_ENVELOPE,

      MUS_AUDIO_CHANNELS_NOT_AVAILABLE, MUS_AUDIO_SRATE_NOT_AVAILABLE, MUS_AUDIO_FORMAT_NOT_AVAILABLE,
      MUS_AUDIO_NO_INPUT_AVAILABLE, MUS_AUDIO_CONFIGURATION_NOT_AVAILABLE, 
      MUS_AUDIO_NO_LINES_AVAILABLE, MUS_AUDIO_WRITE_ERROR, MUS_AUDIO_SIZE_NOT_AVAILABLE, MUS_AUDIO_DEVICE_NOT_AVAILABLE,
      MUS_AUDIO_CANT_CLOSE, MUS_AUDIO_CANT_OPEN, MUS_AUDIO_READ_ERROR, MUS_AUDIO_AMP_NOT_AVAILABLE,
      MUS_AUDIO_CANT_WRITE, MUS_AUDIO_CANT_READ, MUS_AUDIO_NO_READ_PERMISSION,
      MUS_CANT_CLOSE_FILE, MUS_ARG_OUT_OF_RANGE,
      MUS_MIDI_OPEN_ERROR, MUS_MIDI_READ_ERROR, MUS_MIDI_WRITE_ERROR, MUS_MIDI_CLOSE_ERROR, MUS_MIDI_INIT_ERROR, MUS_MIDI_MISC_ERROR,

      MUS_NO_CHANNELS, MUS_NO_HOP, MUS_NO_WIDTH, MUS_NO_FILE_NAME, MUS_NO_RAMP, MUS_NO_RUN, 
      MUS_NO_INCREMENT, MUS_NO_B2, MUS_NO_INSPECT, MUS_NO_OFFSET,
      MUS_NO_X1, MUS_NO_Y1, MUS_NO_X2, MUS_NO_Y2,
      MUS_INITIAL_ERROR_TAG};

/* keep this list in sync with error_names in sound.c and initmus.lisp */

#ifdef MACOS
  /* C's calloc/free are incompatible with Mac's SndDisposeChannel (which we can't avoid using) */
  /*   FREE is used only when we call either CALLOC or MALLOC ourselves -- other cases use free, g_free, XtFree, etc */
  #define CALLOC(a, b)  NewPtrClear((a) * (b))
  #define MALLOC(a)     NewPtr((a))
  #define FREE(a)       DisposePtr((Ptr)(a))
  #define REALLOC(a, b) NewPtr_realloc((Ptr)(a), (Size)(b))
  /* implementation in io.c */
  Ptr NewPtr_realloc(Ptr p, Size newSize);

  #define OPEN(File, Flags, Mode) open((File), (Flags))
  #define FOPEN(File, Flags) fopen((File), (Flags))
  #define CLOSE(Fd) close(Fd)
  #define FCLOSE(Fd) fclose(Fd)
  #ifdef MPW_C
    #define CREAT(File, Flags) creat((File))
  #else
    #define CREAT(File, Flags) creat((File), 0)
  #endif
#else
  #ifdef DEBUG_MEMORY
    #define CALLOC(a, b)  mem_calloc((size_t)(a), (size_t)(b), __FUNCTION__, __FILE__, __LINE__)
    #define MALLOC(a)     mem_malloc((size_t)(a), __FUNCTION__, __FILE__, __LINE__)
    #define FREE(a)       mem_free(a, __FUNCTION__, __FILE__, __LINE__)
    #define REALLOC(a, b) mem_realloc(a, (size_t)(b), __FUNCTION__, __FILE__, __LINE__)

    #define OPEN(File, Flags, Mode) io_open((File), (Flags), (Mode), __FUNCTION__, __FILE__, __LINE__)
    #define FOPEN(File, Flags) io_fopen((File), (Flags), __FUNCTION__, __FILE__, __LINE__)
    #define CLOSE(Fd) io_close((Fd), __FUNCTION__, __FILE__, __LINE__)
    #define FCLOSE(Fd) io_fclose((Fd), __FUNCTION__, __FILE__, __LINE__)
    #define CREAT(File, Flags) io_creat((File), (Flags), __FUNCTION__, __FILE__, __LINE__)
  #else
    #define CALLOC(a, b)  calloc((size_t)(a), (size_t)(b))
    #define MALLOC(a)     malloc((size_t)(a))
    #define FREE(a)       free(a)
    #define REALLOC(a, b) realloc(a, (size_t)(b))

    #ifdef WINDOZE
      #ifdef FOPEN
        #undef FOPEN
      #endif
      #define OPEN(File, Flags, Mode) open((File), (Flags))
    #else
      #define OPEN(File, Flags, Mode) open((File), (Flags), (Mode))
    #endif
    #define FOPEN(File, Flags) fopen((File), (Flags))
    #define CLOSE(Fd) close(Fd)
    #define FCLOSE(Fd) fclose(Fd)
    #define CREAT(File, Flags) creat((File), (Flags))
  #endif
#endif 

#define MUS_MAX_FILE_NAME 256
#define MUS_LOOP_INFO_SIZE 8

#ifndef Float
  #define Float float
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -------- sound.c -------- */

#ifdef __GNUC__
  int mus_error(int error, const char *format, ...) __attribute__ ((format (printf, 2, 3)));
  void mus_print(const char *format, ...)           __attribute__ ((format (printf, 1, 2)));
  char *mus_format(const char *format, ...)         __attribute__ ((format (printf, 1, 2)));
  void mus_snprintf(char *buffer, int buffer_len, const char *format, ...)  __attribute__ ((format (printf, 3, 4)));
#else
  int mus_error(int error, const char *format, ...);
  void mus_print(const char *format, ...);
  char *mus_format(const char *format, ...);
  void mus_snprintf(char *buffer, int buffer_len, const char *format, ...);
#endif

typedef void mus_error_handler_t(int type, char *msg);
mus_error_handler_t *mus_error_set_handler (mus_error_handler_t *new_error_handler);
int mus_make_error(char *error_name);
const char *mus_error_to_string(int err);

typedef void mus_print_handler_t(char *msg);
mus_print_handler_t *mus_print_set_handler (mus_print_handler_t *new_print_handler);

off_t mus_sound_samples(const char *arg);
off_t mus_sound_frames(const char *arg);
int mus_sound_datum_size(const char *arg);
off_t mus_sound_data_location(const char *arg);
int mus_sound_chans(const char *arg);
int mus_sound_srate(const char *arg);
int mus_sound_header_type(const char *arg);
int mus_sound_data_format(const char *arg);
int mus_sound_original_format(const char *arg);
int mus_sound_comment_start(const char *arg);
int mus_sound_comment_end(const char *arg);
off_t mus_sound_length(const char *arg);
int mus_sound_fact_samples(const char *arg);
int mus_sound_write_date(const char *arg);
int mus_sound_type_specifier(const char *arg);
int mus_sound_align(const char *arg);
int mus_sound_bits_per_sample(const char *arg);

int mus_sound_set_chans(const char *arg, int val);
int mus_sound_set_srate(const char *arg, int val);
int mus_sound_set_header_type(const char *arg, int val);
int mus_sound_set_data_format(const char *arg, int val);
int mus_sound_set_data_location(const char *arg, off_t val);
int mus_sound_set_samples(const char *arg, off_t val);

const char *mus_header_type_name(int type);
const char *mus_data_format_name(int format);
const char *mus_short_data_format_name(int format);
char *mus_sound_comment(const char *name);
int mus_data_format_to_bytes_per_sample(int format);
#define mus_bytes_per_sample(Format) mus_data_format_to_bytes_per_sample(Format)
float mus_sound_duration(const char *arg);
int mus_sound_initialize(void);
int mus_sample_bits(void);
int mus_sound_override_header(const char *arg, int srate, int chans, int format, int type, off_t location, off_t size);
int mus_sound_forget(const char *name);
int mus_sound_prune(void);
#define mus_sound_print_cache() mus_sound_report_cache(stdout)
void mus_sound_report_cache(FILE *fp);
int *mus_sound_loop_info(const char *arg);
void mus_sound_set_full_loop_info(const char *arg, int *loop);

int mus_sound_open_input(const char *arg);
int mus_sound_open_output(const char *arg, int srate, int chans, int data_format, int header_type, const char *comment);
int mus_sound_reopen_output(const char *arg, int chans, int format, int type, off_t data_loc);
int mus_sound_close_input(int fd);
int mus_sound_close_output(int fd, off_t bytes_of_data);
int mus_sound_read(int fd, int beg, int end, int chans, mus_sample_t **bufs);
int mus_sound_write(int tfd, int beg, int end, int chans, mus_sample_t **bufs);
off_t mus_sound_seek_frame(int tfd, off_t frame);
off_t mus_sound_maxamp(const char *ifile, mus_sample_t *vals);
off_t mus_sound_maxamps(const char *ifile, int chans, mus_sample_t *vals, off_t *times);
int mus_sound_set_maxamps(const char *ifile, int chans, mus_sample_t *vals, off_t *times);
int mus_sound_maxamp_exists(const char *ifile);
int mus_file_to_array(const char *filename, int chan, int start, int samples, mus_sample_t *array);
int mus_array_to_file(const char *filename, mus_sample_t *ddata, int len, int srate, int channels);
char *mus_array_to_file_with_error(const char *filename, mus_sample_t *ddata, int len, int srate, int channels);


/* -------- audio.c -------- */

#if (HAVE_OSS || HAVE_ALSA)
  #define ALSA_API 0
  #define OSS_API 1
#endif

void mus_audio_describe(void);
char *mus_audio_report(void);
int mus_audio_open_output(int dev, int srate, int chans, int format, int size);
int mus_audio_open_input(int dev, int srate, int chans, int format, int size);
int mus_audio_write(int line, char *buf, int bytes);
int mus_audio_close(int line);
int mus_audio_read(int line, char *buf, int bytes);

int mus_audio_write_buffers(int line, int frames, int chans, mus_sample_t **bufs, int output_format, int clipped);
int mus_audio_read_buffers(int line, int frames, int chans, mus_sample_t **bufs, int input_format);

int mus_audio_mixer_read(int dev, int field, int chan, float *val);
int mus_audio_mixer_write(int dev, int field, int chan, float *val);
void mus_audio_save(void);
void mus_audio_restore(void);
int mus_audio_initialize(void);
#if HAVE_OSS
int mus_audio_reinitialize(void); /* 29-Aug-01 for CLM/Snd bugfix? */
#endif
int mus_audio_systems(void);
char *mus_audio_system_name(int sys);
char *mus_audio_moniker(void);

#if (HAVE_OSS || HAVE_ALSA)
  void mus_audio_set_oss_buffers(int num, int size);
  int mus_audio_api(void);
#endif
int mus_audio_compatible_format(int dev);

#ifdef SUN
  void mus_audio_sun_outputs(int speakers, int headphones, int line_out);
#endif

#if (defined(HAVE_CONFIG_H)) && (!HAVE_STRERROR)
  char *strerror(int errnum);
#endif



/* -------- io.c -------- */

int mus_file_set_descriptors(int tfd, const char *arg, int df, int ds, off_t dl, int dc, int dt);
#define mus_file_open_descriptors(Tfd, Arg, Df, Ds, Dl, Dc, Dt) mus_file_set_descriptors(Tfd, Arg, Df, Ds, Dl, Dc, Dt)
int mus_file_open_read(const char *arg);
int mus_file_probe(const char *arg);
int mus_file_open_write(const char *arg);
int mus_file_create(const char *arg);
int mus_file_reopen_write(const char *arg);
int mus_file_close(int fd);
off_t mus_file_seek_frame(int tfd, off_t frame);
int mus_file_read(int fd, int beg, int end, int chans, mus_sample_t **bufs);
int mus_file_read_chans(int fd, int beg, int end, int chans, mus_sample_t **bufs, mus_sample_t *cm);
off_t mus_file_write_zeros(int tfd, off_t num);
int mus_file_write(int tfd, int beg, int end, int chans, mus_sample_t **bufs);
int mus_file_read_any(int tfd, int beg, int chans, int nints, mus_sample_t **bufs, mus_sample_t *cm);
int mus_file_read_file(int tfd, int beg, int chans, int nints, mus_sample_t **bufs);
int mus_file_read_buffer(int charbuf_data_format, int beg, int chans, int nints, mus_sample_t **bufs, char *charbuf);
int mus_file_write_file(int tfd, int beg, int end, int chans, mus_sample_t **bufs);
int mus_file_write_buffer(int charbuf_data_format, int beg, int end, int chans, mus_sample_t **bufs, char *charbuf, int clipped);
char *mus_expand_filename(char *name);

int mus_file_data_clipped(int tfd);
int mus_file_set_data_clipped(int tfd, int clipped);
int mus_file_set_header_type(int tfd, int type);
int mus_file_header_type(int tfd);
char *mus_file_fd_name(int tfd);
int mus_file_set_chans(int tfd, int chans);
float mus_file_prescaler(int tfd);
float mus_file_set_prescaler(int tfd, float val);

void mus_bint_to_char(unsigned char *j, int x);
int mus_char_to_bint(const unsigned char *inp);
void mus_lint_to_char(unsigned char *j, int x);
int mus_char_to_lint(const unsigned char *inp);
int mus_char_to_uninterpreted_int(const unsigned char *inp);
void mus_bfloat_to_char(unsigned char *j, float x);
float mus_char_to_bfloat(const unsigned char *inp);
void mus_lfloat_to_char(unsigned char *j, float x);
float mus_char_to_lfloat(const unsigned char *inp);
void mus_bshort_to_char(unsigned char *j, short x);
short mus_char_to_bshort(const unsigned char *inp);
void mus_lshort_to_char(unsigned char *j, short x);
short mus_char_to_lshort(const unsigned char *inp);
void mus_ubshort_to_char(unsigned char *j, unsigned short x);
unsigned short mus_char_to_ubshort(const unsigned char *inp);
void mus_ulshort_to_char(unsigned char *j, unsigned short x);
unsigned short mus_char_to_ulshort(const unsigned char *inp);
double mus_char_to_ldouble(const unsigned char *inp);
double mus_char_to_bdouble(const unsigned char *inp);
void mus_bdouble_to_char(unsigned char *j, double x);
void mus_ldouble_to_char(unsigned char *j, double x);
unsigned int mus_char_to_ubint(const unsigned char *inp);
unsigned int mus_char_to_ulint(const unsigned char *inp);

int mus_iclamp(int lo, int val, int hi);
off_t mus_oclamp(off_t lo, off_t val, off_t hi);
Float mus_fclamp(Float lo, Float val, Float hi);

/* for CLM */
/* these are needed to clear a saved lisp image to the just-initialized state */
void mus_reset_io_c(void);
void mus_reset_headers_c(void);
void mus_reset_audio_c(void);
void mus_set_rt_audio_p(int rt);



/* -------- headers.c -------- */

off_t mus_header_samples(void);
off_t mus_header_data_location(void);
int mus_header_chans(void);
int mus_header_srate(void);
int mus_header_type(void);
int mus_header_format(void);
int mus_header_comment_start(void);
int mus_header_comment_end(void);
int mus_header_type_specifier(void);
int mus_header_bits_per_sample(void);
int mus_header_fact_samples(void);
int mus_header_block_align(void);
int mus_header_loop_mode(int which);
int mus_header_loop_start(int which);
int mus_header_loop_end(int which);
int mus_header_mark_position(int id);
int mus_header_base_note(void);
int mus_header_base_detune(void);
void mus_header_set_raw_defaults(int sr, int chn, int frm);
void mus_header_raw_defaults(int *sr, int *chn, int *frm);
off_t mus_header_true_length(void);
int mus_header_original_format(void);
off_t mus_samples_to_bytes(int format, off_t size);
off_t mus_bytes_to_samples(int format, off_t size);
int mus_header_write_next_header(int chan, int srate, int chans, int loc, int siz, int format, const char *comment, int len);
int mus_header_read_with_fd(int chan);
int mus_header_read(const char *name);
int mus_header_write(const char *name, int type, int srate, int chans, off_t loc, off_t size_in_samples, int format, const char *comment, int len);
int mus_header_update_with_fd(int chan, int type, off_t siz);
int mus_header_aux_comment_start(int n);
int mus_header_aux_comment_end(int n);
int mus_header_initialize(void);
int mus_header_writable(int type, int format);
void mus_header_set_full_aiff_loop_info(int *data);
int mus_header_sf2_entries(void);
char *mus_header_sf2_name(int n);
int mus_header_sf2_start(int n);
int mus_header_sf2_end(int n);
int mus_header_sf2_loop_start(int n);
int mus_header_sf2_loop_end(int n);
const char *mus_header_original_format_name(int format, int type);
int mus_header_no_header(const char *name);

char *mus_header_riff_aux_comment(const char *name, int *starts, int *ends);
char *mus_header_aiff_aux_comment(const char *name, int *starts, int *ends);

int mus_header_change_chans(const char *filename, int new_chans);
int mus_header_change_srate(const char *filename, int new_srate);
int mus_header_change_type(const char *filename, int new_type, int new_format);
int mus_header_change_format(const char *filename, int new_format);
int mus_header_change_location(const char *filename, off_t new_location);
int mus_header_change_comment(const char *filename, char *new_comment);
int mus_header_change_samples(const char *filename, off_t new_samples);

typedef void mus_header_write_hook_t(const char *filename);
mus_header_write_hook_t *mus_header_write_set_hook(mus_header_write_hook_t *new_hook);


/* -------- midi.c -------- */
int mus_midi_open_read(const char *name);
int mus_midi_open_write(const char *name);
int mus_midi_close(int line);
int mus_midi_read(int line, unsigned char *buffer, int bytes);
int mus_midi_write(int line, unsigned char *buffer, int bytes);
char *mus_midi_device_name(int sysdev);
char *mus_midi_describe(void);
#if HAVE_EXTENSION_LANGUAGE
  void mus_midi_init(void);
#endif


#ifdef DEBUG_MEMORY
  /* snd-utils.c (only used in conjunction with Snd's memory tracking functions) */
  void *mem_calloc(size_t len, size_t size, const char *func, const char *file, int line);
  void *mem_malloc(size_t len, const char *func, const char *file, int line);
  void mem_free(void *ptr, const char *func, const char *file, int line);
  void *mem_realloc(void *ptr, size_t size, const char *func, const char *file, int line);
  int io_open(const char *pathname, int flags, mode_t mode, const char *func, const char *file, int line);
  int io_creat(const char *pathname, mode_t mode, const char *func, const char *file, int line);
  int io_close(int fd, const char *func, const char *file, int line);
  FILE *io_fopen(const char *path, const char *mode, const char *func, const char *file, int line);
  int io_fclose(FILE *stream, const char *func, const char *file, int line);
#endif

#ifdef MPW_C
char *strdup(const char *str);
#endif

#ifdef __cplusplus
}
#endif

#endif
