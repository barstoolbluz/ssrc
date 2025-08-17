#include <stdio.h>
#include <stdlib.h>

#define DR_WAV_IMPLEMENTATION

#ifdef USE_SOX
#include <soxr.h>
#include "dr_wav.h"
#else
#define SSRC_LIBSOXR_EMULATION
#include "shibatch/ssrcsoxr.h"
#include "xdr_wav.h"
#endif

#define BUFFER_FRAMES 3000

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Usage: %s <input.wav> <output.wav> <new_rate>\n", argv[0]);
    return 1;
  }

  const char* in_filename = argv[1];
  const char* out_filename = argv[2];
  double const out_rate = atof(argv[3]);

  drwav wav_in;
  if (!drwav_init_file(&wav_in, in_filename, NULL)) {
    fprintf(stderr, "Failed to open input file: %s\n", in_filename);
    return 1;
  }

  double const in_rate = (double)wav_in.sampleRate;
  unsigned int const num_channels = wav_in.channels;

  printf("Input: %.0f Hz, %u channels\n", in_rate, num_channels);
  printf("Output: %.0f Hz\n", out_rate);

  soxr_error_t error;
  soxr_io_spec_t io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
  soxr_quality_spec_t q_spec = soxr_quality_spec(SOXR_MQ, 0);
  soxr_t soxr = soxr_create(in_rate, out_rate, num_channels, &error, &io_spec, &q_spec, NULL);
  if (!soxr) {
    fprintf(stderr, "soxr_create failed: %s\n", soxr_strerror(error));
    drwav_uninit(&wav_in);
    return 1;
  }

  printf("Delay: %.0f samples\n", soxr_delay(soxr));

  drwav_data_format format;
  format.container = drwav_container_riff;
  format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
  format.channels = num_channels;
  format.sampleRate = (drwav_uint32)out_rate;
  format.bitsPerSample = 32;

  drwav wav_out;
  if (!drwav_init_file_write(&wav_out, out_filename, &format, NULL)) {
    fprintf(stderr, "Failed to open output file: %s\n", out_filename);
    soxr_delete(soxr);
    drwav_uninit(&wav_in);
    return 1;
  }
    
  float* in_buffer = (float*)malloc(sizeof(float) * BUFFER_FRAMES * num_channels);
  size_t out_buffer_capacity = (size_t)(BUFFER_FRAMES * out_rate / in_rate + 0.5) + 16;
  float* out_buffer = (float*)malloc(sizeof(float) * out_buffer_capacity * num_channels);

  size_t frames_read;
  while ((frames_read = drwav_read_pcm_frames_f32(&wav_in, BUFFER_FRAMES, in_buffer)) > 0) {
    size_t frames_consumed;
    size_t frames_produced;

    error = soxr_process(soxr,
			 in_buffer, frames_read, &frames_consumed,
			 out_buffer, out_buffer_capacity, &frames_produced);

    if (error) fprintf(stderr, "soxr_process error: %s\n", soxr_strerror(error));

    if (frames_produced > 0) {
      drwav_write_pcm_frames(&wav_out, frames_produced, out_buffer);
    }
  }

  size_t frames_produced;
  do {
    error = soxr_process(soxr, NULL, 0, NULL, out_buffer, out_buffer_capacity, &frames_produced);
    if (error) fprintf(stderr, "soxr_process (flush) error: %s\n", soxr_strerror(error));
    if (frames_produced > 0) {
      drwav_write_pcm_frames(&wav_out, frames_produced, out_buffer);
    }
  } while (frames_produced > 0);

  free(in_buffer);
  free(out_buffer);
  soxr_delete(soxr);
  drwav_uninit(&wav_in);
  drwav_uninit(&wav_out);

  printf("Successfully created %s\n", out_filename);
  return 0;
}
