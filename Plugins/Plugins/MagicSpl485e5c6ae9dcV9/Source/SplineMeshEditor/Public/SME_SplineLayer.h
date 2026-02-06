// Copyright 2022 Triple Scale Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SplineComponent.h"
#include "SME_Segment.h"
#include "UObject/NoExportTypes.h"
#include "SME_SplineLayer.generated.h"

namespace SME_SplineLayer
{
	static const float MinMeshLength = 1.0f;
}

UENUM(BlueprintType)
enum ESMECenteredPivotMode
{
	Up		UMETA(DisplayName = "Up"),
	Center	UMETA(DisplayName = "Center"),
	Down	UMETA(DisplayName = "Down"),
};

UENUM(BlueprintType)
enum ESMEStretchMode
{
	FixedSegmentLength		UMETA(DisplayName = "Fixed Segment Length"),
	FixedSegmentCount		UMETA(DisplayName = "Fixed Segment Count"),
	SegmentsPerSplinePoint	UMETA(DisplayName = "Segments Per Spline Point"),
	SemiFixedSegmentLength	UMETA(DisplayName = "Semi-fixed Segment Length")
};

USTRUCT(BlueprintType)
struct FSMEMeshDistribution
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	* The amount of space between segments. If set negative this determines how much each segment will overlap.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		float GapLength = 0.0f;

	/**
	* Use the size of the mesh as size of segment
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool UseDefaultMeshSize = false;

	/**
	* Sets the mode that determines SegmentLength and NumSegments
	* FixedSegmentLength: Allow to specify the length of each mesh of the layer, NumSegments will be computed automatically
	* FixedSegmentCount: Allow to specify the number of segments/meshes of each layer, SegmentLength will be computed automatically
	* SegmentsPerSplinePoint: Create a meshes between each points of the spline. The length is equal to the distance between the points.
	* SemiFixedSegmentLength: Same as FixedSegmentLength but stretch the meshes in order to use all the spline length. 
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte<ESMEStretchMode> DistributionMode = ESMEStretchMode::FixedSegmentLength;

	/**
	* The length, along the spline of an individual spline mesh segment (see FixedSegmentLength mode). 
	* Can't be edited if UseDefaultMeshSize is true
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "1", EditCondition = "DistributionMode == ESMEStretchMode::FixedSegmentLength && UseDefaultMeshSize == false"))
		float SegmentsLength = 100.0f;

	/**
	* The total number of spline mesh segments (see FixedSegmentCount mode)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "1", EditCondition = "DistributionMode == ESMEStretchMode::FixedSegmentCount"))
		int NumSegments = 1;

	/**
	* The amount of segments for each points of the spline (see SegmentsPerSplinePoint mode)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "1", EditCondition = "DistributionMode == ESMEStretchMode::SegmentsPerSplinePoint"))
		int NumSegmentsPerSplinePoint = 1;

	/*
	* Semi-fixed Segment Length to use to stretch the meshes to avoid unused space at the end of the spline (see SemiFixedSegmentLength mode)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "1", EditCondition = "DistributionMode == ESMEStretchMode::SemiFixedSegmentLength && UseDefaultMeshSize == false"))
		float TargetSegmentLength = 100.0f;

	/**
	* The percentage of the spline that should not be considerate to place segments.
	* Can be changed only on 1st layer when DuplicateSegmentsPercentages is true
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "0", ClampMax = "1"))
		float SplineUtilization = 1.0f;

	/**
	* Offset to push the segments along the spline
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "0", ClampMax = "1"))
		float SegmentStartPercentage = 0.0f;

	/**
	* Use only the space available on the spline define by SplineUtilization
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool OnlyUseRemainingLength = false;
};

USTRUCT(BlueprintType)
struct FSMEScaleOffset
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	* A curve that defines the scale of each segment along the length of the spline.
	* When Scale Curve is defined, all four values are used to remap the curve.
	* When no Scale Curve is defined, only the Max values are considered
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		UCurveFloat* ScaleCurve = nullptr;

	//Needs a scale curve to be editable
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "ScaleCurve != nullptr"))
		float MinScaleWidth = 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		float MaxScaleWidth = 1.0;
		
	//Needs a scale curve to be editable
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "ScaleCurve != nullptr"))
		float MinScaleHeight = 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		float MaxScaleHeight = 1.0;

	//For StaticMeshes only. Scale the Length of the static meshes (this is not linked to the scale curve)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		float ExtraLength = 0.0f;
};

USTRUCT(BlueprintType)
struct FSMEScaleOverride
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	* Use the same scale as the other meshes of the layer
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool MatchLayerScale = true;

	/*
	* Modify the width of the start of the mesh
	* Apply to all the mesh's width if UseStaticMeshes = false or Deform = true
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "MatchLayerScale == false"))
		float ScaleWidthStart = 1.0;

	/*
	* Modify the height of the start of the mesh
	* Apply to all the mesh's height if UseStaticMeshes = false or Deform = true
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "MatchLayerScale == false"))
		float ScaleHeightStart = 1.0;

	/*
	* Modify the width of the end of the mesh
	* Need UseStaticMeshes = false and Deform = true in order to apply
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "MatchLayerScale == false"))
		float ScaleWidthEnd = 1.0;

	/*
	* Modify the height of the end of the mesh
	* Need UseStaticMeshes = false and Deform = true in order to apply
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "MatchLayerScale == false"))
		float ScaleHeightEnd = 1.0;
};

USTRUCT(BlueprintType)
struct FSMELocalOffsets
{
	GENERATED_USTRUCT_BODY()
public:

	//Sets the local offset relative to the sline location of all segments.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FVector LocalLocationOffset = FVector::ZeroVector;

	//Sets the world offset Location
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FVector WorldLocationOffset = FVector::ZeroVector;

	//Sets the base rotation of all segments.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FRotator RotationOffset = FRotator::ZeroRotator;

	//Sets the base rotation of all segments.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool AllowWorldOffsetRotation = true;

	//Recursively adds the Rotation Offset value to each segment with the X axis.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool CumulativeSegmentRotationX = false;

	//Recursively adds the Rotation Offset value to each segment with the Y axis.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool CumulativeSegmentRotationY = false;

	//Recursively adds the Rotation Offset value to each segment with the Z axis.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool CumulativeSegmentRotationZ = false;

	// Options to scale the Segments
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FSMEScaleOffset ScaleData;

};


USTRUCT(BlueprintType)
struct FSMESplineTwistData
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	* Use a curve to determines how much twist is applied at any given spot along the spline.
	* Do nothing when Use Static Meshes is set to true
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		UCurveFloat* TwistCurve = nullptr;

	/**
	* Defines the number of degrees the entire spline will twist.
	* Do nothing when Use Static Meshes is set to true
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		float SplineTwistDegrees = 0.0f;

	/**
	* AAllows for Spline Twist Degrees to rotate the beginning and the end of the meshes.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool SeamlessTwist = true;

	/**
	* Allows to Twist the SplineMesh when rotating spline points
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool SplineRollTwist = true;

	/**
	* Multiplies the Spline Twist degrees value by numSegments and uses that as the total twist across the spline
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool CumualativeSegmentTwist = false;

};


USTRUCT(BlueprintType)
struct FSMEMeshesData
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	* Specify a seed to generate a predictable random set of meshes
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		int MeshRandomizationSeed = 0;

	/**
	* Determines whether static meshes or spline meshes will be created along the spline.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool UseStaticMeshes = false;

	/**
	* Allow spline mesh to deform to fit into the spline. When UseStaticMeshes is true, this will automatically switch to false
	* Far more performant when disabled, but less flexible. Segments may be stretched and scaled but can not be deformed or twisted.
	* Proper placement of pivots in source mesh becomes essential. Some other advanced features may not work as well.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "!UseStaticMeshes"))
		bool DeformSegments = true;

	/**
	* Overrides material index 0 for each spline mesh segment. Mostly used for blockouts when placeholder mesh assets dont have materials built in.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		UMaterialInterface* Material = nullptr;

	/**
	* Options to stretch the layer
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FSMEMeshDistribution MeshDistribution;

	/**
	* Options to offset the Segments
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FSMELocalOffsets Offsets;

	/**
	* Options to twist the spline
	* This values overrides any twist set by adjusting spline point roll manually
	* Do nothing when Use Static Meshes is set to true
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "!UseStaticMeshes"))
		FSMESplineTwistData SplineTwistData;

	/**
	* If the pivot point is at the base of the mesh, use this option to make it centered (half of the segment length)
	* Only usefull when using StaticMeshes
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "UseStaticMeshes == true"))
		TEnumAsByte <ESMECenteredPivotMode> CenteredPivotCorrection = ESMECenteredPivotMode::Down;

	/**
	* This negates only the initial Pitch rotation inherited from the spline.
	* All other rotations (offset, imperfection, etc) are still respected.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "UseStaticMeshes == true"))
		bool ApplyPitchFromSpline = true;

	/**
	* This negates only the initial Yaw rotation inherited from the spline.
	* All other rotations (offset, imperfection, etc) are still respected.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "UseStaticMeshes == true"))
		bool ApplyYawFromSpline = true;

	/**
	* This negates only the initial Roll rotation inherited from the spline.
	* All other rotations (offset, imperfection, etc) are still respected.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool ApplyRollFromSpline = true;
};

UENUM(BlueprintType)
enum ESMESegmentOverrideMode
{
	Arbitrary     UMETA(DisplayName = "Arbitrary"),
	Interval      UMETA(DisplayName = "Interval"),
	Last		  UMETA(DisplayName = "Last"),
};

USTRUCT(BlueprintType)
struct FSMESegmentOverrides
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	* The default orientation of the overridden segment.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis = ESplineMeshAxis::Type::X;

	/**
	* The alternate mesh that will be used only for the overridden segment
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "!UseDefaultMesh"))
		UStaticMesh* Mesh = nullptr;

	/**
	* Forces the overridden segment to use the mesh at index 0 of the Meshes array
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool UseDefaultMesh = true;

	/**
	* Read only, IsStaticMesh is true when UseStaticMesh is true on the layer level
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "SplineMesh")
		bool IsStaticMesh = false;

	/**
	*  Overrides global Deform Segments setting for the overridden segment. Disabled when UseStaticMesh = true
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "!IsStaticMesh"))
		bool Deform = true;

	/**
	*  The length of the overridden segment
	* Can't be modify if UseDefaultLength is enable
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "1", EditCondition = "!UseDefaultLength"))
		float SegmentLength = 1.0f;

	/**
	* Forces the overridden segment to use the same segment length as non overridden segments.
	* Can't be unchecked if Distribution mode SegmentsPerSplinePoint is enable.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool UseDefaultLength = true;

	/**
	* Select override mode
	* Arbitrary: Places the selected override mesh at SegmentIndex segment
	* Interval: Places the selected override mesh at every Interval segment
	* Last: Places the selected override mesh at the last segment
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte<ESMESegmentOverrideMode> SegmentOverrideMode = ESMESegmentOverrideMode::Arbitrary;

	/**
	* Defines the segment number that will be overridden. As with all arrays, the first segment is index 0.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "0", EditCondition = "SegmentOverrideMode == ESMESegmentOverrideMode::Arbitrary"))
		int SegmentIndex = 0;

	/**
	* Specify the segment where the interval should start
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "0", EditCondition = "SegmentOverrideMode == ESMESegmentOverrideMode::Interval"))
		int StartIntervalIndex = 0;

	/**
	* Places the selected override mesh at every Interval segment
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (ClampMin = "1", EditCondition = "SegmentOverrideMode == ESMESegmentOverrideMode::Interval"))
		int Interval = 2;

	/**
	* The amount of space at the start of the overridden segment in addition to any Gap Length before the overridden segment
	* May be negative to reduce any gap or in order to cause overlap with the previous segment.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		float StartOffset = 0.0f;

	/**
	* The amount of space at the end of the overridden segment in addition to any Gap Length before the overridden segment
	* May be negative to reduce any gap or in order to cause overlap with the next segment.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		float EndOffset = 0.0f;

	/**
	* Local location offset along the spline of the overridden segment.
	* Unlike Segment Start and End Offsets changing Forward Offset will not adjust the positions and gaps of the previous and next segments.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FVector LocationOffset = FVector::ZeroVector;

	/**
	* Local rotation of the overridden segment in degrees
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FRotator RotationOffset = FRotator::ZeroRotator;

	/**
	* Allow to rotate when the spline points are rolling. Work only if deform is true
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "Deform"))
		bool AllowSplineRoll = true;

	/**
	* Options to scale the Segments override segment
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FSMEScaleOverride ScaleData;

	/**
	* Use the value specified in the ScaleCurve of the layer
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool UseCurveScale = false;


};

USTRUCT(BlueprintType)
struct FSMEImperfectGapData
{
	GENERATED_USTRUCT_BODY()
public:

	//Allow to add imperfection in the gaps
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool EnableGapImperfection = false;

	//Generate randomly gap lenght value between -value and +value 
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "EnableGapImperfection == true"))
		float GapLengthRange = 0.0f;

	//Specify seeds to generate a predictable random gap length
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "EnableGapImperfection == true"))
		int GapLengthSeed = 0;

	//Specify if the random gap length can generate negative values
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh", meta = (EditCondition = "EnableGapImperfection == true"))
		bool AllowNegativeGapLength = false;

};

USTRUCT(BlueprintType)
struct FSMEImperfections
{
	GENERATED_USTRUCT_BODY()
public:

	//Add random Location, Rotation and Scale offsets
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool EnableImperfections = false;

	//Generate randomly Location values between -value and +value 
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FVector LocationRange = FVector::ZeroVector;

	//Specify seeds to generate a predictable random set of Location
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FIntVector LocationSeeds = FIntVector::ZeroValue;

	//Generate randomly rotation values between -value and +value 
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FRotator RotationRange = FRotator::ZeroRotator;

	//Specify seeds to generate a predictable random set of rotations
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FIntVector RotationSeeds = FIntVector::ZeroValue;

	//Generate randomly scale values between -value and +value 
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FVector ScaleRange = FVector(0.0f, 0.0f, 0.0f);

	//Specify seeds to generate a predictable random set of scales
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FIntVector ScaleSeeds = FIntVector::ZeroValue;

	//Keep all scale axis values equals to the random value generated by ScaleRange.X
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		bool UniformScale = false;

	//Data to add imperfections to gaps
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SplineMesh")
		FSMEImperfectGapData GapImperfectionData;

};

USTRUCT(BlueprintType)
struct FSMELayerData
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	* Defines the main mesh or meshes that will be used as segments of the spline mesh.
	* If more than one mesh is assigned to the array, each segment will be assigned at random from the available meshes.
	* Empty slots in the array are valid and can be a useful tool for creating large gaps at random between segments.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		TArray<UStaticMesh*> Meshes;

	/**
	* Draw the meshes
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool Visible = true;

	/**
	* Allow to only see the actor in the editor and not in game
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool HiddenInGame = false;

	/**
	* Determines along which axis the assigned meshes will be swept along the spline.
	* Because splines themselves consider the X axis to be the direction of travel along the spline
	* It is generally easiest if the mesh assets used are built “x forward” with their pivots centered but sometimes assets have differing standards.
	* This allows you to adjust for most pivot orientations.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis = ESplineMeshAxis::Type::X;

	/**
	* Options about the meshes to generate
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FSMEMeshesData MeshesData;


	/**
	* Enable / Disable simple collisions
	*/
	//UPROPERTY(BlueprintReadWrite, EditAnywhere)
	//	TEnumAsByte<ECollisionEnabled::Type> CollisionType = ECollisionEnabled::Type::NoCollision;

	/**
	* Allows for setting properties on a specific spline mesh segment.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		TArray<FSMESegmentOverrides> SegmentOverrides;

	/**
	* Add random offset range with randomizations controlled by seeds.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		FSMEImperfections Imperfections;

	/**
	* Allow to cast shadow (warring: this can affect the performances when UseStaticMeshes = false and when the meshes are moving).
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SplineMesh")
		bool CastShadow = false;

};

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPLINEMESHEDITOR_API USME_SplineLayer : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USME_SplineLayer();

	~USME_SplineLayer();

	//Initialization of the SplineLayer
	void Init(USplineComponent* SplineComp, bool ClosedLoop);

	void UpdateAtRuntime();

	//Return the cumulative length of all Segments together
	const float GetTotalSegmentsLength() { return TotalSegmentsLength; }

	// Mofify the cumulative length of all Segments together
	void SetTotalSegmentsLength(float newLength) { TotalSegmentsLength = newLength; }

	// Return the length of the mesh on the foward axe of the layer
	const float GetMeshForwardAxisLength(UStaticMesh* Mesh);

	const int GetNumSegments() { return SplineSegments.Num(); }

	TArray<USME_Segment*> GetSegments() { return SplineSegments; }

	const float GetGapLength() { return GapLength; }

	const int GetTotalNumSegments();

	const bool IsClosedLoop() { return bClosedLoop; }

	const float GetClampedSplineUtilization() { return ClampedSplineUtilization; }

	USplineMeshComponent* GetLastSplineMeshComp();

	UPROPERTY(EditDefaultsOnly, Category = "Default")
		USplineComponent* SplineComponent = nullptr;

	//Data from the editor about the SplineLayerComponent
	FSMELayerData LayerData;

	FRandomStream RandStreamMeshes;
	FRandomStream RandStreamLocationX;
	FRandomStream RandStreamLocationY;
	FRandomStream RandStreamLocationZ;
	FRandomStream RandStreamRotationX;
	FRandomStream RandStreamRotationY;
	FRandomStream RandStreamRotationZ;
	FRandomStream RandStreamScaleX;
	FRandomStream RandStreamScaleY;
	FRandomStream RandStreamScaleZ;
	FRandomStream RandStreamGapLength;

	FVector PreviousLateralOffset = FVector::ZeroVector;

	TEnumAsByte <EComponentMobility::Type> Mobility = EComponentMobility::Static;

	/** Segments of the layer*/
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Default")
	TArray<USME_Segment*> SplineSegments;

private:


	//Select a random mesh in the array of Meshes
	UStaticMesh* GetRandomMesh();

	//Add a new segment to the layer
	bool AddSegment(int SegmentIndex);

	bool UpdateSegment(int SegmentIndex, USME_Segment* SegmentToUpdate);

	//Return the lengh of a segment based on the size of the mesh on the forward axe selected
	float GetSegmentLengthByAxis(UStaticMesh* Mesh, int SegmentIndex);

	float ComputeNextSegmentLength(int SegmentIndex);

	float TotalSegmentsLength = 0;
	float PreviousSegmentLength = 1.0f;
	float GapLength = 0.0f;
	bool bClosedLoop = false;
	float CurrentSegmentLength = 0.0;
	UStaticMesh* CurrentMesh = nullptr;
	float ClampedSplineUtilization = 1.0f;
	float UnusedEndSplineDist = 0.0f;
	float MinUnusedEndSplineDist = 0.0f;
	float MinSplineUtilization = 0.0f;

};
