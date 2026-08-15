#pragma once

template<typename ParameterStructType, typename ExecuteLambdaFunction>
inline bool RenderGraph::addPass(const char* passName, ParameterStructType* parameterStruct, ExecuteLambdaFunction&& executeLambdaFunction) {
	// 当前 RenderGraph 是顺序 pass 容器；参数指针是非拥有的，调用方需保证执行前有效。

	auto* pass = new RenderGraphExecutePass<ParameterStructType, ExecuteLambdaFunction>(passName, parameterStruct, std::forward<ExecuteLambdaFunction>(executeLambdaFunction));
	pass->describe({});
	this->passes.push_back(std::unique_ptr<RenderGraphPass>(pass));

	return true;
}

template<typename ParameterStructType, typename ExecuteLambdaFunction>
inline bool RenderGraph::addPass(const char* passName, ParameterStructType* parameterStruct,
	std::initializer_list<RenderGraphResourceAccess> resources,
	ExecuteLambdaFunction&& executeLambdaFunction) {
	auto* pass = new RenderGraphExecutePass<ParameterStructType, ExecuteLambdaFunction>(
		passName, parameterStruct, std::forward<ExecuteLambdaFunction>(executeLambdaFunction));
	pass->describe(std::vector<RenderGraphResourceAccess>(resources));
	this->passes.push_back(std::unique_ptr<RenderGraphPass>(pass));
	return true;
}




