#include "execution/operator/aggregate/physical_aggregate.hpp"
#include "execution/operator/aggregate/physical_hash_aggregate.hpp"
#include "execution/operator/aggregate/physical_window.hpp"
#include "execution/operator/filter/physical_filter.hpp"
#include "execution/operator/helper/physical_explain.hpp"
#include "execution/operator/helper/physical_limit.hpp"
#include "execution/operator/helper/physical_prune_columns.hpp"
#include "execution/operator/join/physical_cross_product.hpp"
#include "execution/operator/join/physical_hash_join.hpp"
#include "execution/operator/join/physical_join.hpp"
#include "execution/operator/join/physical_nested_loop_join.hpp"
#include "execution/operator/join/physical_piecewise_merge_join.hpp"
#include "execution/operator/order/physical_order.hpp"
#include "execution/operator/persistent/physical_copy.hpp"
#include "execution/operator/persistent/physical_delete.hpp"
#include "execution/operator/persistent/physical_insert.hpp"
#include "execution/operator/persistent/physical_update.hpp"
#include "execution/operator/projection/physical_projection.hpp"
#include "execution/operator/scan/physical_dummy_scan.hpp"
#include "execution/operator/scan/physical_empty_result.hpp"
#include "execution/operator/scan/physical_index_scan.hpp"
#include "execution/operator/scan/physical_table_function.hpp"
#include "execution/operator/scan/physical_table_scan.hpp"
#include "execution/operator/schema/physical_create_index.hpp"
#include "execution/operator/schema/physical_create_table.hpp"
#include "execution/operator/set/physical_union.hpp"
