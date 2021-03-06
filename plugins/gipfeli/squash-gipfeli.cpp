/* Copyright (c) 2013-2015 The Squash Authors
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *   Evan Nemerson <evan@nemerson.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>

#include <squash/squash.h>

#include "gipfeli/gipfeli.h"

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

static size_t
squash_gipfeli_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  size_t ret;
  util::compression::Compressor* compressor =
    util::compression::NewGipfeliCompressor();

  ret = compressor->MaxCompressedLength (uncompressed_size);

  delete compressor;

  return ret;
}

static size_t
squash_gipfeli_get_uncompressed_size (SquashCodec* codec,
                                      size_t compressed_size,
                                      const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)]) {
  util::compression::Compressor* compressor =
    util::compression::NewGipfeliCompressor();
  std::string compressed_str((const char*) compressed, compressed_size);

  size_t uncompressed_size = 0;
  bool success = compressor->GetUncompressedLength (compressed_str, &uncompressed_size);

  delete compressor;

  return success ? uncompressed_size : 0;
}

class CheckedByteArraySink : public util::compression::Sink {
 public:
  explicit CheckedByteArraySink(char* dest, size_t dest_size) : dest_(dest), remaining_(dest_size) {}
  virtual ~CheckedByteArraySink() {}
  virtual void Append(const char* data, size_t n) {
    if (n > remaining_) {
      throw std::length_error("buffer too small");
    }

    if (data != dest_) {
      memcpy(dest_, data, n);
    }

    dest_ += n;
    remaining_ -= n;
  }
  char* GetAppendBufferVariable(size_t min_size, size_t desired_size_hint,
                                char* scratch, size_t scratch_size,
                                size_t* allocated_size) {
    *allocated_size = desired_size_hint;
    return dest_;
  }

 private:
  char* dest_;
  size_t remaining_;
};

static SquashStatus
squash_gipfeli_decompress_buffer (SquashCodec* codec,
                                  size_t* decompressed_size,
                                  uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_size)],
                                  size_t compressed_size,
                                  const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_size)],
                                  SquashOptions* options) {
  util::compression::Compressor* compressor =
    util::compression::NewGipfeliCompressor();
  util::compression::UncheckedByteArraySink sink((char*) decompressed);
  util::compression::ByteArraySource source((const char*) compressed, compressed_size);
  SquashStatus res = SQUASH_OK;

  if (SQUASH_UNLIKELY(compressor == NULL))
    return squash_error (SQUASH_MEMORY);

  std::string compressed_str((const char*) compressed, compressed_size);
  size_t uncompressed_size;
  if (SQUASH_UNLIKELY(!compressor->GetUncompressedLength (compressed_str, &uncompressed_size))) {
    res = squash_error (SQUASH_FAILED);
    goto cleanup;
  }

  if (SQUASH_UNLIKELY(uncompressed_size > *decompressed_size)) {
    res = squash_error (SQUASH_BUFFER_FULL);
    goto cleanup;
  } else {
    *decompressed_size = uncompressed_size;
  }

  if (SQUASH_UNLIKELY(!compressor->UncompressStream (&source, &sink))) {
    res = squash_error (SQUASH_FAILED);
  }

 cleanup:

  delete compressor;

  return res;
}

static SquashStatus
squash_gipfeli_compress_buffer (SquashCodec* codec,
                                size_t* compressed_size,
                                uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                                size_t uncompressed_size,
                                const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                                SquashOptions* options) {
  util::compression::Compressor* compressor = util::compression::NewGipfeliCompressor();
  CheckedByteArraySink sink((char*) compressed, *compressed_size);
  util::compression::ByteArraySource source((const char*) uncompressed, uncompressed_size);
  SquashStatus res;

  try {
    *compressed_size = compressor->CompressStream (&source, &sink);
    res = SQUASH_OK;
  } catch (const std::bad_alloc& e) {
    res = squash_error (SQUASH_MEMORY);
  } catch (const std::length_error& e) {
    res = squash_error (SQUASH_BUFFER_FULL);
  } catch (...) {
    res = squash_error (SQUASH_FAILED);
  }

  delete compressor;

  if (SQUASH_UNLIKELY(res == SQUASH_OK && *compressed_size == 0))
    res = squash_error (SQUASH_FAILED);

  return res;
}

static SquashStatus
squash_gipfeli_compress_buffer_unsafe (SquashCodec* codec,
                                       size_t* compressed_size,
                                       uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_size)],
                                       size_t uncompressed_size,
                                       const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_size)],
                                       SquashOptions* options) {
  util::compression::Compressor* compressor = util::compression::NewGipfeliCompressor();
  util::compression::UncheckedByteArraySink sink((char*) compressed);
  util::compression::ByteArraySource source((const char*) uncompressed, uncompressed_size);
  SquashStatus res;

  try {
    *compressed_size = compressor->CompressStream (&source, &sink);
    res = SQUASH_OK;
  } catch (const std::bad_alloc& e) {
    res = squash_error (SQUASH_MEMORY);
  } catch (const std::length_error& e) {
    res = squash_error (SQUASH_BUFFER_FULL);
  } catch (...) {
    res = squash_error (SQUASH_FAILED);
  }

  delete compressor;

  if (SQUASH_UNLIKELY(res == SQUASH_OK && *compressed_size == 0))
    res = squash_error (SQUASH_FAILED);

  return res;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("gipfeli", name) == 0)) {
    impl->get_uncompressed_size = squash_gipfeli_get_uncompressed_size;
    impl->get_max_compressed_size = squash_gipfeli_get_max_compressed_size;
    impl->decompress_buffer = squash_gipfeli_decompress_buffer;
    impl->compress_buffer = squash_gipfeli_compress_buffer;
    impl->compress_buffer_unsafe = squash_gipfeli_compress_buffer_unsafe;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
