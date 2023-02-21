#include "SIAnimationData.h"
#include "Matrix3x4.h"
#include "RHI.h"
#include "RenderingThread.h"

#pragma optimize( "", off )

FSIAnimationData::FSIAnimationData()
{
}

FSIAnimationData::~FSIAnimationData()
{
}

void FSIAnimationData::Release()
{
	ENQUEUE_RENDER_COMMAND(ReleaseSIAnimationData)(
		[this](FRHICommandList& RHICmdList)
	{
		ReleaseData_RenderThread();
	});
}

void FSIAnimationData::ReleaseData_RenderThread()
{
	VertexBufferRHI.SafeRelease();
	VertexBufferSRV.SafeRelease();
}

void FSIAnimationData::Init(int InNumBones, const TArray<int>& InSequenceLength)
{
	NumBones = InNumBones;

	SequenceLength.Empty();
	SequenceLength.Append(InSequenceLength);

	SequenceOffset.Empty();
	SequenceOffset.AddZeroed(InSequenceLength.Num());

	const uint32 FrameSize = NumBones * 3 * sizeof(FVector4);
	int Offset = 0;
	for (int SequenceIndex = 0; SequenceIndex < SequenceLength.Num(); SequenceIndex++)
	{
		SequenceOffset[SequenceIndex] = Offset;
		Offset += SequenceLength[SequenceIndex] * NumBones;
	}
}

void FSIAnimationData::Update(TArray<FMatrix>* ReferenceToLocalMatrices)
{
	// update vertex factory components and sync it
	ENQUEUE_RENDER_COMMAND(UpdateSIAnimationData)(
		[this, ReferenceToLocalMatrices](FRHICommandList& CmdList)
	{
		UpdateData_RenderThread(ReferenceToLocalMatrices);
	}
	);
}

void FSIAnimationData::UpdateData_RenderThread(TArray<FMatrix>* InReferenceToLocalMatrices)
{
	if (!InReferenceToLocalMatrices)
		return;

	TArray<FMatrix>& ReferenceToLocalMatrices = *InReferenceToLocalMatrices;

	if (VertexBufferRHI)
		VertexBufferRHI.SafeRelease();

	if (VertexBufferSRV)
		VertexBufferSRV.SafeRelease();

	uint32 BufferSize = ReferenceToLocalMatrices.Num() * 3 * sizeof(FVector4);

	FRHIResourceCreateInfo CreateInfo;
	VertexBufferRHI = RHICreateVertexBuffer(BufferSize, (BUF_Dynamic | BUF_ShaderResource), CreateInfo);
	VertexBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(FVector4), PF_A32B32G32R32F);

	if (ReferenceToLocalMatrices.Num() > 0)
	{
		const int32 PreFetchStride = 2; // FPlatformMisc::Prefetch stride
		FMatrix3x4* LockedBuffer = (FMatrix3x4*)RHILockVertexBuffer(VertexBufferRHI, 0, BufferSize, RLM_WriteOnly);

		for (size_t i = 0; i < ReferenceToLocalMatrices.Num(); i++)
		{
			FPlatformMisc::Prefetch(ReferenceToLocalMatrices.GetData() + i + PreFetchStride);
			FPlatformMisc::Prefetch(ReferenceToLocalMatrices.GetData() + i + PreFetchStride, PLATFORM_CACHE_LINE_SIZE);

			FMatrix3x4& BoneMat = LockedBuffer[i];
			const FMatrix& RefToLocal = ReferenceToLocalMatrices[i];
			RefToLocal.To3x4MatrixTranspose((float*)BoneMat.M);
		}

		RHIUnlockVertexBuffer(VertexBufferRHI);
	}

	delete InReferenceToLocalMatrices;
}

#pragma optimize( "", on )
