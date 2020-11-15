#include "duckdb/execution/operator/scan/physical_chunk_scan.hpp"

using namespace std;

namespace duckdb {

class PhysicalChunkScanState : public PhysicalOperatorState {
public:
	PhysicalChunkScanState(PhysicalOperator &op) : PhysicalOperatorState(op, nullptr), chunk_index(0) {
	}

	//! The current position in the scan
	idx_t chunk_index;
};

void PhysicalChunkScan::GetChunkInternal(ExecutionContext &context, DataChunk &chunk, PhysicalOperatorState *state_) {
	auto state = (PhysicalChunkScanState *)state_;
	D_ASSERT(collection);
	if (collection->count == 0) {
		return;
	}
	D_ASSERT(chunk.GetTypes() == collection->types);
	if (state->chunk_index >= collection->chunks.size()) {
		return;
	}
	auto &collection_chunk = *collection->chunks[state->chunk_index];
	chunk.Reference(collection_chunk);
	state->chunk_index++;
}

unique_ptr<PhysicalOperatorState> PhysicalChunkScan::GetOperatorState() {
	return make_unique<PhysicalChunkScanState>(*this);
}

} // namespace duckdb