#include "RenderGraph.h"
#include"RenderGraphPass.h"
#include <Base/Logger.h>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <sstream>

RenderGraph::RenderGraph() {

}

RenderGraph::~RenderGraph() {

}

void RenderGraph::execute(RenderContext* renderContext) {
	compile();
	for (const RenderGraphPass* pass : executionOrder_)
	{
		const_cast<RenderGraphPass*>(pass)->execute(renderContext);
	}
}

void RenderGraph::compile() {
	stats_ = {};
	stats_.passCount = passes.size();
	executionOrder_.clear();
	if (passes.empty()) return;

	const size_t passCount = passes.size();
	std::vector<std::set<size_t>> edges(passCount);
	std::vector<size_t> indegree(passCount, 0);

	struct ResourceState {
		int lastWriter = -1;
		std::vector<size_t> readers;
	};
	std::map<std::string, ResourceState> resourceStates;

	auto addEdge = [&edges](size_t from, size_t to) {
		if (from != to) edges[from].insert(to);
	};

	for (size_t passIndex = 0; passIndex < passCount; ++passIndex) {
		for (const RenderGraphResourceAccess& resource : passes[passIndex]->resources()) {
			std::ostringstream key;
			key << resource.name;
			if (resource.identity) key << '@' << resource.identity;
			ResourceState& state = resourceStates[key.str()];

			const bool reads = resource.access == RenderGraphAccess::Read ||
				resource.access == RenderGraphAccess::ReadWrite;
			const bool writes = resource.access == RenderGraphAccess::Write ||
				resource.access == RenderGraphAccess::ReadWrite;
			if (reads && state.lastWriter >= 0) addEdge(static_cast<size_t>(state.lastWriter), passIndex);
			if (reads) state.readers.push_back(passIndex);
			if (writes) {
				if (state.lastWriter >= 0) addEdge(static_cast<size_t>(state.lastWriter), passIndex);
				for (size_t reader : state.readers) addEdge(reader, passIndex);
				state.readers.clear();
				state.lastWriter = static_cast<int>(passIndex);
			}
		}
	}

	for (size_t source = 0; source < passCount; ++source) {
		for (size_t destination : edges[source]) ++indegree[destination];
	}
	for (const auto& edgeList : edges) stats_.dependencyEdgeCount += edgeList.size();

	// 使用最小下标优先队列，保证没有依赖的 pass 保持提交顺序。
	std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> ready;
	for (size_t index = 0; index < passCount; ++index) if (indegree[index] == 0) ready.push(index);
	while (!ready.empty()) {
		const size_t current = ready.top();
		ready.pop();
		executionOrder_.push_back(passes[current].get());
		for (size_t destination : edges[current]) {
			if (--indegree[destination] == 0) ready.push(destination);
		}
	}

	if (executionOrder_.size() != passCount) {
		++stats_.cycleCount;
		Logger::Error("RenderGraph dependency cycle detected; falling back to submission order.");
		executionOrder_.clear();
		for (const auto& pass : passes) executionOrder_.push_back(pass.get());
	}
	for (size_t index = 0; index < passCount; ++index) {
		if (executionOrder_[index] != passes[index].get()) ++stats_.reorderedPassCount;
	}
}

void RenderGraph::clear() {
	passes.clear();
	executionOrder_.clear();
	stats_ = {};
}





