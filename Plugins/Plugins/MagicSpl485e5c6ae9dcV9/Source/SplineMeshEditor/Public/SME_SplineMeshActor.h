// Copyright 2022 Triple Scale Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "SME_SplineLayer.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "Chaos/ChaosEngineInterface.h"
#endif
#include "SME_SplineMeshActor.generated.h"

USTRUCT(BlueprintType)
struct FSMEBranchData
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		AActor* Branch = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "0"))
		float PositionOnSpline = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FVector Offset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FRotator Rotation = FRotator::ZeroRotator;

};

USTRUCT(BlueprintType)
struct FSMEArcSettings
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	* Creates a circle or polygon with the number of spline points set in numCircularPoints.
	* Forces Closed Loop to true.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool MakeArc = false;

	/**
	* The number of spline points of the generated Arc.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "3", EditCondition = "MakeArc == true"))
		int NumArcPoints = 8;

	/**
	* The height and width of the Arc
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "MakeArc == true"))
		FVector2D CircularRadius = FVector2D(100.0f, 100.0f);

	/**
	* Force X and Y values of CircularRadius to be equals
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "MakeArc == true"))
		bool SyncRadius = true;

	/**
	* Rotates the position of all points around the root of the actor
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "0", ClampMax = "360", EditCondition = "MakeArc == true"))
		float PointRotationOffset = 0.0f;

	/**
	* Angle of the circle to generate
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "0", ClampMax = "360", EditCondition = "MakeArc == true"))
		int DegreesOfArc = 360;
};

USTRUCT(BlueprintType)
struct FSMEAngularLimits
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte<EAngularConstraintMotion> Swing1LimitMode = EAngularConstraintMotion::ACM_Locked;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		float Swing1LimitAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte <EAngularConstraintMotion> Swing2LimitMode = EAngularConstraintMotion::ACM_Locked;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		float Swing2LimitAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte <EAngularConstraintMotion> TwistLimitMode = EAngularConstraintMotion::ACM_Locked;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		float TwistLimitAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		float AngularDamping = 0.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		float Mass = 100.0f;
};

USTRUCT(BlueprintType)
struct FSMELinearLimits
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte <ELinearConstraintMotion> xMotion = ELinearConstraintMotion::LCM_Locked;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte <ELinearConstraintMotion> yMotion = ELinearConstraintMotion::LCM_Locked;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte <ELinearConstraintMotion> zMotion = ELinearConstraintMotion::LCM_Locked;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		float LimitAmount = 0.0f;

};

USTRUCT(BlueprintType)
struct FSMEPhysicData
{
	GENERATED_USTRUCT_BODY()
public:

	//When true, physics constraints are generated between segments (only for static meshes)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool SetupPhysicsConnections = false;

	//If true a physics constrain will be added at the end of the spline so that both ends will be fixed in place. This is useful for rope bridges and such.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool AttachEnd = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FSMEAngularLimits AngularLimits;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FSMELinearLimits LinearLimits;
};

UENUM(BlueprintType)
enum ESMESplinePointType
{
	UserDefined	UMETA(DisplayName = "User Defined"),
	Curved		UMETA(DisplayName = "Curved"),
	Linear		UMETA(DisplayName = "Linear")
};


UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPLINEMESHEDITOR_API ASME_SplineMeshActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASME_SplineMeshActor();
	~ASME_SplineMeshActor();

	// Called when modifying the object in the editor
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Called when this actor is explicitly being destroyed during gameplay or in the editor, not called during level streaming or gameplay ending */
	virtual void Destroyed() override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ConstructSplineMeshActor();
	/**
	* Actors to attach to the Spline
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Branches")
		TArray<FSMEBranchData> Branches;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Default")
		USplineComponent* SplineComponent;

	/**
	* Read only. The total number of points on the spline. A convenience to avoid having to count manually
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Default")
		int NumSplinePoints = 1;


	/**
	* Data about the layers of the spline
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Layers")
		TArray<FSMELayerData> SplineLayersData;

	/**
	* Use the value of SplineUtilization and SegmentStartPercentage of the 1st layer for all other layers
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Layers")
		bool DuplicateSegmentsPercentages = true;

	/**
	* Array of layers, each layers contain an array of spline segments with a spline mesh or a static mesh
	*/
		TArray<USME_SplineLayer*> SplineLayers;

	/**
	* Array of layers, each layers contain an array of spline segments with a spline mesh or a static mesh
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Layers")
		TArray<USME_SplineLayer*> SplineLayersRuntime;

	/**
	* Allow the meshes of the layer to be movable or not
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineOptions")
		TEnumAsByte <EComponentMobility::Type> Mobility = EComponentMobility::Static;

	/**
	* Whether the spline is to be considered as a closed loop
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineOptions")
		bool ClosedLoop = false;

	/**
	* Set the type of spline point
	* UserDefined: The type of the points are defined individually in the spline settings
	* Curved: All points are using curved tangents
	* Linear: All points are using linear tangents
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineOptions")
		TEnumAsByte<ESMESplinePointType> SplinePointMode = ESMESplinePointType::UserDefined;


	/**
	* Tool to create arc spline
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineOptions")
		FSMEArcSettings ArcSettings;

	/**
	* Use a pre-existing spline of another actor (the actor need to have a SplineComponent)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineOptions")
		AActor* ExternalSplineActor = nullptr;

	/**
	* Reference to the External SplineMeshActor so it can be updated when the External Spline is modified
	*/
	UPROPERTY(BlueprintReadOnly, Category = "SplineOptions")
		ASME_SplineMeshActor* LinkedExternalSplineMeshActor = nullptr;

	/**
	* Settings to enable the physics on the static mesh present on the layers
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Physics")
		FSMEPhysicData PhysicData;

	/**
	* StaticMesh to use as a reference for the Anchor
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Physics")
		UStaticMesh* DefaultAnchorMesh = nullptr;

	/**
	* Material to identify the anchor when using physics
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Physics")
		UMaterialInterface* DefaultAnchorMaterial = nullptr;

	/**
	* Use a pre-existing mesh as an anchor (need a StaticMeshComponent inside)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Physics")
		AActor* ExternalEndAnchorActor = nullptr;

	/**
	* To hide the anchors when playing
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Physics")
		bool HideDefaultAnchorInGame = true;

	/**
	* Detect Hit event and break the physicConstraints of the static mesh collided
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Physics")
		bool BreackablePhysicsConstraints = true;

	/**
	* Modify the spline in parameter to make it circular
	*/
	UFUNCTION(BlueprintCallable, Category = "MagicSplineMesh")
		static void MakeSplineArc(USplineComponent* SplineComp, int NumPoints, FVector2D Radius, float RotationOffset, float Degrees, bool Closed, ESMESplinePointType PointMode = ESMESplinePointType::UserDefined);
	
	UFUNCTION(BlueprintCallable, Category = "MagicSplineMesh")
		void UpdateConstruction();

	/**
	* Modify the spline at runtime if the Mobility is set to Movable
	*/
	UFUNCTION(BlueprintCallable, Category = "MagicSplineMesh")
		void UpdateSplineAtRuntime();

	UFUNCTION(BlueprintCallable, Category = "MagicSplineMesh")
		float GetSplineUtilization();

	UFUNCTION(BlueprintCallable, Category = "MagicSplineMesh")
		void SetSplineUtilization(float value);

	UFUNCTION()
		void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
private:

	static void SetElipseTangent(USplineComponent* SplineComp, int TargetPointIndex, int ReferencePointIndex, FVector2D Radius);
	AActor* PreviousExternalSplineActor = nullptr;
	bool ShouldUpdateLinkedExternalSpline = false;

#if ENGINE_MAJOR_VERSION >= 5
	void InitPhysicsConstraints();
	void AddPhysicsConstraint(UStaticMeshComponent* Comp1, UStaticMeshComponent* Comp2);
	void SetNeutralPhysics(UStaticMeshComponent* StaticMeshComp);
#endif

	UPROPERTY()
		TArray<UPhysicsConstraintComponent*> PhysicsConstraints;
	

	UPROPERTY()
		TArray<AActor*> PreviousBranches;

	UPROPERTY()
		TArray<FTransform> PreviousBranchesInitialTransform;

	UFUNCTION()
		int FindBranch(AActor* ActorToFind);

	bool bIsRuningBeginPlay = false;
};

