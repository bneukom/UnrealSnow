#pragma once

#include "Private/ShaderUtilityPrivatePCH.h"
#include "RHIResources.h"
#include "RHICommandList.h"
#include "RHI.h"

/** Encapsulates a GPU read/write structured buffer with its UAV and SRV. */
struct FRWStructuredBuffer
{
	FStructuredBufferRHIRef Buffer;
	FUnorderedAccessViewRHIRef UAV;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;
	FRWStructuredBuffer() : NumBytes(0) {}

	void Initialize(uint32 BytesPerElement, uint32 NumElements, FResourceArrayInterface* Data = nullptr, uint32 AdditionalUsage = 0, bool bUseUavCounter = false, bool bAppendBuffer = false)
	{
		check(GMaxRHIFeatureLevel == ERHIFeatureLevel::SM5);
		NumBytes = BytesPerElement * NumElements;
		FRHIResourceCreateInfo CreateInfo;
		CreateInfo.ResourceArray = Data;
		Buffer = RHICreateStructuredBuffer(BytesPerElement, NumBytes, BUF_UnorderedAccess | BUF_ShaderResource | AdditionalUsage, CreateInfo);
		UAV = RHICreateUnorderedAccessView(Buffer, bUseUavCounter, bAppendBuffer);
		SRV = RHICreateShaderResourceView(Buffer);
	}

	void Release()
	{
		NumBytes = 0;
		Buffer.SafeRelease();
		UAV.SafeRelease();
		SRV.SafeRelease();
	}
};