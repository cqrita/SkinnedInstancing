#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "InstancedSkinnedMeshComponent.generated.h"

USTRUCT()
struct FInstancedSkinnedMeshInstanceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Instances)
	FMatrix Transform;

	FInstancedSkinnedMeshInstanceData()
		: Transform(FMatrix::Identity)
	{
	}

	FInstancedSkinnedMeshInstanceData(const FMatrix& InTransform)
		: Transform(InTransform)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FInstancedSkinnedMeshInstanceData& InstanceData)
	{
		Ar << InstanceData.Transform;
		return Ar;
	}
};

UCLASS(hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class ENGINE_API UInstancedSkinnedMeshComponent : public UMeshComponent
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

private:
	/** Internal version of AddInstance */
	int32 AddInstanceInternal(int32 InstanceIndex, FInstancedSkinnedMeshInstanceData* InNewInstanceData, const FTransform& InstanceTransform);

public:
	/** Array of instances, bulk serialized. */
	UPROPERTY(EditAnywhere, SkipSerialization, DisplayName = "Instances", Category = Instances, meta = (MakeEditWidget = true, EditFixedOrder))
	TArray<FInstancedSkinnedMeshInstanceData> PerInstanceSMData;

	/** The skeletal mesh used by this component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	class USkeletalMesh* SkeletalMesh;

	/** Object responsible for sending bone transforms, morph target state etc. to render thread. */
	class FInstancedSkinnedMeshObject* MeshObject;

public:
	/** Add an instance to this component. Transform is given in local space of this component. */
	UFUNCTION(BlueprintCallable, Category = "Components|InstancedSkinMesh")
	virtual int32 AddInstance(const FTransform& InstanceTransform);

private:
	friend class FInstancedSkinnedMeshSceneProxy;
};
