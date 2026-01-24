// Copyright 2022 Triple Scale Games. All Rights Reserved.

//#pragma optimize("", off)
#include "SME_SplineLayer.h"
#include "Engine/StaticMesh.h"

// Sets default values for this component's properties
USME_SplineLayer::USME_SplineLayer()
{

}

USME_SplineLayer::~USME_SplineLayer()
{
	for (int i = 0 ; i < SplineSegments.Num(); i++)
	{
		SplineSegments[i] = nullptr;
	}

	SplineSegments.Empty();
	CurrentMesh = nullptr;
	SplineComponent = nullptr;
}

void USME_SplineLayer::Init(USplineComponent* SplineComp, bool ClosedLoop)
{
	SplineComponent = SplineComp;
	bClosedLoop = ClosedLoop;
	SplineSegments.Empty();
	TotalSegmentsLength = 0.0f;

	RandStreamMeshes = FRandomStream(LayerData.MeshesData.MeshRandomizationSeed);
	RandStreamLocationX = FRandomStream(LayerData.Imperfections.LocationSeeds.X);
	RandStreamLocationY = FRandomStream(LayerData.Imperfections.LocationSeeds.Y);
	RandStreamLocationZ = FRandomStream(LayerData.Imperfections.LocationSeeds.Z);
	RandStreamRotationX = FRandomStream(LayerData.Imperfections.RotationSeeds.X);
	RandStreamRotationY = FRandomStream(LayerData.Imperfections.RotationSeeds.Y);
	RandStreamRotationZ = FRandomStream(LayerData.Imperfections.RotationSeeds.Z);
	RandStreamScaleX = FRandomStream(LayerData.Imperfections.ScaleSeeds.X);
	RandStreamScaleY = FRandomStream(LayerData.Imperfections.ScaleSeeds.Y);
	RandStreamScaleZ = FRandomStream(LayerData.Imperfections.ScaleSeeds.Z);
	RandStreamGapLength = FRandomStream(LayerData.Imperfections.GapImperfectionData.GapLengthSeed);

	if (LayerData.MeshesData.UseStaticMeshes)
	{
		LayerData.MeshesData.DeformSegments = false;
	}

	if (LayerData.Meshes.Num() > 0)
	{
		UnusedEndSplineDist = SplineComponent->GetSplineLength() - SplineComponent->GetSplineLength() * LayerData.MeshesData.MeshDistribution.SplineUtilization;
		ClampedSplineUtilization = LayerData.MeshesData.MeshDistribution.SplineUtilization;

		//Add 1st segment
		int CurrentSegmentIndex = 0;
		ComputeNextSegmentLength(0);
		AddSegment(CurrentSegmentIndex);

		bool ShouldContinueIteration = true;
		//Iterate to create the spline meshes on the segments. CurrentSegmentIndex + 1 is because we did the AddSegment(0) just before the loop
		while (ShouldContinueIteration && TotalSegmentsLength > 0.0f && TotalSegmentsLength <= SplineComponent->GetSplineLength() + 1 - ComputeNextSegmentLength(CurrentSegmentIndex + 1) - UnusedEndSplineDist)
		{
			CurrentSegmentIndex++;
			ShouldContinueIteration = AddSegment(CurrentSegmentIndex);
		}
	}
}

void USME_SplineLayer::UpdateAtRuntime()
{
	UnusedEndSplineDist = SplineComponent->GetSplineLength() - SplineComponent->GetSplineLength() * LayerData.MeshesData.MeshDistribution.SplineUtilization;
	ClampedSplineUtilization = LayerData.MeshesData.MeshDistribution.SplineUtilization;

	TotalSegmentsLength = 0.0f;
	for (int i = 0; i < SplineSegments.Num(); i++)
	{
		ComputeNextSegmentLength(i);
		UpdateSegment(i, SplineSegments[i]);
	}
}

float USME_SplineLayer::ComputeNextSegmentLength(int SegmentIndex)
{
	CurrentMesh = GetRandomMesh();

	if (LayerData.Imperfections.GapImperfectionData.EnableGapImperfection)
	{
		if (LayerData.Imperfections.GapImperfectionData.AllowNegativeGapLength)
		{
			GapLength = RandStreamGapLength.FRandRange(-1.0f * LayerData.Imperfections.GapImperfectionData.GapLengthRange, LayerData.Imperfections.GapImperfectionData.GapLengthRange);
		}
		else
		{
			GapLength = RandStreamGapLength.FRandRange(0.0f, LayerData.Imperfections.GapImperfectionData.GapLengthRange);
		}
		LayerData.MeshesData.MeshDistribution.GapLength = GapLength;
	}
	else
	{
		GapLength = LayerData.MeshesData.MeshDistribution.GapLength;
	}

	float MeshSize = GetMeshForwardAxisLength(CurrentMesh);
	float NumSegments = GetTotalNumSegments();
	if (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentLength || LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SemiFixedSegmentLength)
	{
		GapLength = FMath::Clamp(GapLength, -1 * LayerData.MeshesData.MeshDistribution.TargetSegmentLength + 1.0f, SplineComponent->GetSplineLength() - 1.0f);
	}
	else if (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentCount)
	{
		GapLength = FMath::Clamp(GapLength, -1 * MeshSize * NumSegments + 1.0f, SplineComponent->GetSplineLength() / (NumSegments - 1.0f));
	}
	else if (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SegmentsPerSplinePoint)
	{
		GapLength = FMath::Clamp(GapLength, -1 * SplineComponent->GetSplineLength() + 1.0f, SplineComponent->GetSplineLength() / (NumSegments)-1.0f);
	}

	CurrentSegmentLength = GetSegmentLengthByAxis(CurrentMesh, SegmentIndex);

	if (IsClosedLoop() && (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentCount || LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SemiFixedSegmentLength))
	{
		CurrentSegmentLength -= GapLength / (GetTotalNumSegments() - 1);
	}

	return CurrentSegmentLength;
}

bool USME_SplineLayer::AddSegment(int SegmentIndex)
{
	if (CurrentSegmentLength > 0.0f)
	{
		FString ComponentName = GetName().Append(FString::Printf(TEXT("SplineSegment%d"), SegmentIndex));
		USME_Segment* NewSplineSegement = NewObject<USME_Segment>(USME_Segment::StaticClass(), *ComponentName);
		if (IsValid(NewSplineSegement))
		{
			TotalSegmentsLength += CurrentSegmentLength;
			NewSplineSegement->Init(SegmentIndex, CurrentMesh, this, CurrentSegmentLength);
			TotalSegmentsLength += GapLength;
			TotalSegmentsLength += NewSplineSegement->GetOverrideEndOffset();
			SplineSegments.Add(NewSplineSegement);
			return true;
		}
	}
	return false;
}

bool USME_SplineLayer::UpdateSegment(int SegmentIndex, USME_Segment* SegmentToUpdate)
{
	if (CurrentSegmentLength > 0.0f)
	{
		if (IsValid(SegmentToUpdate))
		{
			TotalSegmentsLength += CurrentSegmentLength;
			SegmentToUpdate->Init(SegmentIndex, CurrentMesh, this, CurrentSegmentLength);
			TotalSegmentsLength += GapLength;
			TotalSegmentsLength += SegmentToUpdate->GetOverrideEndOffset();
		}
	}
	return false;
}

UStaticMesh* USME_SplineLayer::GetRandomMesh()
{
	UStaticMesh* NewMesh = nullptr;

	bool HasAtLeastOneValidMesh = false;
	for (int i = 0; i < LayerData.Meshes.Num(); i++)
	{
		if (LayerData.Meshes[i] != nullptr)
		{
			HasAtLeastOneValidMesh = true;
		}
	}

	if (HasAtLeastOneValidMesh)
	{
		int RandomMeshIndex = RandStreamMeshes.FRandRange(0, LayerData.Meshes.Num());
		//Avoid starting with an empty mesh
		if (SplineSegments.Num() == 0)
		{
			if (RandomMeshIndex < LayerData.Meshes.Num())
			{
				while (LayerData.Meshes[RandomMeshIndex] == nullptr)
				{
					RandomMeshIndex = FMath::RandRange(0, LayerData.Meshes.Num());
				}
			}
		}

		if (RandomMeshIndex < LayerData.Meshes.Num())
		{
			return LayerData.Meshes[RandomMeshIndex];
		}
	}

	return NewMesh;
}

float USME_SplineLayer::GetSegmentLengthByAxis(UStaticMesh* Mesh, int SegmentIndex)
{
	float SegmentLength = 1.0f;

	//Compute minimum value of SplineUtilization
	const int NbSegments = GetTotalNumSegments();
	float OverrideCustomLengths = 0.0f;
	int  NbCustomLengths = 0;
	for (int i = 0; i < LayerData.SegmentOverrides.Num(); i++)
	{
		float StartOffset;
		float EndOffset;
		if (LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Last)
		{
			StartOffset = FMath::Max(0.0f, LayerData.SegmentOverrides[i].StartOffset);
			EndOffset = FMath::Max(0.0f, LayerData.SegmentOverrides[i].EndOffset);
		}
		else
		{
			float NormalSegmentLength = LayerData.MeshesData.MeshDistribution.SegmentsLength;
			StartOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength * 2.0f + NormalSegmentLength), 0.0f), LayerData.SegmentOverrides[i].StartOffset);
			EndOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength), 0.0f), LayerData.SegmentOverrides[i].EndOffset);
		}

		float OverridedSegmentLength = LayerData.SegmentOverrides[i].SegmentLength;
		if (LayerData.SegmentOverrides[i].UseDefaultLength)
		{
			OverridedSegmentLength = LayerData.MeshesData.MeshDistribution.SegmentsLength;
		}
		OverrideCustomLengths += OverridedSegmentLength + StartOffset + EndOffset;
		NbCustomLengths++;
	}

	float LengthSegmentsNormal = float(NbSegments - 1 - NbCustomLengths) * SME_SplineLayer::MinMeshLength;
	float GapLengths = float(NbSegments - 1) * GapLength;
	MinUnusedEndSplineDist = FMath::Max(0.0f, SplineComponent->GetSplineLength() - LengthSegmentsNormal - OverrideCustomLengths - GapLengths);
	MinSplineUtilization = FMath::Min(1.0f, (SplineComponent->GetSplineLength() - MinUnusedEndSplineDist) / SplineComponent->GetSplineLength());


	if (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SegmentsPerSplinePoint)
	{
		int NumSegmentsPerSplinePoint = LayerData.MeshesData.MeshDistribution.NumSegmentsPerSplinePoint;
		if (NumSegmentsPerSplinePoint > 0)
		{
			float DistanceAlongSpline = SplineComponent->GetDistanceAlongSplineAtSplinePoint(SegmentIndex / NumSegmentsPerSplinePoint + 1);

			float PreviousDistanceAlongSpline = SplineComponent->GetDistanceAlongSplineAtSplinePoint(SegmentIndex / NumSegmentsPerSplinePoint);
			SegmentLength = (DistanceAlongSpline - PreviousDistanceAlongSpline) / NumSegmentsPerSplinePoint;
			SegmentLength -= GapLength;
		}

		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			SegmentLength = 0.0f;
		}

		if (SegmentIndex == 0)
		{
			LayerData.MeshesData.MeshDistribution.SegmentsLength = SegmentLength;
		}
	}
	else if (LayerData.MeshesData.MeshDistribution.DistributionMode != ESMEStretchMode::FixedSegmentLength)
	{
		float SegmentDivider = 0.0f;
		float SplineLengthAdjusted = SplineComponent->GetSplineLength() - UnusedEndSplineDist;
		SplineLengthAdjusted += GapLength;//add gap length so the last segment finish at the end of the spline

		if (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentCount)
		{
			SegmentDivider = LayerData.MeshesData.MeshDistribution.NumSegments;
		}
		else if (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SemiFixedSegmentLength)
		{
			SegmentDivider = int(SplineLengthAdjusted / (LayerData.MeshesData.MeshDistribution.TargetSegmentLength + GapLength));
		}

		if (SegmentDivider < 1)
		{
			SegmentDivider = 1;
		}

		SegmentLength = SplineLengthAdjusted / SegmentDivider - GapLength;

		if (SegmentLength <= SME_SplineLayer::MinMeshLength)
		{
			SegmentLength = SME_SplineLayer::MinMeshLength;
			UnusedEndSplineDist = MinUnusedEndSplineDist;
			ClampedSplineUtilization = (SplineComponent->GetSplineLength() - MinUnusedEndSplineDist) / SplineComponent->GetSplineLength();
		}
		LayerData.MeshesData.MeshDistribution.SegmentsLength = SegmentLength;
	}
	else if (Mesh != nullptr)//Fixed Segment Length
	{
		if (LayerData.MeshesData.MeshDistribution.UseDefaultMeshSize)
		{
			SegmentLength = GetMeshForwardAxisLength(Mesh);
		}
		else
		{
			SegmentLength = LayerData.MeshesData.MeshDistribution.SegmentsLength;
		}
		LayerData.MeshesData.MeshDistribution.SegmentsLength = SegmentLength;
	}
	else
	{
		SegmentLength = PreviousSegmentLength;
	}
	PreviousSegmentLength = SegmentLength;

	//Special case when the length of the meshes is not the same on all segments in FixedSegmentCount DistributionMode
	//The length of the overrided meshes should affect the length of the other meshes
	if (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentCount
		|| LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SemiFixedSegmentLength)
	{
		if (LayerData.SegmentOverrides.Num() > 0)
		{
			float CumulativeLengthOverrides = 0.0f;
			for (int i = 0; i < LayerData.SegmentOverrides.Num(); i++)
			{
				float StartOffset;
				float EndOffset;
				/*if (LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Last)
				{
					StartOffset = FMath::Max(0.0f, LayerData.SegmentOverrides[i].StartOffset);
					EndOffset = FMath::Max(0.0f, LayerData.SegmentOverrides[i].EndOffset);
				}
				else
				{*/
					float NormalSegmentLength = LayerData.MeshesData.MeshDistribution.SegmentsLength;
					StartOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength * 2.0f + NormalSegmentLength), 0.0f), LayerData.SegmentOverrides[i].StartOffset);
					EndOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength), 0.0f), LayerData.SegmentOverrides[i].EndOffset);
				//}

				if (LayerData.SegmentOverrides[i].UseDefaultLength)
				{
					CumulativeLengthOverrides += LayerData.MeshesData.MeshDistribution.SegmentsLength + StartOffset + EndOffset;
				}
				else
				{
					CumulativeLengthOverrides += LayerData.SegmentOverrides[i].SegmentLength + StartOffset + EndOffset;
				}
			}

			for (int i = 0; i < LayerData.SegmentOverrides.Num(); i++)
			{
				int NbInterval = 1;
				if (LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Interval)
				{
					NbInterval = FMath::DivideAndRoundUp(NbSegments, LayerData.SegmentOverrides[i].Interval);
				}

				float Divider = NbSegments - NbInterval;
				int NumSegmentsClosedLoop = NbSegments;
				if (!bClosedLoop)
				{
					NumSegmentsClosedLoop--;
				}

				float StartOffset;
				float EndOffset;
				/*if (LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Last)
				{
					StartOffset = FMath::Max(0.0f, LayerData.SegmentOverrides[i].StartOffset);
					EndOffset = FMath::Max(0.0f, LayerData.SegmentOverrides[i].EndOffset);
				}
				else
				{*/
					float NormalSegmentLength = LayerData.MeshesData.MeshDistribution.SegmentsLength;
					StartOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength * 2.0f + NormalSegmentLength), 0.0f), LayerData.SegmentOverrides[i].StartOffset);
					EndOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength), 0.0f), LayerData.SegmentOverrides[i].EndOffset);
				//}
				LayerData.SegmentOverrides[i].StartOffset = StartOffset;
				LayerData.SegmentOverrides[i].EndOffset = EndOffset;

				float OverridedSegmentLength = LayerData.SegmentOverrides[i].SegmentLength;
				if (LayerData.SegmentOverrides[i].UseDefaultLength)
				{
					OverridedSegmentLength = LayerData.MeshesData.MeshDistribution.SegmentsLength;
				}
				float AdjustedSegmentLength = 0.0f;
				if (LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Interval)
				{
					AdjustedSegmentLength = (SplineComponent->GetSplineLength() * FMath::Max(MinSplineUtilization, LayerData.MeshesData.MeshDistribution.SplineUtilization) - GetGapLength() * NumSegmentsClosedLoop - (OverridedSegmentLength + StartOffset + EndOffset) * float(NbInterval)) / Divider;
				}
				else
				{
					Divider = NbSegments - LayerData.SegmentOverrides.Num();
					AdjustedSegmentLength = (SplineComponent->GetSplineLength() * FMath::Max(MinSplineUtilization, LayerData.MeshesData.MeshDistribution.SplineUtilization) - GetGapLength() * NumSegmentsClosedLoop - CumulativeLengthOverrides) / Divider;
				}
				
				if (AdjustedSegmentLength < SME_SplineLayer::MinMeshLength)
				{
					AdjustedSegmentLength = SME_SplineLayer::MinMeshLength;

					//Clamp SplineUtilization
					UnusedEndSplineDist = MinUnusedEndSplineDist;
					ClampedSplineUtilization = MinSplineUtilization;

					//Clamp StartOffset
					float MaxStartOffset = SplineComponent->GetSplineLength() * FMath::Max(MinSplineUtilization, LayerData.MeshesData.MeshDistribution.SplineUtilization) - LengthSegmentsNormal - (OverridedSegmentLength + EndOffset) * float(NbInterval) - GapLengths;
					StartOffset = FMath::Max(0.0f, FMath::Min(StartOffset, MaxStartOffset));
					LayerData.SegmentOverrides[i].StartOffset = StartOffset;

					//Clamp EndOffset
					float MaxEndOffset = SplineComponent->GetSplineLength() * FMath::Max(MinSplineUtilization, LayerData.MeshesData.MeshDistribution.SplineUtilization) - LengthSegmentsNormal - (OverridedSegmentLength + StartOffset) * float(NbInterval) - GapLengths;
					EndOffset = FMath::Max(0.0f, FMath::Min(EndOffset, MaxEndOffset));
					LayerData.SegmentOverrides[i].EndOffset = EndOffset;

					//Clamp custom length of the overrided segment
					LayerData.SegmentOverrides[i].SegmentLength = (SplineComponent->GetSplineLength() * FMath::Max(MinSplineUtilization, LayerData.MeshesData.MeshDistribution.SplineUtilization) - GetGapLength() * NumSegmentsClosedLoop - AdjustedSegmentLength * Divider - (StartOffset + EndOffset) * float(NbInterval)) / float(NbInterval);
				}

				LayerData.MeshesData.MeshDistribution.SegmentsLength = AdjustedSegmentLength;
				SegmentLength = FMath::Min(OverridedSegmentLength, AdjustedSegmentLength);
			}
		}
		else if (SegmentLength < SME_SplineLayer::MinMeshLength)
		{
			UnusedEndSplineDist = MinUnusedEndSplineDist;
			ClampedSplineUtilization = MinSplineUtilization;
		}
	}

	return SegmentLength;
}

const float USME_SplineLayer::GetMeshForwardAxisLength(UStaticMesh* Mesh)
{
	float result = 1.0f;

	if (Mesh != nullptr)
	{
		double minX = Mesh->GetBoundingBox().Min.X;
		if (minX < 0) minX *= -1;
		double maxX = Mesh->GetBoundingBox().Max.X;
		if (maxX < 0) maxX *= -1;
		double minY = Mesh->GetBoundingBox().Min.Y;
		if (minY < 0) minY *= -1;
		double maxY = Mesh->GetBoundingBox().Max.Y;
		if (maxY < 0) maxY *= -1;
		double minZ = Mesh->GetBoundingBox().Min.Z;
		if (minZ < 0) minZ *= -1;
		double maxZ = Mesh->GetBoundingBox().Max.Z;
		if (maxZ < 0) maxZ *= -1;

		switch (LayerData.ForwardAxis)
		{
		case ESplineMeshAxis::Type::X:
			result = SplineComponent->GetComponentScale().X * (minX + maxX);
			break;

		case ESplineMeshAxis::Type::Y:
			result = SplineComponent->GetComponentScale().Y * (minY + maxY);
			break;

		case ESplineMeshAxis::Type::Z:
			result = SplineComponent->GetComponentScale().Z * (minZ + maxZ);
			break;

		default:
			break;
		}
	}
	else
	{
		result = PreviousSegmentLength;
	}
	return result;
}

const int USME_SplineLayer::GetTotalNumSegments()
{
	int NbTotalSegments = 0;
	if (LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SegmentsPerSplinePoint)
	{
		NbTotalSegments = (SplineComponent->GetNumberOfSplinePoints() - 1) * LayerData.MeshesData.MeshDistribution.NumSegmentsPerSplinePoint;
		if (bClosedLoop)
		{
			NbTotalSegments++;
		}
	}
	else if (LayerData.MeshesData.MeshDistribution.UseDefaultMeshSize ||
		LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentCount ||
		LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SemiFixedSegmentLength)
	{
		NbTotalSegments = LayerData.MeshesData.MeshDistribution.NumSegments;
	}
	else // FixedSegmentLength && !UseDefaultMeshSize
	{
		NbTotalSegments = FMath::DivideAndRoundDown(SplineComponent->GetSplineLength(), PreviousSegmentLength + GapLength);
	}

	return NbTotalSegments;
}

USplineMeshComponent* USME_SplineLayer::GetLastSplineMeshComp()
{
	if (SplineSegments.Num() != 0)
	{
		USME_Segment* LastSegment = SplineSegments.Last();
		if (IsValid(LastSegment))
		{
			return LastSegment->GetSplineMeshComponent();
		}
	}
	return nullptr;
}