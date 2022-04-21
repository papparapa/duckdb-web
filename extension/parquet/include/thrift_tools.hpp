#pragma once
#include <iostream>
#include "thrift/protocol/TCompactProtocol.h"
#include "thrift/transport/TBufferTransports.h"

#include "duckdb.hpp"
#ifndef DUCKDB_AMALGAMATION
#include "duckdb/common/file_system.hpp"
#include "duckdb/common/allocator.hpp"
#endif

namespace duckdb {

struct PrefetchBuffer {
	PrefetchBuffer(idx_t location, size_t size, Allocator &allocator) : location(location) {
		data = allocator.Allocate(size);
	}
	idx_t location;
	unique_ptr<AllocatedData> data;
	bool operator == (const PrefetchBuffer& other) const { return data == other.data; }
};

/// A ReadHead contains a range that was hinted to be accessed in the near future.
struct ReadHead {
	ReadHead(idx_t location, size_t size) : location(location), size(size){};
	/// Hint info
	idx_t location;
	size_t size;

	/// Current info
	unique_ptr<AllocatedData> data;

	idx_t GetEnd() const {
		return size + location;
	}

	void Allocate(Allocator &allocator) {
		data = allocator.Allocate(size);
	}
};

/// Read ahead buffer implementation that requires specifying the ranges that can be accesssed
struct ReadAheadBuffer {
	ReadAheadBuffer(Allocator &allocator, FileHandle &handle) : allocator(allocator), handle(handle) {
	}

	/// The list of read heads
	std::list<ReadHead> read_heads;
	Allocator &allocator;
	FileHandle &handle;

	std::map<idx_t, ReadHead*> start_map;
	std::map<idx_t, ReadHead*> end_map;

	idx_t total_size = 0;

	/// Add a read head to the prefetching list
	void AddReadHead(idx_t pos, idx_t len, bool merge_buffers = true) {

		// Attempt to merge with existing
		if (merge_buffers) {
			auto lookup_start = end_map.find(pos);
			if (lookup_start != end_map.end()) {
				auto read_head = lookup_start->second;
				// Merge existing read head with this one
				read_head->size += len;
				// Add new end
				end_map.insert(std::pair<idx_t, ReadHead*>(read_head->GetEnd(), read_head));
				// Erase old end
				end_map.erase(lookup_start->first);
				return;
			}

			auto lookup_end = start_map.find(pos + len);
			if (lookup_end != start_map.end()) {
				auto read_head = lookup_start->second;
				// Merge existing read head with this one
				read_head->location -= len;
				read_head->size += len;
				// Add new start
				start_map.insert(std::pair<idx_t, ReadHead*>(read_head->location, read_head));
				// Erase old start
				end_map.erase(lookup_start->first);
				return;
			}
		}

		// No merge candidate found, just add it
		read_heads.emplace_front(ReadHead(pos, len));
		total_size += len;

		auto& read_head = read_heads.front();

		// Insert begin and end into maps for later merge lookups
		start_map.insert(std::pair<idx_t, ReadHead*>(read_head.location, &read_head));
		end_map.insert(std::pair<idx_t, ReadHead*>(read_head.GetEnd(), &read_head));
	}

	/// Returns the relevant read head
	ReadHead* GetReadHead(idx_t pos) {
		for (auto& read_head: read_heads) {
			if (pos >= read_head.location && pos < read_head.GetEnd()) {
				return &read_head;
			}
		}
		return nullptr;
	}

	/// Prefetch all read heads
	void Prefetch() {
		// TODO we could do these prefetches in parallel probably
		for (auto& read_head: read_heads) {
			read_head.Allocate(allocator);
			std::cout << "Prefetching registered: " << read_head.location << " till " << read_head.location + read_head.size << " bytes (total:" << read_head.size << ") \n";
			handle.Read(read_head.data->get(), read_head.size, read_head.location);
		}
	}
};

class ThriftFileTransport : public duckdb_apache::thrift::transport::TVirtualTransport<ThriftFileTransport> {
public:
	ThriftFileTransport(Allocator &allocator, FileHandle &handle_p)
	    : allocator(allocator), handle(handle_p), location(0), ra_buffer(ReadAheadBuffer(allocator, handle_p)) {
	}

	uint32_t read(uint8_t *buf, uint32_t len) {
		if (prefetched_data && location >= prefetch_location &&
		    location + len < prefetch_location + prefetched_data->GetSize()) {
//			std::cout << "Reading through old prefetch: " << location << " till " << location + len << " bytes \n";
			memcpy(buf, prefetched_data->get() + location - prefetch_location, len);
		} else {
			auto prefetch_buffer = ra_buffer.GetReadHead(location);
			if (prefetch_buffer != nullptr) {
				memcpy(buf, prefetch_buffer->data->get() + location - prefetch_buffer->location, len);
			} else {
				std::cout << "Directly reading: " << location << " till " << location + len << " bytes (total:" << len << ") \n";
				handle.Read(buf, len, location);
			}
		}
		location += len;
		return len;
	}

	/// Old prefetch
	void Prefetch(idx_t pos, idx_t len) {
		std::cout << "Prefetching OLD: " << pos << " till " << pos + len << " bytes (total:" << len << ") \n";
		prefetch_location = pos;
		prefetched_data = allocator.Allocate(len);
		handle.Read(prefetched_data->get(), len, prefetch_location); // here
	}
	void ClearPrefetch() {
		prefetched_data.reset();
		prefetched_data = nullptr;
	}

	/// New prefetch
	void RegisterPrefetch(idx_t pos, idx_t len) {
		ra_buffer.AddReadHead(pos,len);
	}
	void PrefetchRegistered() {
		ra_buffer.Prefetch();
	}
	void ClearRegisterPrefetch() {
		ra_buffer.read_heads.clear();
	}

	void SetLocation(idx_t location_p) {
		location = location_p;
	}

	idx_t GetLocation() {
		return location;
	}
	idx_t GetSize() {
		return handle.file_system.GetFileSize(handle);
	}

private:
	Allocator &allocator;
	FileHandle &handle;
	idx_t location;

	// Main full row_group prefetch
	unique_ptr<AllocatedData> prefetched_data;
	idx_t prefetch_location;

	// Multi-buffer prefetch
	ReadAheadBuffer ra_buffer;
};

} // namespace duckdb
