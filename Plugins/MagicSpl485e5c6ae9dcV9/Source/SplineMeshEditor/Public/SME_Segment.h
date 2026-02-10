// Copyright 2022 Triple Scale Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Components/ActorComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SplineComponent.h"
#include "UObject/NoExportTypes.h"
#include "SME_Segment.generated.h"

class USME_SplineLayer;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPLINEMESHEDITOR_API USME_Segment : public UObject
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USME_Segment();
	~USME_Segment();

	void Init(int CurrentSegmentIndex, UStaticMesh* CurrentMesh, USME_SplineLayer* CurrentLayer, float CurrentSegmentLength);

	const float GetOverrideEndOffset() { return OverrideEndOffset; }

	USplineMeshComponent* GetSplineMeshComponent() { return NewSplineMeshComponent; }
	UStaticMeshComponent* GetStaticMeshComponent() { return NewStaticMeshComponent; }

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Segment")
		UStaticMeshComponent* NewStaticMeshComponent = nullptr;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Segment")
		USplineMeshComponent* NewSplineMeshComponent = nullptr;

private:

	//Apply scale data to the mesh in param
	void ScaleSegment(UStaticMeshComponent* StaticMeshComponent, bool IsUsingMeshOverride, float DistOnSpline);

	//Apply locations and rotations offsets to the mesh in param
	void AddOffsets(UStaticMeshComponent* StaticMeshComponent, bool IsUsingMeshOverride = false);

	//Apply twist to the the SplineMesh in param
	void Twist(USplineMeshComponent* SplineMeshComponent, bool IsUsingMeshOverride = false);

	//Apply SegmentsOverrides option to replace specified segments
	bool SegmentsOverrides();

	bool ApplySegmentsOverrides(UStaticMesh* OverrideMesh, int SegmentOverridesIndex);

	void SetSplineRollWithoutDeform(float Roll);

	void SetSplineRollWithDeform(float StartRoll, float EndRoll, float EndRollTwist);

	float SegmentLength = 0.0f;
	int SegmentIndex = 0;
	USME_SplineLayer* Layer = nullptr;
	FRotator RotationOffsetCumulative = FRotator::ZeroRotator;
	UStaticMesh* Mesh = nullptr;
	float OverrideScaleWidthStart = 1.0f;
	float OverrideScaleHeightStart = 1.0f;
	float OverrideScaleWidthEnd = 1.0f;
	float OverrideScaleHeightEnd = 1.0f;
	FRotator OverrideRotationOffset = FRotator::ZeroRotator;
	TEnumAsByte<ESplineMeshAxis::Type> ForwardAxisToUse = ESplineMeshAxis::Type::X;
	float OverrideEndOffset = 0.0f;
	FVector OverrideLocationOffset = FVector::ZeroVector;
	bool OverrideDeform = true;
	bool OverrideUseCurveScale = false;
	FVector LateralOffset = FVector::ZeroVector;
	float OverrideLengthDifference = 0.0f;
	FRotator SplineMeshLocalRotation = FRotator::ZeroRotator;
	float TwistAngleCumulativeStart = 0.0f;
	float TwistAngleCumulativeEnd = 0.0f;
	bool OverrideMatchScale = true;
};
