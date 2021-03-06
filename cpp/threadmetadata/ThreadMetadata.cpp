/**
 * Copyright 2004-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <sys/resource.h>
#include <cstring>

#include <profilo/LogEntry.h>
#include <profilo/logger/buffer/RingBuffer.h>
#include <profilo/threadmetadata/ThreadMetadata.h>
#include <profilo/util/ProcFsUtils.h>
#include <profilo/util/common.h>

namespace facebook {
namespace profilo {
namespace threadmetadata {

static int32_t logAnnotation(
    Logger& logger,
    EntryType type,
    const char* key,
    const char* value) {
  StandardEntry entry{};
  entry.tid = threadID();
  entry.timestamp = monotonicTime();
  entry.type = type;

  int32_t matchId = logger.write(std::move(entry));
  if (key != nullptr) {
    matchId = logger.writeBytes(
        EntryType::STRING_KEY,
        matchId,
        reinterpret_cast<const uint8_t*>(key),
        strlen(key));
  }
  return logger.writeBytes(
      EntryType::STRING_VALUE,
      matchId,
      reinterpret_cast<const uint8_t*>(value),
      strlen(value));
}

static void logThreadName(Logger& logger, uint32_t tid) {
  std::string threadName = util::getThreadName(tid);

  if (threadName.empty()) {
    return;
  }

  char threadId[16]{};
  snprintf(threadId, 16, "%d", tid);

  logAnnotation(
      logger, EntryType::TRACE_THREAD_NAME, threadId, threadName.data());
}

static void logThreadPriority(Logger& logger, int32_t tid) {
  errno = 0;
  int priority = getpriority(PRIO_PROCESS, tid);
  if (priority == -1 && errno != 0) {
    errno = 0;
    return; // Priority is not available
  }

  logger.write(StandardEntry{
      .id = 0,
      .type = EntryType::TRACE_THREAD_PRI,
      .timestamp = monotonicTime(),
      .tid = tid,
      .callid = 0,
      .matchid = 0,
      .extra = priority,
  });
}

/* Log thread names and priorities. */
void logThreadMetadata(fbjni::alias_ref<jobject>, JBuffer* buffer) {
  const auto& threads = util::threadListFromProcFs();
  auto buf = buffer->get();
  auto& logger = buf->logger();

  for (auto& tid : threads) {
    logThreadName(logger, tid);
    logThreadPriority(logger, tid);
  }
}

} // namespace threadmetadata
} // namespace profilo
} // namespace facebook
