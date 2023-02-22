#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "SIMeshComponent.generated.h"

struct FSIMeshInstanceData
{
	struct FAnimData
	{
		int Sequence;
		int PrevFrame;
		int NextFrame;
		float FrameLerp;
		float BlendWeight;
	};
	FMatrix Transform;
	FAnimData AnimDatas[2];
};

UCLASS(hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class SKINNEDINSTANCING_API USIMeshComponent : public USkinnedMeshComponent
{
	GENERATED_BODY()
	
	//~ Override Functions
private:
	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	//~ Begin UObject Interface
	//~ End UObject Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.
	USIMeshComponent(const FObjectInitializer& ObjectInitializer);
protected:
	//~ Begin UActorComponent Interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
	virtual bool RequiresGameThreadEndOfFrameRecreate() const override { return false; }
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual UObject const* AdditionalStatObject() const override { return SkeletalMesh; }
	//~ End UActorComponent Interface

public:

	/** Object responsible for sending bone transforms, morph target state etc. to render thread. */
	class FSIMeshObject* MeshObject;

	/** The AnimSequence used by this component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkinnedInstancing")
	TArray<UAnimSequence*> AnimSequences;

	/** Base pose to use when retargeting */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkinnedInstancing")
	FName RetargetSource;

	UFUNCTION(BlueprintCallable, Category = "Components|SkinnedInstancing")
	UAnimSequence* GetSequence(int Id);

	const class FSIAnimationData* GetAnimationData() const { return AnimationData; }
private:
	void UpdateMeshObejctDynamicData();
	void CreateAnimationData();
	void UpdateBoneData(TArray<FMatrix>& BoneMatrices, int SequenceOffset, UAnimSequence* AnimSequence, const FBoneContainer* BoneContainer);

private:
	TMap<int, FSIMeshInstanceData> PerInstanceSMData;
	int InstanceIdIncrease;
	class FSIAnimationData* AnimationData;
public:
	int32 AddInstance(const FTransform& Transform);

	void RemoveInstance(int Id);

	FSIMeshInstanceData* GetInstanceData(int Id);

private:
	friend class FSIMeshSceneProxy;
};
