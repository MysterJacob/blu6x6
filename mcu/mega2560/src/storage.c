// storage.c
#include "storage.h"

#include <avr/eeprom.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fail.h"
#include "obd.h"

extern obd_fault_cfg_t obd_fault_table[_OBD_FAULT_COUNT];

#define STORAGE_MAGIC 0xCAFEu
#define STORAGE_VERSION 0x0001u
#define MAX_PERSISTENT_FAULTS _OBD_FAULT_COUNT

typedef struct __attribute__((packed)) {
  uint16_t magic;
  uint16_t version;
  uint16_t persistent_count;
  uint16_t history_head;
  uint16_t history_count;
  uint16_t crc;
  obd_fault_nv_t persistent[MAX_PERSISTENT_FAULTS];
  obd_fault_nv_t history[MAX_HISTORY_FAULTS];
} nv_store_t;

static nv_store_t nv_cache;
static nv_store_t EEMEM nv_store_ee;

/* ── CRC ──────────────────────────────────────────────────────────────── */

static uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
  uint16_t crc = 0xFFFFu;
  for(size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for(uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                            : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

static uint16_t nv_crc(const nv_store_t *s)
{
  nv_store_t tmp = *s;
  tmp.crc = 0;
  return crc16_ccitt((const uint8_t *)&tmp, sizeof(tmp));
}

/* ── Persistence helpers ──────────────────────────────────────────────── */

static bool fault_is_persistent(uint16_t id)
{
  if(id >= _OBD_FAULT_COUNT) return false;
  return (obd_fault_table[id].flags & OBD_PERSISTENT) != 0;
}

static void nv_default(nv_store_t *s)
{
  memset(s, 0, sizeof(*s));
  s->magic = STORAGE_MAGIC;
  s->version = STORAGE_VERSION;
  s->crc = nv_crc(s);
}

static int nv_flush(void)
{
  nv_cache.crc = nv_crc(&nv_cache);
  eeprom_update_block((const void *)&nv_cache, (void *)&nv_store_ee,
                      sizeof(nv_cache));
  return 0;
}

/* ── History ring buffer ──────────────────────────────────────────────── */

static void history_append(uint16_t id, uint8_t status, uint8_t fault,
                           uint64_t last_bootcycle_present)
{
  obd_fault_nv_t rec = {id, status, fault, last_bootcycle_present};
  nv_cache.history[nv_cache.history_head] = rec;
  nv_cache.history_head =
      (uint16_t)((nv_cache.history_head + 1u) % MAX_HISTORY_FAULTS);
  if(nv_cache.history_count < MAX_HISTORY_FAULTS) nv_cache.history_count++;
}

/* ── Persistent fault table ───────────────────────────────────────────── */

static int persistent_find(uint16_t id)
{
  for(uint16_t i = 0; i < nv_cache.persistent_count; i++) {
    if(nv_cache.persistent[i].id == id) return (int)i;
  }
  return -1;
}

static void persistent_remove_idx(uint16_t idx)
{
  if(idx >= nv_cache.persistent_count) return;
  for(uint16_t i = idx; (uint16_t)(i + 1u) < nv_cache.persistent_count; i++)
    nv_cache.persistent[i] = nv_cache.persistent[i + 1u];
  nv_cache.persistent_count--;
}

static void persistent_upsert(uint16_t id, uint8_t status, uint8_t fault,
                              uint64_t last_bootcycle_present)
{
  int idx = persistent_find(id);
  obd_fault_nv_t rec = {id, status, fault, last_bootcycle_present};

  if((status & OBD_ACTIVE) == 0) {
    /* fault cleared — remove from persistent store */
    if(idx >= 0) persistent_remove_idx((uint16_t)idx);
    return;
  }

  if(idx >= 0) {
    nv_cache.persistent[idx] = rec;
    return;
  }

  if(nv_cache.persistent_count < MAX_PERSISTENT_FAULTS)
    nv_cache.persistent[nv_cache.persistent_count++] = rec;
}

/* ── Public API ───────────────────────────────────────────────────────── */

int storage_init(void)
{
  eeprom_read_block((void *)&nv_cache, (const void *)&nv_store_ee,
                    sizeof(nv_cache));

  if(nv_cache.magic != STORAGE_MAGIC || nv_cache.version != STORAGE_VERSION ||
     nv_cache.crc != nv_crc(&nv_cache) ||
     nv_cache.persistent_count > MAX_PERSISTENT_FAULTS ||
     nv_cache.history_count > MAX_HISTORY_FAULTS ||
     nv_cache.history_head >= MAX_HISTORY_FAULTS) {
    nv_default(&nv_cache);
    return nv_flush();
  }

  return 0;
}

int store_obd_fault(const uint16_t id, const uint8_t status,
                    const uint8_t fault, const uint64_t last_bootcycle_present)
{
  if(id >= _OBD_FAULT_COUNT) return -1;

  history_append(id, status, fault, last_bootcycle_present);

  if(fault_is_persistent(id))
    persistent_upsert(id, status, fault, last_bootcycle_present);

  return nv_flush();
}

int read_obd_faults(size_t buffer_size, size_t *active_count,
                    obd_fault_cfg_t faults[])
{
  size_t count = 0;

  for(uint16_t i = 0; i < nv_cache.persistent_count; i++) {
    const obd_fault_nv_t *src = &nv_cache.persistent[i];
    if(src->id >= buffer_size) continue;
    if(faults[src->id].id != src->id) continue;

    faults[src->id].status = src->status;
    faults[src->id].qualifier = src->qualifier;
    faults[src->id].last_bootcycle_present = src->last_bootcycle_present;
    count++;
  }

  if(active_count != NULL) *active_count = count;
  return 0;
}

int read_obd_faults_history(size_t buffer_size, size_t *count,
                            obd_fault_nv_t faults[])
{
  size_t n = 0;

  for(; n < buffer_size && n < nv_cache.history_count; n++) {
    uint16_t idx =
        (uint16_t)((nv_cache.history_head + MAX_HISTORY_FAULTS - 1u - n) %
                   MAX_HISTORY_FAULTS);
    faults[n] = nv_cache.history[idx];
  }

  if(count != NULL) *count = n;
  return 0;
}
