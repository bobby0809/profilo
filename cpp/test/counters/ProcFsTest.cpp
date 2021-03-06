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

#include <folly/experimental/TestUtil.h>
#include <gtest/gtest.h>

#include <fstream>

#include <profilo/counters/ProcFs.h>
#include <profilo/util/common.h>

namespace fs = boost::filesystem;
namespace test = folly::test;

namespace facebook {
namespace profilo {
namespace counters {

constexpr auto ALL_STATS_MASK = 0xffffffff;

constexpr char STAT_CONTENT[] =
    "4653 (ebook.wakizashi) R 671 670 0 0 -1 4211008 22794 0 553 0 104 32 0 0 12 -8 32 0 147144270 2242772992 46062 18446744073709551615 2909851648 2909870673 4288193888 4288148192 4077764852 0 4612 0 1098945784 0 0 0 17 2 0 0 0 0 0 2909875376 2909876224 2913968128 4288195386 4288195495 4288195495 4288196574 0";
constexpr char SCHEDSTAT_CONTENT[] = "2075550186 1196266356 3934";
constexpr char SCHED_CONTENT_CUT[] =
    "procfs_sched (29376, #threads: 1) \n"
    "-------------------------------------------------------------------\n"
    "se.exec_start                                :     115728485.820777\n"
    "se.vruntime                                  :      52304885.848107\n"
    "se.sum_exec_runtime                          :            43.046463\n"
    "se.statistics.iowait_sum                     :            21.256929\n"
    "se.statistics.iowait_count                   :                   14\n"
    "avg_per_cpu                                  :            21.523231\n"
    "nr_switches                                  :                   60\n"
    "nr_voluntary_switches                        :            23";
constexpr char SCHED_CONTENT[] =
    "procfs_sched (29376, #threads: 1) \n"
    "-------------------------------------------------------------------\n"
    "se.exec_start                                :     115728485.820777\n"
    "se.vruntime                                  :      52304885.848107\n"
    "se.sum_exec_runtime                          :            43.046463\n"
    "se.statistics.iowait_sum                     :            21.256929\n"
    "se.statistics.iowait_count                   :                   14\n"
    "avg_per_cpu                                  :            21.523231\n"
    "nr_switches                                  :                   60\n"
    "nr_voluntary_switches                        :   572848514115728485\n"
    "nr_involuntary_switches                      :                   46\n"
    "prio                                         :                  120\n"
    "clock-delta                                  :                  573\n";
constexpr char SCHED_CONTENT_2[] =
    "sh (29240, #threads: 1) \n"
    "---------------------------------------------------------\n"
    "se.exec_start                      :      57790837.521475\n"
    "se.statistics.wait_sum             :             4.286404\n"
    "se.statistics.wait_count           :                   59\n"
    "se.statistics.iowait_sum           :             3.000000\n"
    "se.statistics.iowait_count         :                   14\n"
    "se.statistics.nr_failed_migrations_affine:                    0\n"
    "se.statistics.nr_failed_migrations_running:                    4\n"
    "se.statistics.nr_failed_migrations_hot:                    5\n"
    "se.statistics.nr_forced_migrations :                    0\n"
    "se.statistics.nr_wakeups_affine_attempts:                   32\n"
    "se.statistics.nr_wakeups_passive   :                    0\n"
    "nr_voluntary_switches              :                   38\n"
    "nr_involuntary_switches            :                   19\n";
constexpr char VMSTAT_CONTENT[] =
    "nr_free_pages 123\n"
    "nr_dirty 74317\n"
    "nr_mapped 76672\n"
    "nr_file_pages 235292\n"
    "nr_writeback 17\n"
    "nr_unstable 0\n"
    "nr_bounce 0\n"
    "pgpgin 862085\n"
    "pgpgout 133892\n"
    "pgmajfault 109\n"
    "pgfree 978797\n"
    "kswapd_steal 101\n"
    "kswapd_low_wmark_hit_quickly 0\n"
    "kswapd_high_wmark_hit_quickly 0\n"
    "pageoutrun 12\n"
    "allocstall 3\n";
constexpr char VMSTAT_CONTENT_MODIFIED[] =
    "nr_free_pages 123\n"
    "nr_dirty 74317\n"
    "nr_mapped 76672\n"
    "nr_file_pages 235292\n"
    "nr_writeback 1\n"
    "nr_unstable 0\n"
    "nr_bounce 0\n"
    "pgpgin 1857940636\n"
    "pgpgout 133892\n"
    "pgmajfault 109\n"
    "pgfree 978797\n"
    "kswapd_steal 101\n"
    "kswapd_low_wmark_hit_quickly 0\n"
    "kswapd_high_wmark_hit_quickly 0\n"
    "pageoutrun 12\n"
    "allocstall 32\n";
constexpr char VMSTAT_CONTENT_WITH_SPLIT_KSWAPD[] =
    "nr_free_pages 123\n"
    "pgsteal_kswapd_dma 236448262\n"
    "pgsteal_kswapd_normal 228999324\n"
    "pgsteal_kswapd_movable 1\n"
    "pageoutrun 12\n"
    "allocstall 3\n";
constexpr char STATM_CONTENT[] = "458494 18445 11398 6 0 38020 0";
constexpr char MEMINFO_CONTENT[] =
    "MemTotal:       32521744 kB\n"
    "MemFree:         6883612 kB\n"
    "MemAvailable:   21235196 kB\n"
    "Buffers:         4518588 kB\n"
    "Cached:          9887488 kB\n"
    "SwapCached:       402120 kB\n"
    "Active:         16627892 kB\n"
    "Inactive:        5855820 kB\n"
    "Active(anon):    7427148 kB\n"
    "Inactive(anon):  2410516 kB\n"
    "Active(file):    9200744 kB\n"
    "Inactive(file):  3445304 kB\n"
    "Unevictable:      303980 kB\n"
    "Mlocked:           42472 kB\n"
    "SwapTotal:      49061884 kB\n"
    "SwapFree:       48631292 kB\n"
    "Dirty:              3568 kB\n"
    "Writeback:            12 kB\n"
    "AnonPages:       7980164 kB\n"
    "Mapped:          1396028 kB\n"
    "Shmem:           1813380 kB\n"
    "KReclaimable:    2174312 kB\n";

class ProcFsTest : public ::testing::Test {
 protected:
  ProcFsTest() : ::testing::Test(), temp_stat_file_("test_stat") {}

  fs::path SetUpTempFile(char const* stat_content) {
    std::ofstream statFileStream(temp_stat_file_.path().c_str());
    statFileStream << stat_content;
    return temp_stat_file_.path();
  }

  test::TemporaryFile temp_stat_file_;
};

TEST_F(ProcFsTest, testPartialSchedFile) {
  fs::path statPath = SetUpTempFile(SCHED_CONTENT_CUT);
  TaskSchedFile schedFile{statPath.native()};
  SchedInfo schedInfo = schedFile.refresh(ALL_STATS_MASK);

  EXPECT_EQ(schedInfo.iowaitCount, 14);
  EXPECT_EQ(schedInfo.iowaitSum, 21);
  EXPECT_EQ(schedInfo.nrVoluntarySwitches, 0);
  EXPECT_EQ(schedInfo.nrInvoluntarySwitches, 0);
}

TEST_F(ProcFsTest, testSchedFile) {
  fs::path statPath = SetUpTempFile(SCHED_CONTENT);
  TaskSchedFile schedFile{statPath.native()};
  SchedInfo schedInfo = schedFile.refresh(ALL_STATS_MASK);

  EXPECT_EQ(schedInfo.iowaitCount, 14);
  EXPECT_EQ(schedInfo.iowaitSum, 21);
  EXPECT_EQ(schedInfo.nrVoluntarySwitches, 572848514115728485);
  EXPECT_EQ(schedInfo.nrInvoluntarySwitches, 46);
}

TEST_F(ProcFsTest, testSchedFileWithLongKeys) {
  fs::path statPath = SetUpTempFile(SCHED_CONTENT_2);
  TaskSchedFile schedFile{statPath.native()};
  SchedInfo schedInfo = schedFile.refresh(ALL_STATS_MASK);

  EXPECT_EQ(schedInfo.iowaitCount, 14);
  EXPECT_EQ(schedInfo.iowaitSum, 3);
  EXPECT_EQ(schedInfo.nrVoluntarySwitches, 38);
  EXPECT_EQ(schedInfo.nrInvoluntarySwitches, 19);
}

TEST_F(ProcFsTest, testSchedStatFile) {
  fs::path statPath = SetUpTempFile(SCHEDSTAT_CONTENT);
  TaskSchedstatFile schedStatFile{statPath.native()};
  SchedstatInfo statInfo = schedStatFile.refresh(ALL_STATS_MASK);

  EXPECT_EQ(statInfo.cpuTimeMs, 2075);
  EXPECT_EQ(statInfo.waitToRunTimeMs, 1196);
}

TEST_F(ProcFsTest, testStatFile) {
  fs::path statPath = SetUpTempFile(STAT_CONTENT);
  TaskStatFile statFile{statPath.native()};
  TaskStatInfo statInfo = statFile.refresh(ALL_STATS_MASK);

  static const auto CLK_TICK = systemClockTickIntervalMs();

  EXPECT_EQ(statInfo.state, TS_RUNNING);
  EXPECT_EQ(statInfo.minorFaults, 22794);
  EXPECT_EQ(statInfo.majorFaults, 553);
  EXPECT_EQ(statInfo.kernelCpuTimeMs, 32 * CLK_TICK);
  EXPECT_EQ(statInfo.cpuTime, 136 * CLK_TICK);
  EXPECT_EQ(statInfo.cpuNum, 2);
  EXPECT_EQ(statInfo.threadPriority, 12);
}

TEST_F(ProcFsTest, testVmStatFile) {
  fs::path statPath = SetUpTempFile(VMSTAT_CONTENT);
  VmStatFile statFile{statPath.native()};
  VmStatInfo statInfo = statFile.refresh(ALL_STATS_MASK);

  EXPECT_EQ(statInfo.pgPgIn, 862085);
  EXPECT_EQ(statInfo.pgPgOut, 133892);
  EXPECT_EQ(statInfo.pgMajFault, 109);
  EXPECT_EQ(statInfo.allocStall, 3);
  EXPECT_EQ(statInfo.pageOutrun, 12);
  EXPECT_EQ(statInfo.kswapdSteal, 101);

  SetUpTempFile(VMSTAT_CONTENT_MODIFIED);
  statInfo = statFile.refresh(ALL_STATS_MASK);
  EXPECT_EQ(statInfo.pgPgIn, 1857940636);
  EXPECT_EQ(statInfo.pgPgOut, 133892);
  EXPECT_EQ(statInfo.pgMajFault, 109);
  EXPECT_EQ(statInfo.allocStall, 32);
  EXPECT_EQ(statInfo.pageOutrun, 12);
  EXPECT_EQ(statInfo.kswapdSteal, 101);
}

TEST_F(ProcFsTest, testVmStatFileWithSplitKswapd) {
  fs::path statPath = SetUpTempFile(VMSTAT_CONTENT_WITH_SPLIT_KSWAPD);
  VmStatFile statFile{statPath.native()};
  VmStatInfo statInfo = statFile.refresh(ALL_STATS_MASK);

  EXPECT_EQ(statInfo.allocStall, 3);
  EXPECT_EQ(statInfo.pageOutrun, 12);
  EXPECT_EQ(statInfo.kswapdSteal, 465447587);
}

TEST_F(ProcFsTest, testStatmFile) {
  fs::path statPath = SetUpTempFile(STATM_CONTENT);
  ProcStatmFile statFile{statPath.native()};
  StatmInfo statInfo = statFile.refresh(ALL_STATS_MASK);

  EXPECT_EQ(statInfo.resident, 18445);
  EXPECT_EQ(statInfo.shared, 11398);
}

TEST_F(ProcFsTest, testMeminfoFile) {
  fs::path statPath = SetUpTempFile(MEMINFO_CONTENT);
  MeminfoFile statFile{statPath.native()};
  MeminfoInfo statInfo = statFile.refresh(ALL_STATS_MASK);

  EXPECT_EQ(statInfo.freeKB, 6883612);
  EXPECT_EQ(statInfo.dirtyKB, 3568);
  EXPECT_EQ(statInfo.writebackKB, 12);
  EXPECT_EQ(statInfo.cachedKB, 9887488);
  EXPECT_EQ(statInfo.activeKB, 16627892);
  EXPECT_EQ(statInfo.inactiveKB, 5855820);
}

} // namespace counters
} // namespace profilo
} // namespace facebook
