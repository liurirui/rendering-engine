#include "RenderGraph.h"
#include"RenderGraphPass.h"

RenderGraph::RenderGraph() {

}

RenderGraph::~RenderGraph() {

}

void RenderGraph::execute(RenderContext* renderContext) {

	for (const auto & pass : passes)
	{
		pass->execute(renderContext);
	}
}





