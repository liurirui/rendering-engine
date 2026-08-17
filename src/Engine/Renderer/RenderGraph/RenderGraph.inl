#pragma once

template<typename ParameterStructType, typename ExecuteLambdaFunction>
inline bool RenderGraph::addPass(const char* passName, ParameterStructType* parameterStruct, ExecuteLambdaFunction&& executeLambdaFunction) {
	// RenderGraph 按提交顺序收集 Pass，执行前会根据资源读写声明构建依赖并进行稳定拓扑排序。
	// 参数指针仍然是非拥有的，调用方必须保证其在 Graph 执行期间有效。

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




