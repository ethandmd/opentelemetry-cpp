// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/nostd/variant.h"
#include "opentelemetry/sdk/metrics/aggregation/aggregation.h"
#include "opentelemetry/sdk/metrics/aggregation/aggregation_config.h"
#include "opentelemetry/sdk/metrics/aggregation/base2_exponential_histogram_aggregation.h"
#include "opentelemetry/sdk/metrics/aggregation/histogram_aggregation.h"
#include "opentelemetry/sdk/metrics/aggregation/lastvalue_aggregation.h"
#include "opentelemetry/sdk/metrics/aggregation/sum_aggregation.h"
#include "opentelemetry/sdk/metrics/data/point_data.h"

using namespace opentelemetry::sdk::metrics;
namespace nostd = opentelemetry::nostd;
TEST(Aggregation, LongSumAggregation)
{
  LongSumAggregation aggr(true);
  auto data = aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<SumPointData>(data));
  auto sum_data = nostd::get<SumPointData>(data);
  ASSERT_TRUE(nostd::holds_alternative<int64_t>(sum_data.value_));
  EXPECT_EQ(nostd::get<int64_t>(sum_data.value_), 0);
  aggr.Aggregate(static_cast<int64_t>(12), {});
  aggr.Aggregate(static_cast<int64_t>(0), {});
  sum_data = nostd::get<SumPointData>(aggr.ToPoint());
  EXPECT_EQ(nostd::get<int64_t>(sum_data.value_), 12);
}

TEST(Aggregation, DoubleSumAggregation)
{
  DoubleSumAggregation aggr(true);
  auto data = aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<SumPointData>(data));
  auto sum_data = nostd::get<SumPointData>(data);
  ASSERT_TRUE(nostd::holds_alternative<double>(sum_data.value_));
  EXPECT_EQ(nostd::get<double>(sum_data.value_), 0);
  aggr.Aggregate(12.0, {});
  aggr.Aggregate(1.0, {});
  sum_data = nostd::get<SumPointData>(aggr.ToPoint());
  EXPECT_EQ(nostd::get<double>(sum_data.value_), 13.0);
}

TEST(Aggregation, LongLastValueAggregation)
{
  LongLastValueAggregation aggr;
  auto data = aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<LastValuePointData>(data));
  auto lastvalue_data = nostd::get<LastValuePointData>(data);
  ASSERT_TRUE(nostd::holds_alternative<int64_t>(lastvalue_data.value_));
  EXPECT_EQ(lastvalue_data.is_lastvalue_valid_, false);
  aggr.Aggregate(static_cast<int64_t>(12), {});
  aggr.Aggregate(static_cast<int64_t>(1), {});
  lastvalue_data = nostd::get<LastValuePointData>(aggr.ToPoint());
  EXPECT_EQ(nostd::get<int64_t>(lastvalue_data.value_), 1.0);
}

TEST(Aggregation, DoubleLastValueAggregation)
{
  DoubleLastValueAggregation aggr;
  auto data = aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<LastValuePointData>(data));
  auto lastvalue_data = nostd::get<LastValuePointData>(data);
  ASSERT_TRUE(nostd::holds_alternative<double>(lastvalue_data.value_));
  EXPECT_EQ(lastvalue_data.is_lastvalue_valid_, false);
  aggr.Aggregate(12.0, {});
  aggr.Aggregate(1.0, {});
  lastvalue_data = nostd::get<LastValuePointData>(aggr.ToPoint());
  EXPECT_EQ(nostd::get<double>(lastvalue_data.value_), 1.0);
}

TEST(Aggregation, LongHistogramAggregation)
{
  LongHistogramAggregation aggr;
  auto data = aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<HistogramPointData>(data));
  auto histogram_data = nostd::get<HistogramPointData>(data);
  ASSERT_TRUE(nostd::holds_alternative<int64_t>(histogram_data.sum_));
  EXPECT_EQ(nostd::get<int64_t>(histogram_data.sum_), 0);
  EXPECT_EQ(histogram_data.count_, 0);
  aggr.Aggregate(static_cast<int64_t>(12), {});   // lies in third bucket
  aggr.Aggregate(static_cast<int64_t>(100), {});  // lies in sixth bucket
  histogram_data = nostd::get<HistogramPointData>(aggr.ToPoint());
  EXPECT_EQ(nostd::get<int64_t>(histogram_data.min_), 12);
  EXPECT_EQ(nostd::get<int64_t>(histogram_data.max_), 100);
  EXPECT_EQ(nostd::get<int64_t>(histogram_data.sum_), 112);
  EXPECT_EQ(histogram_data.count_, 2);
  EXPECT_EQ(histogram_data.counts_[3], 1);
  EXPECT_EQ(histogram_data.counts_[6], 1);
  aggr.Aggregate(static_cast<int64_t>(13), {});   // lies in third bucket
  aggr.Aggregate(static_cast<int64_t>(252), {});  // lies in eight bucket
  histogram_data = nostd::get<HistogramPointData>(aggr.ToPoint());
  EXPECT_EQ(histogram_data.count_, 4);
  EXPECT_EQ(histogram_data.counts_[3], 2);
  EXPECT_EQ(histogram_data.counts_[8], 1);
  EXPECT_EQ(nostd::get<int64_t>(histogram_data.min_), 12);
  EXPECT_EQ(nostd::get<int64_t>(histogram_data.max_), 252);

  // Merge
  LongHistogramAggregation aggr1;
  aggr1.Aggregate(static_cast<int64_t>(1), {});
  aggr1.Aggregate(static_cast<int64_t>(11), {});
  aggr1.Aggregate(static_cast<int64_t>(26), {});

  LongHistogramAggregation aggr2;
  aggr2.Aggregate(static_cast<int64_t>(2), {});
  aggr2.Aggregate(static_cast<int64_t>(3), {});
  aggr2.Aggregate(static_cast<int64_t>(13), {});
  aggr2.Aggregate(static_cast<int64_t>(28), {});
  aggr2.Aggregate(static_cast<int64_t>(105), {});

  auto aggr3     = aggr1.Merge(aggr2);
  histogram_data = nostd::get<HistogramPointData>(aggr3->ToPoint());

  EXPECT_EQ(histogram_data.count_, 8);      // 3 each from aggr1 and aggr2
  EXPECT_EQ(histogram_data.counts_[1], 3);  // 1, 2, 3
  EXPECT_EQ(histogram_data.counts_[3], 2);  // 11, 13
  EXPECT_EQ(histogram_data.counts_[4], 2);  // 25, 28
  EXPECT_EQ(histogram_data.counts_[7], 1);  // 105
  EXPECT_EQ(nostd::get<int64_t>(histogram_data.min_), 1);
  EXPECT_EQ(nostd::get<int64_t>(histogram_data.max_), 105);

  // Diff
  auto aggr4     = aggr1.Diff(aggr2);
  histogram_data = nostd::get<HistogramPointData>(aggr4->ToPoint());
  EXPECT_EQ(histogram_data.count_, 2);      // aggr2:5 - aggr1:3
  EXPECT_EQ(histogram_data.counts_[1], 1);  // aggr2(2, 3) - aggr1(1)
  EXPECT_EQ(histogram_data.counts_[3], 0);  // aggr2(13) - aggr1(11)
  EXPECT_EQ(histogram_data.counts_[4], 0);  // aggr2(28) - aggr1(25)
  EXPECT_EQ(histogram_data.counts_[7], 1);  // aggr2(105) - aggr1(0)
}

TEST(Aggregation, LongHistogramAggregationBoundaries)
{
  std::shared_ptr<opentelemetry::sdk::metrics::HistogramAggregationConfig> aggregation_config{
      new opentelemetry::sdk::metrics::HistogramAggregationConfig};
  std::vector<double> user_boundaries = {0.0,   50.0,   100.0,  250.0,  500.0,
                                         750.0, 1000.0, 2500.0, 5000.0, 10000.0};
  aggregation_config->boundaries_     = user_boundaries;
  LongHistogramAggregation aggr{aggregation_config.get()};
  auto data = aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<HistogramPointData>(data));
  auto histogram_data = nostd::get<HistogramPointData>(data);
  EXPECT_EQ(histogram_data.boundaries_, user_boundaries);
}

TEST(Aggregation, DoubleHistogramAggregationBoundaries)
{
  std::shared_ptr<opentelemetry::sdk::metrics::HistogramAggregationConfig> aggregation_config{
      new opentelemetry::sdk::metrics::HistogramAggregationConfig};
  std::vector<double> user_boundaries = {0.0,   50.0,   100.0,  250.0,  500.0,
                                         750.0, 1000.0, 2500.0, 5000.0, 10000.0};
  aggregation_config->boundaries_     = user_boundaries;
  DoubleHistogramAggregation aggr{aggregation_config.get()};
  auto data = aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<HistogramPointData>(data));
  auto histogram_data = nostd::get<HistogramPointData>(data);
  EXPECT_EQ(histogram_data.boundaries_, user_boundaries);
}

TEST(Aggregation, DoubleHistogramAggregation)
{
  DoubleHistogramAggregation aggr;
  auto data = aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<HistogramPointData>(data));
  auto histogram_data = nostd::get<HistogramPointData>(data);
  ASSERT_TRUE(nostd::holds_alternative<double>(histogram_data.sum_));
  EXPECT_EQ(nostd::get<double>(histogram_data.sum_), 0);
  EXPECT_EQ(histogram_data.count_, 0);
  aggr.Aggregate(12.0, {});   // lies in third bucket
  aggr.Aggregate(100.0, {});  // lies in sixth bucket
  histogram_data = nostd::get<HistogramPointData>(aggr.ToPoint());
  EXPECT_EQ(nostd::get<double>(histogram_data.sum_), 112);
  EXPECT_EQ(histogram_data.count_, 2);
  EXPECT_EQ(histogram_data.counts_[3], 1);
  EXPECT_EQ(histogram_data.counts_[6], 1);
  EXPECT_EQ(nostd::get<double>(histogram_data.min_), 12);
  EXPECT_EQ(nostd::get<double>(histogram_data.max_), 100);
  aggr.Aggregate(13.0, {});   // lies in third bucket
  aggr.Aggregate(252.0, {});  // lies in eight bucket
  histogram_data = nostd::get<HistogramPointData>(aggr.ToPoint());
  EXPECT_EQ(histogram_data.count_, 4);
  EXPECT_EQ(histogram_data.counts_[3], 2);
  EXPECT_EQ(histogram_data.counts_[8], 1);
  EXPECT_EQ(nostd::get<double>(histogram_data.sum_), 377);
  EXPECT_EQ(nostd::get<double>(histogram_data.min_), 12);
  EXPECT_EQ(nostd::get<double>(histogram_data.max_), 252);

  // Merge
  DoubleHistogramAggregation aggr1;
  aggr1.Aggregate(1.0, {});
  aggr1.Aggregate(11.0, {});
  aggr1.Aggregate(25.1, {});

  DoubleHistogramAggregation aggr2;
  aggr2.Aggregate(2.0, {});
  aggr2.Aggregate(3.0, {});
  aggr2.Aggregate(13.0, {});
  aggr2.Aggregate(28.1, {});
  aggr2.Aggregate(105.0, {});

  auto aggr3     = aggr1.Merge(aggr2);
  histogram_data = nostd::get<HistogramPointData>(aggr3->ToPoint());

  EXPECT_EQ(histogram_data.count_, 8);      // 3 each from aggr1 and aggr2
  EXPECT_EQ(histogram_data.counts_[1], 3);  // 1.0, 2.0, 3.0
  EXPECT_EQ(histogram_data.counts_[3], 2);  // 11.0, 13.0
  EXPECT_EQ(histogram_data.counts_[4], 2);  // 25.1, 28.1
  EXPECT_EQ(histogram_data.counts_[7], 1);  // 105.0
  EXPECT_EQ(nostd::get<double>(histogram_data.min_), 1);
  EXPECT_EQ(nostd::get<double>(histogram_data.max_), 105);

  // Diff
  auto aggr4     = aggr1.Diff(aggr2);
  histogram_data = nostd::get<HistogramPointData>(aggr4->ToPoint());
  EXPECT_EQ(histogram_data.count_, 2);      // aggr2:5 - aggr1:3
  EXPECT_EQ(histogram_data.counts_[1], 1);  // aggr2(2.0, 3.0) - aggr1(1.0)
  EXPECT_EQ(histogram_data.counts_[3], 0);  // aggr2(13.0) - aggr1(11.0)
  EXPECT_EQ(histogram_data.counts_[4], 0);  // aggr2(28.1) - aggr1(25.1)
  EXPECT_EQ(histogram_data.counts_[7], 1);  // aggr2(105.0) - aggr1(0)
}

TEST(aggregation, Base2ExponentialHistogramAggregation)
{
  // Low res histo
  auto SCALE0       = 0;
  auto MAX_BUCKETS0 = 7;
  Base2ExponentialHistogramAggregationConfig scale0_config;
  scale0_config.max_scale_      = SCALE0;
  scale0_config.max_buckets_    = MAX_BUCKETS0;
  scale0_config.record_min_max_ = true;
  Base2ExponentialHistogramAggregation scale0_aggr(&scale0_config);
  auto point = scale0_aggr.ToPoint();
  ASSERT_TRUE(nostd::holds_alternative<Base2ExponentialHistogramPointData>(point));
  auto histo_point = nostd::get<Base2ExponentialHistogramPointData>(point);
  EXPECT_EQ(histo_point.count_, 0);
  EXPECT_EQ(histo_point.sum_, 0.0);
  EXPECT_EQ(histo_point.zero_count_, 0);
  EXPECT_EQ(histo_point.min_, std::numeric_limits<double>::max());
  EXPECT_EQ(histo_point.max_, std::numeric_limits<double>::min());
  EXPECT_EQ(histo_point.scale_, SCALE0);
  EXPECT_EQ(histo_point.max_buckets_, MAX_BUCKETS0);
  ASSERT_TRUE(histo_point.positive_buckets_.Empty());

  // zero point
  scale0_aggr.Aggregate(0.0, {});
  histo_point = nostd::get<Base2ExponentialHistogramPointData>(scale0_aggr.ToPoint());
  EXPECT_EQ(histo_point.count_, 1);
  EXPECT_EQ(histo_point.zero_count_, 1);

  // Two recordings in the same bucket (bucket 1 at scale 0)
  scale0_aggr.Aggregate(3.0, {});
  scale0_aggr.Aggregate(3.5, {});
  histo_point = nostd::get<Base2ExponentialHistogramPointData>(scale0_aggr.ToPoint());
  EXPECT_EQ(histo_point.count_, 3);
  EXPECT_EQ(histo_point.sum_, 6.5);
  EXPECT_EQ(histo_point.min_, 0.0);
  EXPECT_EQ(histo_point.max_, 3.5);
  ASSERT_FALSE(histo_point.positive_buckets_.Empty());
  auto start_index = histo_point.positive_buckets_.StartIndex();
  auto end_index   = histo_point.positive_buckets_.EndIndex();
  EXPECT_EQ(start_index, 1);
  EXPECT_EQ(end_index, 1);
  EXPECT_EQ(histo_point.positive_buckets_.Get(start_index), 2);

  // Recording in a different bucket (bucket -2 at scale 0)
  scale0_aggr.Aggregate(-0.3, {});
  histo_point = nostd::get<Base2ExponentialHistogramPointData>(scale0_aggr.ToPoint());
  EXPECT_EQ(histo_point.count_, 4);
  EXPECT_EQ(histo_point.sum_, 6.2);
  EXPECT_EQ(histo_point.min_, -0.3);
  EXPECT_EQ(histo_point.max_, 3.5);
  EXPECT_EQ(histo_point.negative_buckets_.Get(-2), 1);
  EXPECT_EQ(histo_point.positive_buckets_.Get(1), 2);

  Base2ExponentialHistogramAggregationConfig scale1_config;
  scale1_config.max_scale_      = 1;
  scale1_config.max_buckets_    = 14;
  scale1_config.record_min_max_ = true;
  Base2ExponentialHistogramAggregation scale1_aggr(&scale1_config);

  scale1_aggr.Aggregate(0.0, {});
  scale1_aggr.Aggregate(3.0, {});
  scale1_aggr.Aggregate(3.5, {});
  scale1_aggr.Aggregate(0.3, {});
  auto scale1_point = nostd::get<Base2ExponentialHistogramPointData>(scale1_aggr.ToPoint());
  EXPECT_EQ(scale1_point.count_, 4);
  EXPECT_EQ(scale1_point.sum_, 6.8);
  EXPECT_EQ(scale1_point.zero_count_, 1);
  EXPECT_EQ(scale1_point.min_, 0.0);
  EXPECT_EQ(scale1_point.max_, 3.5);

  auto merged       = scale0_aggr.Merge(scale1_aggr);
  auto merged_point = nostd::get<Base2ExponentialHistogramPointData>(merged->ToPoint());
  EXPECT_EQ(merged_point.count_, 8);
  EXPECT_EQ(merged_point.sum_, 13.0);
  EXPECT_EQ(merged_point.zero_count_, 2);
  EXPECT_EQ(merged_point.min_, -0.3);
  EXPECT_EQ(merged_point.max_, 3.5);
  EXPECT_EQ(merged_point.scale_, 0);
  EXPECT_EQ(merged_point.positive_buckets_.Get(1), 4);
  EXPECT_EQ(merged_point.negative_buckets_.Get(-2), 1);
  EXPECT_EQ(merged_point.positive_buckets_.Get(2), 0);

  auto diffd       = merged->Diff(scale1_aggr);
  auto diffd_point = nostd::get<Base2ExponentialHistogramPointData>(diffd->ToPoint());
  EXPECT_EQ(diffd_point.count_, 4);
  EXPECT_EQ(diffd_point.sum_, 6.2);
  EXPECT_EQ(diffd_point.zero_count_, 1);
  EXPECT_EQ(diffd_point.scale_, 0);
  EXPECT_EQ(diffd_point.positive_buckets_.Get(1), 2);
  EXPECT_EQ(diffd_point.negative_buckets_.Get(-2), 1);
  EXPECT_EQ(diffd_point.positive_buckets_.Get(2), 0);
}