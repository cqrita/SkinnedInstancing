#include "SIAnimationComponent.h"
#include "BonePose.h"
#include "SIAnimationData.h"

#pragma optimize( "", off )

USIAnimationComponent::USIAnimationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	AnimationData = nullptr;
}

UAnimSequence * USIAnimationComponent::GetSequence(int Id)
{
	if (Id >= AnimSequences.Num())
		return nullptr;
	return AnimSequences[Id];
}

USIAnimationComponent::~USIAnimationComponent()
{
}

void USIAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USIAnimationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void USIAnimationComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	if (Skeleton && AnimSequences.Num() > 0 && AnimSequences[0])
	{
		// No need to create the mesh object if we aren't actually rendering anything (see UPrimitiveComponent::Attach)
		if (FApp::CanEverRender() && ShouldComponentAddToScene())
		{
			CreateAnimationData();
		}
	}

	Super::CreateRenderState_Concurrent(Context);
}

void USIAnimationComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();

	if (AnimationData)
	{
		AnimationData->Release();

		// Begin a deferred delete of MeshObject.  BeginCleanup will call MeshObject->FinishDestroy after the above release resource
		// commands execute in the rendering thread.
		BeginCleanup(AnimationData);
		AnimationData = nullptr;
	}
}

namespace
{
	void UpdateBoneData(TArray<FMatrix>& BoneMatrices, int SequenceOffset, UAnimSequence* AnimSequence, const FBoneContainer* BoneContainer)
	{
		FCompactPose OutPose;
		FBlendedCurve OutCurve;
		FStackCustomAttributes AdditiveAttributes;
		OutPose.SetBoneContainer(BoneContainer);
		OutPose.ResetToRefPose();
		FAnimationPoseData AnimationPoseData(OutPose, OutCurve, AdditiveAttributes);
		

		int NumBones = BoneContainer->GetReferenceSkeleton().GetRawBoneNum();
		int NumFrames = AnimSequence->GetNumberOfFrames();
		float Interval = (NumFrames > 1) ? (AnimSequence->SequenceLength / (NumFrames - 1)) : MINIMUM_ANIMATION_LENGTH;

		check(NumFrames > 0);

		for (int FrameIndex = 0; FrameIndex < NumFrames; FrameIndex++)
		{
			float Time = FrameIndex * Interval;
			AnimSequence->GetBonePose(AnimationPoseData, FAnimExtractContext(Time));

			TArray<FTransform> ComponentSpaceTransforms;

			ComponentSpaceTransforms.AddUninitialized(NumBones);

			auto& LocalTransform = AnimationPoseData.GetPose().GetBones();

			check(LocalTransform.Num() == ComponentSpaceTransforms.Num());

			const FTransform* LocalTransformsData = LocalTransform.GetData();
			FTransform* ComponentSpaceData = ComponentSpaceTransforms.GetData();

			ComponentSpaceTransforms[0] = LocalTransform[0];

			for (int32 BoneIndex = 1; BoneIndex < LocalTransform.Num(); BoneIndex++)
			{
				// For all bones below the root, final component-space transform is relative transform * component-space transform of parent.
				const int32 ParentIndex = BoneContainer->GetReferenceSkeleton().GetParentIndex(BoneIndex);
				FTransform* ParentSpaceBase = ComponentSpaceData + ParentIndex;
				FPlatformMisc::Prefetch(ParentSpaceBase);

				FTransform* SpaceBase = ComponentSpaceData + BoneIndex;

				FTransform::Multiply(SpaceBase, LocalTransformsData + BoneIndex, ParentSpaceBase);

				SpaceBase->NormalizeRotation();

				checkSlow(SpaceBase->IsRotationNormalized());
				checkSlow(!SpaceBase->ContainsNaN());
			}

			for (int BoneIndex = 0; BoneIndex < NumBones; BoneIndex++)
			{
				int PoseDataOffset = SequenceOffset + NumBones * FrameIndex + BoneIndex;
				BoneMatrices[PoseDataOffset] = ComponentSpaceTransforms[BoneIndex].ToMatrixWithScale();
			}
		}
	}
}

void USIAnimationComponent::CreateAnimationData()
{
	TArray<UAnimSequence*> AnimSequencesExist;
	for (int i = 0; i < AnimSequences.Num(); i++)
	{
		if (AnimSequences[i])
		{
			AnimSequencesExist.Add(AnimSequences[i]);
		}
	}

	if (AnimSequencesExist.Num() < 0)
		return;

	AnimationData = new FSIAnimationData();

	int NumBones = Skeleton->GetReferenceSkeleton().GetRawBoneNum();

	FBoneContainer BoneContainer;
	TArray<FBoneIndexType> RequiredBones;
	for (int i = 0; i < NumBones; i++)
		RequiredBones.Add(i);
	BoneContainer.InitializeTo(RequiredBones, FCurveEvaluationOption(), *Skeleton);

	int NumBoneMatrices = 0;
	TArray<int> SequenceLengths;
	SequenceLengths.AddZeroed(AnimSequencesExist.Num());
	for (int i = 0; i < AnimSequencesExist.Num(); i++)
	{
		SequenceLengths[i] = AnimSequencesExist[i]->GetNumberOfFrames();
		NumBoneMatrices += SequenceLengths[i] * NumBones;
	}

	AnimationData->Init(NumBones, SequenceLengths);

	TArray<FMatrix>* BoneMatrices = new TArray<FMatrix>();
	BoneMatrices->AddUninitialized(NumBoneMatrices);

	int SequenceOffset = 0;
	for (int i = 0; i < AnimSequencesExist.Num(); i++)
	{
		FName SavedRetargetSource = AnimSequencesExist[i]->RetargetSource;
		if (RetargetSource.IsValid())
			AnimSequencesExist[i]->RetargetSource = RetargetSource;
		UpdateBoneData(*BoneMatrices, SequenceOffset, AnimSequencesExist[i], &BoneContainer);
		SequenceOffset += AnimSequencesExist[i]->GetNumberOfFrames() * NumBones;
		AnimSequencesExist[i]->RetargetSource = SavedRetargetSource;
	}

	AnimationData->Update(BoneMatrices);
}

#pragma optimize( "", on )
