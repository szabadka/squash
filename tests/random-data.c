#include "test-codecs.h"

// When testing new codecs, it's a good idea to try larger values to
// verify the max compressed size calculation.  I usually set this to
// about 512 MiB.
#define MAX_UNCOMPRESSED_LENGTH ((size_t) (1024 * 1024 * 4))

void
check_codec (SquashCodec* codec) {
  size_t uncompressed_length;
  uint8_t* uncompressed_data = (uint8_t*) malloc (MAX_UNCOMPRESSED_LENGTH);
  size_t compressed_length;
  uint8_t* compressed_data = (uint8_t*) malloc (squash_codec_get_max_compressed_size (codec, MAX_UNCOMPRESSED_LENGTH));
  size_t decompressed_length;
  uint8_t* decompressed_data = (uint8_t*) malloc (MAX_UNCOMPRESSED_LENGTH);

  size_t uncompressed_data_filled = 0;
  SquashStatus res;

  memset (compressed_data, 0, squash_codec_get_max_compressed_size (codec, MAX_UNCOMPRESSED_LENGTH));

  for ( uncompressed_length = 1;
        uncompressed_length <= MAX_UNCOMPRESSED_LENGTH ;
        uncompressed_length += g_test_quick () ?
          (g_test_rand_int_range (256, 1024) * (2 + (uncompressed_length / 512))) :
          1) {
    for ( ; uncompressed_data_filled < uncompressed_length ; uncompressed_data_filled += sizeof(int) ) {
      gint r = g_test_rand_int ();
      memcpy (uncompressed_data + uncompressed_data_filled, &r, sizeof(int));
    }

    compressed_length = squash_codec_get_max_compressed_size (codec, uncompressed_length);
    g_assert (compressed_length > 0);

    res = squash_codec_compress (codec, &compressed_length, compressed_data, uncompressed_length, (uint8_t*) uncompressed_data, NULL);
    if (SQUASH_UNLIKELY(res != SQUASH_OK)) {
      size_t requested = squash_codec_get_max_compressed_size (codec, uncompressed_length);
      compressed_length *= 2;
      free (compressed_data);
      compressed_data = malloc (compressed_length);
      res = squash_codec_compress (codec, &compressed_length, compressed_data, uncompressed_length, (uint8_t*) uncompressed_data, NULL);
      if (res == SQUASH_OK) {
        g_error ("Failed for %zu bytes (requested %zu extra bytes for a total of %zu, needed %zu more)",
                 uncompressed_length,
                 requested - uncompressed_length,
                 requested,
                 compressed_length - requested);
      } else {
        g_error ("Failed for %zu bytes (requested %zu extra bytes for a total of %zu).  Doubling the allowed storage didn't help.",
                 uncompressed_length,
                 requested - uncompressed_length,
                 requested);
      }
    }
    g_assert_cmpint (compressed_length, >, 0);
    g_assert_cmpint (compressed_length, <=, squash_codec_get_max_compressed_size (codec, uncompressed_length));

    // Helpful when adding new codecs which don't document this…
    /* g_message ("%" G_GSIZE_FORMAT " -> %" G_GSIZE_FORMAT " (needs %" G_GSIZE_FORMAT ", requested %" G_GSIZE_FORMAT ")", */
    /*            uncompressed_length, */
    /*            compressed_length, */
    /*            compressed_length - uncompressed_length, */
    /*            squash_codec_get_max_compressed_size (codec, uncompressed_length) - uncompressed_length); */

    decompressed_length = uncompressed_length;
    res = squash_codec_decompress (codec, &decompressed_length, decompressed_data, compressed_length, compressed_data, NULL);
    SQUASH_ASSERT_OK(res);
    g_assert (decompressed_length == uncompressed_length);

    int memcmpres = memcmp (uncompressed_data, decompressed_data, uncompressed_length);
    if (memcmpres != 0) {
      size_t pos;
      for (pos = 0 ; pos < decompressed_length ; pos++ ) {
        if (uncompressed_data[pos] != decompressed_data[pos]) {
          fprintf (stderr, "failed at %" G_GSIZE_FORMAT " of %" G_GSIZE_FORMAT "\n", pos, decompressed_length);
          break;
        }
      }
    }
    g_assert (memcmp (uncompressed_data, decompressed_data, uncompressed_length) == 0);

    /* How about decompressing some random data?  Note that this may
       actually succeed, so don't check the response—we just want to
       make sure it doesn't crash. */
    compressed_length = uncompressed_length;
    memcpy (compressed_data, uncompressed_data, uncompressed_length);
    squash_codec_decompress (codec, &decompressed_length, decompressed_data, compressed_length, compressed_data, NULL);
  }

  free (uncompressed_data);
  free (compressed_data);
  free (decompressed_data);
}

void
squash_check_setup_tests_for_codec (SquashCodec* codec, void* user_data) {
  gchar* test_name = g_strdup_printf ("/random-data/%s/%s",
                                      squash_plugin_get_name (squash_codec_get_plugin (codec)),
                                      squash_codec_get_name (codec));
  g_test_add_data_func (test_name, codec, (GTestDataFunc) check_codec);
  g_free (test_name);
}
