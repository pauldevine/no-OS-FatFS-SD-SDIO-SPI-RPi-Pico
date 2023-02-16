/* FatFsSd.cpp
Copyright 2023 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

#include "FatFsSd.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

/*
    See FatFs - Generic FAT Filesystem Module, "Application Interface",
    http://elm-chan.org/fsw/ff/00index_e.html
 */

std::vector<FatFs_Spi> FatFs::Spis;
std::vector<FatFs_SdCard> FatFs::SdCards;

size_t __attribute__((weak))
FatFs::SdCard_get_num() {
    return SdCards.size();
}

FatFs_SdCard* __attribute__((weak))
FatFs::SdCard_get_by_num(size_t num) {
    if (num <= SdCard_get_num()) {
        return &SdCards[num];
    } else {
        return NULL;
    }
}

FatFs_SdCard* __attribute__((weak))
FatFs::SdCard_get_by_name(const char* const name) {
    for (size_t i = 0; i < SdCard_get_num(); ++i)
        if (0 == strcmp(SdCard_get_by_num(i)->get_name(), name))
            return SdCard_get_by_num(i);
    // printf("%s: unknown name %s\n", __func__, name);
    return NULL;
}

size_t __attribute__((weak))
FatFs::Spi_get_num() {
    return Spis.size();
}

FatFs_Spi* __attribute__((weak))
FatFs::Spi_get_by_num(size_t num) {
    if (num <= Spi_get_num()) {
        return &Spis[num];
    } else {
        return NULL;
    }
}

size_t __attribute__((weak)) spi_get_num() {
    return FatFs::Spi_get_num();
}
spi_t __attribute__((weak)) * spi_get_by_num(size_t num) {
    return &FatFs::Spi_get_by_num(num)->m_spi;
}
size_t __attribute__((weak)) sd_get_num() {
    return FatFs::SdCard_get_num();
}
sd_card_t __attribute__((weak)) * sd_get_by_num(size_t num) {
    return &(FatFs::SdCard_get_by_num(num)->m_sd_card);
}

/* Put a formatted string to the file */
int FatFs_File::printf(const TCHAR* format, ...) {
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer) {
            return 0;
        }
        va_start(arg, format);
        int vrc = vsnprintf(buffer, len + 1, format, arg);
        // Notice that only when this returned value is non-negative and less than n,
        //   the string has been completely written.
        assert(vrc >= 0 && vrc < len + 1);
        va_end(arg);
    }
    UINT bw;
    FRESULT fr = f_write(&fil, buffer, len, &bw);
    int rc = bw;
    if (FR_OK != fr) {
        rc = -1;
    }
    if (buffer != temp) {
        delete[] buffer;
    }
    return rc;
}

bool FatFs::begin() {
    if (!sd_init_driver())
        return false;
    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t* sd_card_p = sd_get_by_num(i);
        if (!sd_card_p) return false;
        // See http://elm-chan.org/fsw/ff/doc/dstat.html
        int dstatus = sd_card_p->init(sd_card_p);
        if (dstatus & STA_NOINIT) return false;
    }
    return true;
}