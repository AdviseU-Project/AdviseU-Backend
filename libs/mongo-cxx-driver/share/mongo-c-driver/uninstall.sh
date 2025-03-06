#!/usr/bin/env bash
#
# MongoDB C Driver uninstall program, generated with CMake
#
# Copyright 2009-present MongoDB, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License")
#
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -eu

__rmfile() {
    set -eu
    abs=$__prefix/$1
    printf "Remove file %s: " "$abs"
    if test -f "$abs" || test -L "$abs"
    then
        rm -- "$abs"
        echo "ok"
    else
        echo "skipped: not present"
    fi
}

__rmdir() {
    set -eu
    abs=$__prefix/$1
    printf "Remove directory %s: " "$abs"
    if test -d "$abs"
    then
        list=$(ls --almost-all "$abs")
        if test "$list" = ""
        then
            rmdir -- "$abs"
            echo "ok"
        else
            echo "skipped: not empty"
        fi
    else
        echo "skipped: not present"
    fi
}

__prefix=${DESTDIR:-}/home/kaiblack/source/repos/mongo-cxx-driver/install

__rmfile "lib/libbson-static-1.0.a"
__rmfile "lib/cmake/bson-1.0/bson_static-targets.cmake"
__rmfile "lib/pkgconfig/libbson-static-1.0.pc"
__rmfile "lib/libbson-1.0.so.0.0.0"
__rmfile "lib/libbson-1.0.so.0"
__rmfile "lib/libbson-1.0.so"
__rmfile "lib/cmake/bson-1.0/bson_shared-targets.cmake"
__rmfile "lib/pkgconfig/libbson-1.0.pc"
__rmfile "include/libbson-1.0/bson/bson-reader.h"
__rmfile "include/libbson-1.0/bson/bson-endian.h"
__rmfile "include/libbson-1.0/bson/bson-macros.h"
__rmfile "include/libbson-1.0/bson/bson-md5.h"
__rmfile "include/libbson-1.0/bson/bson-error.h"
__rmfile "include/libbson-1.0/bson/bson-writer.h"
__rmfile "include/libbson-1.0/bson/bson-compat.h"
__rmfile "include/libbson-1.0/bson/bson-version-functions.h"
__rmfile "include/libbson-1.0/bson/bson-types.h"
__rmfile "include/libbson-1.0/bson/bson-keys.h"
__rmfile "include/libbson-1.0/bson/bson.h"
__rmfile "include/libbson-1.0/bson/bson-json.h"
__rmfile "include/libbson-1.0/bson/bson-utf8.h"
__rmfile "include/libbson-1.0/bson/bson-iter.h"
__rmfile "include/libbson-1.0/bson/bson-decimal128.h"
__rmfile "include/libbson-1.0/bson/bson-oid.h"
__rmfile "include/libbson-1.0/bson/bson-atomic.h"
__rmfile "include/libbson-1.0/bson/bcon.h"
__rmfile "include/libbson-1.0/bson/bson-clock.h"
__rmfile "include/libbson-1.0/bson/bson-memory.h"
__rmfile "include/libbson-1.0/bson/bson-prelude.h"
__rmfile "include/libbson-1.0/bson/bson-cmp.h"
__rmfile "include/libbson-1.0/bson/bson-string.h"
__rmfile "include/libbson-1.0/bson/bson-context.h"
__rmfile "include/libbson-1.0/bson/bson-value.h"
__rmfile "include/libbson-1.0/bson/bson-version.h"
__rmfile "include/libbson-1.0/bson/bson-config.h"
__rmfile "include/libbson-1.0/bson.h"
__rmfile "lib/cmake/bson-1.0/bson-1.0-config.cmake"
__rmfile "lib/cmake/bson-1.0/bson-1.0-config-version.cmake"
__rmfile "lib/cmake/bson-1.0/bson-targets.cmake"
__rmfile "lib/cmake/libbson-1.0/libbson-1.0-config.cmake"
__rmfile "lib/cmake/libbson-1.0/libbson-1.0-config-version.cmake"
__rmfile "lib/cmake/libbson-static-1.0/libbson-static-1.0-config.cmake"
__rmfile "lib/cmake/libbson-static-1.0/libbson-static-1.0-config-version.cmake"
__rmfile "lib/cmake/mongoc-1.0/3rdParty/FindSASL2.cmake"
__rmfile "lib/pkgconfig/libmongoc-1.0.pc"
__rmfile "bin/mongoc-stat"
__rmfile "lib/pkgconfig/libmongoc-static-1.0.pc"
__rmfile "lib/libmongoc-static-1.0.a"
__rmfile "lib/libmongoc-1.0.so.0.0.0"
__rmfile "lib/libmongoc-1.0.so.0"
__rmfile "lib/libmongoc-1.0.so"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-config.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-version.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-apm.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-bulk-operation.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-bulkwrite.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-change-stream.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-client.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-client-pool.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-client-side-encryption.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-collection.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-cursor.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-database.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-error.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-flags.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-find-and-modify.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-gridfs.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-gridfs-bucket.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-gridfs-file.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-gridfs-file-page.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-gridfs-file-list.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-handshake.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-host-list.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-init.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-index.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-iovec.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-log.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-macros.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-matcher.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-opcode.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-optional.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-prelude.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-read-concern.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-read-prefs.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-server-api.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-server-description.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-client-session.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-sleep.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-socket.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-stream-tls-libressl.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-stream-tls-openssl.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-stream.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-stream-buffered.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-stream-file.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-stream-gridfs.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-stream-socket.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-structured-log.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-topology-description.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-uri.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-version-functions.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-write-concern.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-rand.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-stream-tls.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-ssl.h"
__rmfile "include/libmongoc-1.0/mongoc/mongoc-bulkwrite.h"
__rmfile "include/libmongoc-1.0/mongoc.h"
__rmfile "lib/pkgconfig/libmongoc-ssl-1.0.pc"
__rmfile "lib/cmake/mongoc-1.0/mongoc-targets.cmake"
__rmfile "lib/cmake/mongoc-1.0/mongoc-1.0-config.cmake"
__rmfile "lib/cmake/mongoc-1.0/mongoc-1.0-config-version.cmake"
__rmfile "lib/cmake/libmongoc-1.0/libmongoc-1.0-config.cmake"
__rmfile "lib/cmake/libmongoc-1.0/libmongoc-1.0-config-version.cmake"
__rmfile "lib/cmake/libmongoc-static-1.0/libmongoc-static-1.0-config.cmake"
__rmfile "lib/cmake/libmongoc-static-1.0/libmongoc-static-1.0-config-version.cmake"
__rmfile "share/mongo-c-driver/COPYING"
__rmfile "share/mongo-c-driver/NEWS"
__rmfile "share/mongo-c-driver/README.rst"
__rmfile "share/mongo-c-driver/THIRD_PARTY_NOTICES"
__rmfile "share/mongo-c-driver/uninstall.sh"
__rmdir "share/mongo-c-driver"
__rmdir "lib/pkgconfig"
__rmdir "lib/cmake/mongoc-1.0/3rdParty"
__rmdir "lib/cmake/mongoc-1.0"
__rmdir "lib/cmake/libmongoc-static-1.0"
__rmdir "lib/cmake/libmongoc-1.0"
__rmdir "lib/cmake/libbson-static-1.0"
__rmdir "lib/cmake/libbson-1.0"
__rmdir "lib/cmake/bson-1.0"
__rmdir "lib/cmake"
__rmdir "include/libmongoc-1.0/mongoc"
__rmdir "include/libmongoc-1.0"
__rmdir "include/libbson-1.0/bson"
__rmdir "include/libbson-1.0"
