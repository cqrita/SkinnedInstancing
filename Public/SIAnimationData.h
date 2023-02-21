#pragma once
#include "CoreMinimal.h"

class FSIAnimationData : public FDeferredCleanupInterface
{
public:
	FSIAnimationData();

	virtual ~FSIAnimationData();

	void Init(int InNumBones, const TArray<int>& InSequenceLength);

	void Update(TArray<FMatrix>* ReferenceToLocalMatrices);

	void Release();

	bool IsBufferValid() { return IsValidRef(VertexBufferRHI) && IsValidRef(VertexBufferSRV); }

	const FShaderResourceViewRHIRef& GetSRVForReading() const { return VertexBufferSRV; }

	uint32 GetNumBones() const { return NumBones; }

	const TArray<uint32>& GetSequenceOffset() const { return SequenceOffset; }

	const TArray<uint32>& GetSequenceLength() const { return SequenceLength; }

private:
	void UpdateData_RenderThread(TArray<FMatrix>* InReferenceToLocalMatrices);
	void ReleaseData_RenderThread();

private:
	uint32 NumBones;
	TArray<uint32> SequenceOffset;
	TArray<uint32> SequenceLength;
private:
	FVertexBufferRHIRef VertexBufferRHI;
	FShaderResourceViewRHIRef VertexBufferSRV;
};
