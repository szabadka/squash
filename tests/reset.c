#include "test-codecs.h"

static SquashStatus
buffer_to_buffer_compress_with_stream (SquashStream* stream,
                                       size_t* compressed_length,
                                       uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                       size_t uncompressed_length,
                                       const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)]) {
  size_t step_size = g_test_rand_int_range (64, 255);
  SquashStatus res;

	stream->next_out = compressed;
	stream->avail_out = (step_size < *compressed_length) ? step_size : *compressed_length;
  stream->next_in = uncompressed;

  while (stream->total_in < uncompressed_length) {
    stream->avail_in = MIN(uncompressed_length - stream->total_in, step_size);

    do {
      res = squash_stream_process (stream);

      if (stream->avail_out < step_size) {
        stream->avail_out = MIN(*compressed_length - stream->total_out, step_size);
      }
    } while (res == SQUASH_PROCESSING);

    SQUASH_ASSERT_OK(res);
  }

  do {
    stream->avail_out = MIN(*compressed_length - stream->total_out, step_size);

    res = squash_stream_finish (stream);
	} while (res == SQUASH_PROCESSING);

  if (res == SQUASH_OK) {
    *compressed_length = stream->total_out;
  }

  return res;
}

static SquashStatus
buffer_to_buffer_decompress_with_stream (SquashStream* stream,
                                         size_t* decompressed_length,
                                         uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                         size_t compressed_length,
                                         const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]) {
  size_t step_size = g_test_rand_int_range (64, 255);
  SquashStatus res = SQUASH_OK;

	stream->next_out = decompressed;
	stream->avail_out = (step_size < *decompressed_length) ? step_size : *decompressed_length;
  stream->next_in = compressed;

  while (stream->total_in < compressed_length &&
         stream->total_out < *decompressed_length) {
    stream->avail_in = MIN(compressed_length - stream->total_in, step_size);
    stream->avail_out = MIN(*decompressed_length - stream->total_out, step_size);

    res = squash_stream_process (stream);
    if (res == SQUASH_END_OF_STREAM || res < 0) {
      break;
    }
  }

  if (res == SQUASH_END_OF_STREAM) {
    res = SQUASH_OK;
  } else if (res > 0) {
    do {
      stream->avail_in = MIN(compressed_length - stream->total_in, step_size);
      stream->avail_out = MIN(*decompressed_length - stream->total_out, step_size);

      res = squash_stream_finish (stream);
    } while (res == SQUASH_PROCESSING);
  }

  if (res == SQUASH_OK) {
    *decompressed_length = stream->total_out;
  }

  return res;
}

void
check_codec (SquashCodec* codec) {
  SquashStream* compressor = squash_stream_new (codec, SQUASH_STREAM_COMPRESS, NULL);
  SquashStream* decompressor = squash_stream_new (codec, SQUASH_STREAM_DECOMPRESS, NULL);
  g_assert (compressor != NULL);
  g_assert (decompressor != NULL);

  size_t uncompressed1_size = g_test_rand_int_range (1, (LOREM_IPSUM_LENGTH / 4) * 3);
  size_t uncompressed2_size = LOREM_IPSUM_LENGTH - uncompressed1_size;
  size_t compressed1_size = squash_codec_get_max_compressed_size (codec, uncompressed1_size);
  size_t compressed2_size = squash_codec_get_max_compressed_size (codec, uncompressed2_size);
  size_t decompressed1_size = uncompressed1_size;
  size_t decompressed2_size = uncompressed2_size;

  uint8_t* compressed1 = g_malloc (compressed1_size);
  uint8_t* compressed2 = g_malloc (compressed2_size);
  uint8_t* decompressed1 = g_malloc (decompressed1_size);
  uint8_t* decompressed2 = g_malloc (decompressed2_size);

  SquashStatus res;

  res = buffer_to_buffer_compress_with_stream (compressor, &compressed1_size, compressed1, uncompressed1_size, (uint8_t*) LOREM_IPSUM);
  SQUASH_ASSERT_VALUE(res, SQUASH_OK);
  res = squash_stream_reset (compressor);
  SQUASH_ASSERT_VALUE(res, SQUASH_OK);
  res = buffer_to_buffer_compress_with_stream (compressor, &compressed2_size, compressed2, uncompressed2_size, (uint8_t*) LOREM_IPSUM + uncompressed1_size);
  SQUASH_ASSERT_VALUE(res, SQUASH_OK);

  res = buffer_to_buffer_decompress_with_stream(decompressor, &decompressed2_size, decompressed2, compressed2_size, compressed2);
  SQUASH_ASSERT_VALUE(res, SQUASH_OK);
  res = squash_stream_reset (decompressor);
  SQUASH_ASSERT_VALUE(res, SQUASH_OK);
  res = buffer_to_buffer_decompress_with_stream(decompressor, &decompressed1_size, decompressed1, compressed1_size, compressed1);
  SQUASH_ASSERT_VALUE(res, SQUASH_OK);

  g_free (compressed1);
  g_free (compressed2);
  g_free (decompressed1);
  g_free (decompressed2);

  squash_object_unref (compressor);
  squash_object_unref (decompressor);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/reset/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
