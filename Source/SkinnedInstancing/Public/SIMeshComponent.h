#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "SIMeshComponent.generated.h"

struct FInstancedSkinnedMeshInstanceData
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
class SKINNEDINSTANCING_API USIMeshComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()
	
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

protected:
	//~ Begin UActorComponent Interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
	virtual bool RequiresGameThreadEndOfFrameRecreate() const override { return false; }
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual UObject const* AdditionalStatObject() const override { return SkeletalMesh; }
	//~ End UActorComponent Interface

public:

	/** The skeletal mesh used by this component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Instances")
	USkeletalMesh* SkeletalMesh;

	/** The AnimSequence used by this component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Instances")
	TArray<class UAnimSequence*> AnimSequences;

	/** Object responsible for sending bone transforms, morph target state etc. to render thread. */
	class FInstancedSkinnedMeshObject* MeshObject;

private:
	void UpdateMeshObejctDynamicData();

private:
	TMap<int, FInstancedSkinnedMeshInstanceData> PerInstanceSMData;
	int InstanceIdIncrease;

public:
	int32 AddInstance(const FTransform& Transform);

	void RemoveInstance(int Id);
	
	UAnimSequence* GetSequence(int Id);

	FInstancedSkinnedMeshInstanceData* GetInstanceData(int Id);

private:
	friend class FInstancedSkinnedMeshSceneProxy;
};
