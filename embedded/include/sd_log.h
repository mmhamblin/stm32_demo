#ifndef SD_LOG_H
#define SD_LOG_H

/**
 * @file sd_log.h
 * @brief SD-card logging boundary for completed acquisition bursts.
 *
 * This is a portfolio skeleton for where STM32 SDMMC/FileX code would live.
 * The acquisition thread does not call this module directly; the storage
 * thread owns SD writes so SD latency cannot block the capture path.
 *
 * @requirement SRS-SD-001
 * @requirement SRS-RTOS-002
 */

#include <stdbool.h>
#include <stdint.h>

/** @brief SD logging counters for status reporting. */
typedef struct
{
    bool mounted;
    uint32_t records_written;
    uint32_t bytes_written;
    uint32_t write_errors;
} sd_log_status_t;

/**
 * @brief Initialize or mount the SD logging target.
 *
 * @return true when the SD log is ready for writes.
 *
 * @requirement SRS-SD-001
 */
bool SdLog_Init(void);

/**
 * @brief Append one completed acquisition burst to the SD log.
 *
 * @param sequence Record sequence number.
 * @param timestamp_ms Capture timestamp.
 * @param samples Pointer to 2046 12-bit sample words.
 * @param sample_count Number of sample words.
 * @return true when the record is written.
 *
 * @requirement SRS-SD-001
 */
bool SdLog_WriteBurstAt(uint32_t sequence,
                        uint32_t timestamp_ms,
                        const uint16_t *samples,
                        uint32_t sample_count);

/**
 * @brief Return the latest SD log status.
 *
 * @return SD logging status counters.
 *
 * @requirement SRS-GUI-001
 */
sd_log_status_t SdLog_GetStatus(void);

#endif
