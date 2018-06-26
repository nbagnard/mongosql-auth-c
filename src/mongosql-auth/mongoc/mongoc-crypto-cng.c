/* Copyright 2016 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_CRYPTO_CNG

#include "mongoc-crypto-private.h"
#include "mongoc-crypto-cng-private.h"

#include <windows.h>
#include <bcrypt.h>

#define NT_SUCCESS(Status) (((NTSTATUS) (Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS) 0xC0000001L)

static BCRYPT_ALG_HANDLE _sha1_hash_algo;
static BCRYPT_ALG_HANDLE _sha1_hmac_algo;
static BCRYPT_ALG_HANDLE _sha256_hash_algo;
static BCRYPT_ALG_HANDLE _sha256_hmac_algo;


void
mongoc_crypto_cng_init (void)
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   _sha1_hash_algo = 0;
   status = BCryptOpenAlgorithmProvider (
      &_sha1_hash_algo, BCRYPT_SHA1_ALGORITHM, NULL, 0);
   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptOpenAlgorithmProvider(SHA1): %x", status);
   }

   _sha1_hmac_algo = 0;
   status = BCryptOpenAlgorithmProvider (&_sha1_hmac_algo,
                                         BCRYPT_SHA1_ALGORITHM,
                                         NULL,
                                         BCRYPT_ALG_HANDLE_HMAC_FLAG);
   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptOpenAlgorithmProvider(SHA1 HMAC): %x", status);
   }

   _sha256_hash_algo = 0;
   status = BCryptOpenAlgorithmProvider (
      &_sha256_hash_algo, BCRYPT_SHA256_ALGORITHM, NULL, 0);
   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptOpenAlgorithmProvider(SHA256): %x", status);
   }

   _sha256_hmac_algo = 0;
   status = BCryptOpenAlgorithmProvider (&_sha256_hmac_algo,
                                         BCRYPT_SHA256_ALGORITHM,
                                         NULL,
                                         BCRYPT_ALG_HANDLE_HMAC_FLAG);
   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptOpenAlgorithmProvider(SHA256 HMAC): %x", status);
   }
}

void
mongoc_crypto_cng_cleanup (void)
{
   if (_sha1_hash_algo) {
      BCryptCloseAlgorithmProvider (&_sha1_hash_algo, 0);
   }
   if (_sha1_hmac_algo) {
      BCryptCloseAlgorithmProvider (&_sha1_hmac_algo, 0);
   }
   if (_sha256_hash_algo) {
      BCryptCloseAlgorithmProvider (&_sha256_hash_algo, 0);
   }
   if (_sha256_hash_algo) {
      BCryptCloseAlgorithmProvider (&_sha256_hash_algo, 0);
   }
}

my_bool
_mongoc_crypto_cng_hmac_or_hash (BCRYPT_ALG_HANDLE algorithm,
                                 void *key,
                                 size_t key_length,
                                 void *data,
                                 size_t data_length,
                                 void *hmac_out)
{
   char *hash_object_buffer = 0;
   ULONG hash_object_length = 0;
   BCRYPT_HASH_HANDLE hash = 0;
   ULONG mac_length = 0;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   my_bool retval = FALSE;
   ULONG noop = 0;

   status = BCryptGetProperty (algorithm,
                               BCRYPT_OBJECT_LENGTH,
                               (char *) &hash_object_length,
                               sizeof hash_object_length,
                               &noop,
                               0);

   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptGetProperty(): OBJECT_LENGTH %x", status);
      return FALSE;
   }

   status = BCryptGetProperty (algorithm,
                               BCRYPT_HASH_LENGTH,
                               (char *) &mac_length,
                               sizeof mac_length,
                               &noop,
                               0);

   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptGetProperty(): HASH_LENGTH %x", status);
      return FALSE;
   }

   hash_object_buffer = malloc (hash_object_length);

   status = BCryptCreateHash (algorithm,
                              &hash,
                              hash_object_buffer,
                              hash_object_length,
                              key,
                              (ULONG) key_length,
                              0);

   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptCreateHash(): %x", status);
      goto cleanup;
   }

   status = BCryptHashData (hash, data, (ULONG) data_length, 0);
   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptHashData(): %x", status);
      goto cleanup;
   }

   status = BCryptFinishHash (hash, hmac_out, mac_length, 0);
   if (!NT_SUCCESS (status)) {
      MONGOC_LOG ("BCryptFinishHash(): %x", status);
      goto cleanup;
   }

   retval = TRUE;

cleanup:
   if (hash) {
      (void) BCryptDestroyHash (hash);
   }

   free (hash_object_buffer);
   return retval;
}


void
mongoc_crypto_cng_hmac_sha1 (mongoc_crypto_t *crypto,
                             const void *key,
                             int key_len,
                             const unsigned char *data,
                             int data_len,
                             unsigned char *hmac_out)
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;

   if (!_sha1_hmac_algo) {
      return;
   }

   _mongoc_crypto_cng_hmac_or_hash (
      _sha1_hmac_algo, key, key_len, data, data_len, hmac_out);
}


my_bool
mongoc_crypto_cng_sha1 (mongoc_crypto_t *crypto,
                        const unsigned char *input,
                        const size_t input_len,
                        unsigned char *hash_out)
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   my_bool res;

   if (!_sha1_hash_algo) {
      return FALSE;
   }

   return _mongoc_crypto_cng_hmac_or_hash (
      _sha1_hash_algo, NULL, 0, input, input_len, hash_out);
}


void
mongoc_crypto_cng_hmac_sha256 (mongoc_crypto_t *crypto,
                               const void *key,
                               int key_len,
                               const unsigned char *data,
                               int data_len,
                               unsigned char *hmac_out)
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;

   if (!_sha256_hmac_algo) {
      return;
   }

   _mongoc_crypto_cng_hmac_or_hash (
      _sha256_hmac_algo, key, key_len, data, data_len, hmac_out);
}

my_bool
mongoc_crypto_cng_sha256 (mongoc_crypto_t *crypto,
                          const unsigned char *input,
                          const size_t input_len,
                          unsigned char *hash_out)
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   my_bool res;

   if (!_sha256_hash_algo) {
      return FALSE;
   }

   res = _mongoc_crypto_cng_hmac_or_hash (
      _sha256_hash_algo, NULL, 0, input, input_len, hash_out);
   return res;
}
#endif
