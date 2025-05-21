// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/sdk/metrics/state/filtered_ordered_attribute_map.h"
#include "opentelemetry/nostd/function_ref.h"
#include "opentelemetry/sdk/common/attribute_utils.h"
#include "opentelemetry/sdk/metrics/view/attributes_processor.h"
#include <cassert>
#include "opentelemetry/sdk/common/global_log_handler.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace sdk
{
namespace metrics
{
FilteredOrderedAttributeMap::FilteredOrderedAttributeMap(
    const opentelemetry::common::KeyValueIterable &attributes,
    const AttributesProcessor *processor)
    : OrderedAttributeMap()
{
  OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap::FilteredOrderedAttributeMap");
  OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap attributes @" 
                         << static_cast<const void *>(&attributes));
  if (!processor)
  {
    OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap::FilteredOrderedAttributeMap processor is null");
  }
  attributes.ForEachKeyValue(
      [&](nostd::string_view key, opentelemetry::common::AttributeValue value) noexcept {
        OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap::FilteredOrderedAttributeMap key: " 
                               << key);
        if (!key.data())
        {
          OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap::FilteredOrderedAttributeMap key is null");
        }
        OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap::FilteredOrderedAttributeMap isPresent check...");
        if (!processor || processor->isPresent(key))
        {
          OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap::FilteredOrderedAttributeMap Setting Attribute...");
          SetAttribute(key, value);
        }
        OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap::FilteredOrderedAttributeMap Did not Set Attributes...");
        return true;
      });
  OTEL_INTERNAL_LOG_WARN("FilteredOrderedAttributeMap::FilteredOrderedAttributeMap UpdatingHash...");
  UpdateHash();
}

FilteredOrderedAttributeMap::FilteredOrderedAttributeMap(
    std::initializer_list<std::pair<nostd::string_view, opentelemetry::common::AttributeValue>>
        attributes,
    const AttributesProcessor *processor)
    : OrderedAttributeMap()
{
  for (auto &kv : attributes)
  {
    if (!processor || processor->isPresent(kv.first))
    {
      SetAttribute(kv.first, kv.second);
    }
  }

  UpdateHash();
}
}  // namespace metrics
}  // namespace sdk
OPENTELEMETRY_END_NAMESPACE
