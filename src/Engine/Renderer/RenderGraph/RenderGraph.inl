#pragma once

template<typename ParameterStructType, typename ExecuteLambdaFunction>
inline bool RenderGraph::addPass(const char* passName, ParameterStructType* parameterStruct, ExecuteLambdaFunction&& executeLambdaFunction) {

	auto* pass = new RenderGraphExecutePass<ParameterStructType, ExecuteLambdaFunction>(passName, parameterStruct, std::forward<ExecuteLambdaFunction>(executeLambdaFunction));
	this->passes.push_back(pass);

	return true;
}




