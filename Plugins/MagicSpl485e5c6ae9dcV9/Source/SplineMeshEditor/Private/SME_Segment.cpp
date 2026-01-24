// Copyright 2022 Triple Scale Games. All Rights Reserved.
//#pragma optimize("", off)

#include "SME_Segment.h"
#include "SME_SplineLayer.h"
#include "Curves/CurveFloat.h"


USME_Segment::USME_Segment()
{
}

USME_Segment::~USME_Segment()
{
	NewStaticMeshComponent = nullptr;
	NewSplineMeshComponent = nullptr;
	Layer = nullptr;
}

void USME_Segment::Init(int CurrentSegmentIndex, UStaticMesh* CurrentMesh, USME_SplineLayer* CurrentLayer, float CurrentSegmentLength)
{
	SegmentIndex = CurrentSegmentIndex;
	Mesh = CurrentMesh;
	Layer = CurrentLayer;
	SegmentLength = CurrentSegmentLength;
	ForwardAxisToUse = Layer->LayerData.ForwardAxis;

	bool IsUsingMeshOverride = SegmentsOverrides();
	float OffsetDistOnSpline = 0.0f;

	float GapLength = Layer->GetGapLength();

	if (Layer->LayerData.MeshesData.MeshDistribution.OnlyUseRemainingLength)
	{
		float CropedDist = (1.0f - Layer->LayerData.MeshesData.MeshDistribution.SplineUtilization) * (Layer->SplineComponent->GetSplineLength());
		OffsetDistOnSpline = Layer->LayerData.MeshesData.MeshDistribution.SegmentStartPercentage * CropedDist;
	}
	else
	{
		OffsetDistOnSpline = Layer->LayerData.MeshesData.MeshDistribution.SegmentStartPercentage * Layer->SplineComponent->GetSplineLength();
	}

	float DistOnSpline = FMath::Abs(Layer->GetTotalSegmentsLength() - SegmentLength + OverrideLocationOffset.X) + Layer->LayerData.MeshesData.Offsets.LocalLocationOffset.X + OffsetDistOnSpline;

	//Skip the next segment if we passed the end of the spline (SplineLength + 1.0f)
	if (DistOnSpline < 0.0f || DistOnSpline + SegmentLength > Layer->SplineComponent->GetSplineLength() + 1.0f)
	{
		return;
	}

	float LocationOffsetX = 0.0f;
	float ScaleOffsetX = 0.0f;
	if (Layer->LayerData.Imperfections.EnableImperfections)
	{
		FVector LocationRange = Layer->LayerData.Imperfections.LocationRange;
		LocationOffsetX += Layer->RandStreamLocationX.FRandRange(-1.0f * LocationRange.X, LocationRange.X);
		FVector ScaleRange = Layer->LayerData.Imperfections.ScaleRange;
		ScaleOffsetX = SegmentLength / 2.0f * Layer->RandStreamScaleX.FRandRange(-1.0f * ScaleRange.X, ScaleRange.X);
	}
	DistOnSpline += LocationOffsetX;

	if (Layer->LayerData.MeshesData.UseStaticMeshes)
	{
		float MeshLength = 0.0f;
		MeshLength = Layer->GetMeshForwardAxisLength(Mesh);

		float StretchedScale = (SegmentLength) / MeshLength;
		if (StretchedScale <= KINDA_SMALL_NUMBER)
		{
			StretchedScale = 0.001f;
		}

		if (Layer->LayerData.MeshesData.CenteredPivotCorrection == ESMECenteredPivotMode::Center)
		{
			//Extra offset to make the 1st mesh start of the begining of the spline
			DistOnSpline += SegmentLength / 2.0f;
		}
		else if (Layer->LayerData.MeshesData.CenteredPivotCorrection == ESMECenteredPivotMode::Down)
		{
			DistOnSpline += SegmentLength;
		}

		FTransform NewStaticMeshTransform = Layer->SplineComponent->GetTransformAtDistanceAlongSpline(DistOnSpline, ESplineCoordinateSpace::World, false);
		FRotator correctedRotation = FRotator::ZeroRotator;
		FVector localScale = NewStaticMeshTransform.GetScale3D();
		FVector StartPos = Layer->SplineComponent->GetLocationAtDistanceAlongSpline(DistOnSpline, ESplineCoordinateSpace::World);
		FVector EndPos = Layer->SplineComponent->GetLocationAtDistanceAlongSpline(DistOnSpline + SegmentLength, ESplineCoordinateSpace::World);
		FRotator NewStaticMeshRotation = FRotator(NewStaticMeshTransform.GetRotation());
		if (!Layer->LayerData.MeshesData.ApplyPitchFromSpline)
		{
			NewStaticMeshRotation.Pitch = 0;
		}
		if (!Layer->LayerData.MeshesData.ApplyYawFromSpline)
		{
			NewStaticMeshRotation.Yaw = 0;
		}
		if (!Layer->LayerData.MeshesData.ApplyRollFromSpline)
		{
			NewStaticMeshRotation.Roll = 0;
		}
		NewStaticMeshTransform.SetRotation(FQuat(NewStaticMeshRotation));

		switch (ForwardAxisToUse)
		{
		case ESplineMeshAxis::Type::X:
			localScale.X = StretchedScale;
			break;

		case ESplineMeshAxis::Type::Y:
			localScale.Y = StretchedScale;
			correctedRotation = FRotator(90.0f, 0.0f, 90.0f);
			break;

		case ESplineMeshAxis::Type::Z:
			localScale.Z = StretchedScale;
			correctedRotation = FRotator(0.0f, -90.0f, -90.0f);
			break;

		default:
			break;
		}

		NewStaticMeshTransform.SetScale3D(localScale);

		bool IsAlreadyConstructed = IsValid(NewStaticMeshComponent);
		FString ComponentName;

		if (!IsAlreadyConstructed)
		{
			ComponentName = Layer->GetName().Append(GetNameSafe(this).Append("StaticMesh"));
			//NewStaticMeshComponent = NewObject<UStaticMeshComponent>(Layer->SplineComponent, *ComponentName);
			FTransform RelativeMeshTransform = FTransform();
			NewStaticMeshComponent = Cast<UStaticMeshComponent>(Layer->SplineComponent->GetAttachmentRootActor()->AddComponentByClass(UStaticMeshComponent::StaticClass(), true, RelativeMeshTransform, false));
		}
		
		if (IsValid(NewStaticMeshComponent))
		{
			if (!IsAlreadyConstructed)
			{
				NewStaticMeshComponent->AppendName(ComponentName);
				NewStaticMeshComponent->AttachToComponent(Layer->SplineComponent, FAttachmentTransformRules::KeepWorldTransform);
				NewStaticMeshComponent->RegisterComponent();
				//NewStaticMeshComponent->SetMobility(Layer->LayerData.Mobility);
				NewStaticMeshComponent->SetStaticMesh(Mesh);
				//NewStaticMeshComponent->bCustomOverrideVertexColorPerLOD = 1;
				//NewStaticMeshComponent->CachePaintedDataIfNecessary();
			}
			NewStaticMeshComponent->SetWorldTransform(NewStaticMeshTransform);

			NewStaticMeshComponent->SetMaterial(0, Layer->LayerData.MeshesData.Material);
			//MeshComponent->SetCollisionEnabled(LayerData.CollisionType);

			//Offsets
			AddOffsets(NewStaticMeshComponent, IsUsingMeshOverride);

			//Scaling
			ScaleSegment(NewStaticMeshComponent, IsUsingMeshOverride, DistOnSpline);

			NewStaticMeshComponent->AddLocalRotation(correctedRotation);

			NewStaticMeshComponent->SetVisibleFlag(Layer->LayerData.Visible);
			NewStaticMeshComponent->SetHiddenInGame(Layer->LayerData.HiddenInGame);
			NewStaticMeshComponent->SetCastShadow(Layer->LayerData.CastShadow);
		}
	}
	else // if useStaticMesh is false, use SplineMesehes
	{
		bool IsAlreadyConstructed = IsValid(NewSplineMeshComponent);
		FString ComponentName;

		if (!IsAlreadyConstructed)
		{
			ComponentName = GetName().Append(FString::Printf(TEXT("SplineMesh")));
			//NewSplineMeshComponent = NewObject<USplineMeshComponent>(Layer->SplineComponent, *ComponentName);
			FTransform RelativeMeshTransform = FTransform();
			NewSplineMeshComponent = Cast<USplineMeshComponent>(Layer->SplineComponent->GetAttachmentRootActor()->AddComponentByClass(USplineMeshComponent::StaticClass(), true, RelativeMeshTransform, false));
			//NewSplineMeshComponent->bCustomOverrideVertexColorPerLOD = 1;
			//NewSplineMeshComponent->CachePaintedDataIfNecessary();
		}

		if (IsValid(NewSplineMeshComponent) && NewSplineMeshComponent != nullptr)
		{
			if (!IsAlreadyConstructed)
			{
				NewSplineMeshComponent->AppendName(ComponentName);
				//NewSplineMeshComponent->AttachToComponent(Layer->SplineComponent, FAttachmentTransformRules::KeepWorldTransform);
				NewSplineMeshComponent->RegisterComponent();
				NewSplineMeshComponent->SetMobility(Layer->Mobility);
				NewSplineMeshComponent->SetStaticMesh(Mesh);
			}

			NewSplineMeshComponent->SetMaterial(0, Layer->LayerData.MeshesData.Material);
			NewSplineMeshComponent->SetForwardAxis(ForwardAxisToUse, false);
			//NewSplineMeshComponent->SetCollisionEnabled(LayerData.CollisionType);

			FVector StartPos = Layer->SplineComponent->GetLocationAtDistanceAlongSpline(DistOnSpline - ScaleOffsetX, ESplineCoordinateSpace::Local);
			FVector EndPos = Layer->SplineComponent->GetLocationAtDistanceAlongSpline(DistOnSpline + SegmentLength + ScaleOffsetX, ESplineCoordinateSpace::Local);
			FVector StartTangent;
			FVector EndTangent;

			StartTangent = Layer->SplineComponent->GetTangentAtDistanceAlongSpline(DistOnSpline - ScaleOffsetX, ESplineCoordinateSpace::Local).GetClampedToSize(0.0f, SegmentLength);
			EndTangent = Layer->SplineComponent->GetTangentAtDistanceAlongSpline(DistOnSpline + SegmentLength + ScaleOffsetX, ESplineCoordinateSpace::Local).GetClampedToSize(0.0f, SegmentLength);

			FRotator RotationOffset = Layer->LayerData.MeshesData.Offsets.RotationOffset;
			if (Layer->LayerData.Imperfections.EnableImperfections)
			{
				FRotator RotationRange = Layer->LayerData.Imperfections.RotationRange;
				RotationOffset.Pitch += Layer->RandStreamRotationY.FRandRange(-1 * RotationRange.Pitch, RotationRange.Pitch);
				RotationOffset.Yaw += Layer->RandStreamRotationZ.FRandRange(-1 * RotationRange.Yaw, RotationRange.Yaw);
			}

			//Remove deform
			if ((!IsUsingMeshOverride && !Layer->LayerData.MeshesData.DeformSegments) || (IsUsingMeshOverride && !OverrideDeform))
			{
				float StartDistance = DistOnSpline;
				float EndDistance = Layer->GetTotalSegmentsLength() + OverrideLocationOffset.X;
				float MidDist = StartDistance + SegmentLength / 2.0f;
				FVector MidPointStart = Layer->SplineComponent->GetLocationAtDistanceAlongSpline(MidDist - ScaleOffsetX, ESplineCoordinateSpace::Local);
				FVector MidPointEnd = Layer->SplineComponent->GetLocationAtDistanceAlongSpline(MidDist + ScaleOffsetX, ESplineCoordinateSpace::Local);
				FVector MidTangentNomalizedStart = Layer->SplineComponent->GetTangentAtDistanceAlongSpline(MidDist - ScaleOffsetX, ESplineCoordinateSpace::Local);
				FVector MidTangentNomalizedEnd = Layer->SplineComponent->GetTangentAtDistanceAlongSpline(MidDist + ScaleOffsetX, ESplineCoordinateSpace::Local);
				MidTangentNomalizedStart.Normalize();
				MidTangentNomalizedEnd.Normalize();
				StartPos = MidPointStart - MidTangentNomalizedStart * SegmentLength / 2.0f;
				StartTangent = MidTangentNomalizedStart * SegmentLength;
				EndPos = MidTangentNomalizedEnd * (SegmentLength) / 2.0f + MidPointEnd;
				EndTangent = MidTangentNomalizedEnd * (SegmentLength);
			}

			if (RotationOffset.Pitch != 0.0f || RotationOffset.Yaw != 0.0f || OverrideRotationOffset.Pitch != 0.0f || OverrideRotationOffset.Yaw != 0.0f)
			{
				FVector MidPos = Layer->SplineComponent->GetLocationAtDistanceAlongSpline(DistOnSpline + SegmentLength / 2.0f + ScaleOffsetX, ESplineCoordinateSpace::Local);
				FVector dir = Layer->SplineComponent->GetDirectionAtDistanceAlongSpline(DistOnSpline + SegmentLength / 2.0f, ESplineCoordinateSpace::Local);
				dir.Normalize();
				FVector dirRot = dir.RotateAngleAxis(RotationOffset.Yaw + OverrideRotationOffset.Yaw, FVector::UpVector);
				FVector Axis = dir.RotateAngleAxis(91.0f, -FVector::UpVector);
				Axis.Normalize();
				dirRot = dirRot.RotateAngleAxis(RotationOffset.Pitch + OverrideRotationOffset.Pitch, Axis);

				if ((!IsUsingMeshOverride && !Layer->LayerData.MeshesData.DeformSegments) || (IsUsingMeshOverride && !OverrideDeform))
				{
					//when deform = false
					StartPos = MidPos + dirRot * SegmentLength / 2.0f;
					EndPos = MidPos - dirRot * SegmentLength / 2.0f;
					StartTangent = -dirRot * SegmentLength;
					EndTangent = -dirRot * SegmentLength;
				}
				else//when deform = true
				{
					//Position Start
					FVector dirStart = MidPos - StartPos;
					dirStart.Normalize();
					FVector dirRotStart = dirStart.RotateAngleAxis(RotationOffset.Yaw + OverrideRotationOffset.Yaw, FVector::UpVector);
					FVector AxisStart = dirStart.RotateAngleAxis(91.0f, -FVector::UpVector);
					AxisStart.Normalize();
					dirRotStart = dirRotStart.RotateAngleAxis(RotationOffset.Pitch + OverrideRotationOffset.Pitch, AxisStart);
					StartPos = MidPos - dirRotStart * SegmentLength / 2.0f;

					//Tangent Start
					FVector dirTangentStart = Layer->SplineComponent->GetDirectionAtDistanceAlongSpline(DistOnSpline - ScaleOffsetX, ESplineCoordinateSpace::Local);
					dirTangentStart.Normalize();
					FVector dirRotTangentStart = dirTangentStart.RotateAngleAxis(RotationOffset.Yaw + OverrideRotationOffset.Yaw, FVector::UpVector);
					FVector AxisStartTangent = dirTangentStart.RotateAngleAxis(91.0f, -FVector::UpVector);
					AxisStartTangent.Normalize();
					dirRotTangentStart = dirRotTangentStart.RotateAngleAxis(RotationOffset.Pitch + OverrideRotationOffset.Pitch, AxisStartTangent);
					StartTangent = dirRotTangentStart * SegmentLength;

					//Position End
					FVector dirEnd = MidPos - EndPos;
					dirEnd.Normalize();
					FVector dirRotEnd = dirEnd.RotateAngleAxis(RotationOffset.Yaw + OverrideRotationOffset.Yaw, FVector::UpVector);
					FVector AxisEnd = dirEnd.RotateAngleAxis(91.0f, FVector::UpVector);
					AxisEnd.Normalize();
					dirRotEnd = dirRotEnd.RotateAngleAxis(RotationOffset.Pitch + OverrideRotationOffset.Pitch, AxisEnd);
					EndPos = MidPos - dirRotEnd * SegmentLength / 2.0f;

					//Tangent End
					FVector dirTangentEnd = Layer->SplineComponent->GetDirectionAtDistanceAlongSpline(DistOnSpline + SegmentLength - ScaleOffsetX, ESplineCoordinateSpace::Local);
					dirTangentEnd.Normalize();
					FVector dirRotTangentEnd = dirTangentEnd.RotateAngleAxis(RotationOffset.Yaw + OverrideRotationOffset.Yaw, FVector::UpVector);
					FVector AxisEndTangent = dirTangentEnd.RotateAngleAxis(91.0f, FVector::UpVector);
					AxisEndTangent.Normalize();
					dirRotTangentEnd = dirRotTangentEnd.RotateAngleAxis(-1 * (RotationOffset.Pitch + OverrideRotationOffset.Pitch), AxisEndTangent);
					EndTangent = dirRotTangentEnd * SegmentLength;
				}
			}

			//Scaling
			ScaleSegment(NewSplineMeshComponent, IsUsingMeshOverride, DistOnSpline);

			//Offsets + Twist
			AddOffsets(NewSplineMeshComponent);

			//Manual Attachement
			//FRotator WorldRotationComp = Layer->SplineComponent->GetComponentToWorld().GetRotation().Rotator();
			FRotator WorldRotationComp = Layer->SplineComponent->GetComponentRotation();
			NewSplineMeshComponent->SetWorldRotation(WorldRotationComp);

			if (!Layer->LayerData.MeshesData.Offsets.AllowWorldOffsetRotation)
			{
				FVector WorldLocationOffset = Layer->LayerData.MeshesData.Offsets.WorldLocationOffset;

				//FVector WorldLocationComp = Layer->SplineComponent->GetComponentToWorld().GetLocation();
				FVector WorldLocationComp = Layer->SplineComponent->GetComponentLocation() + WorldLocationOffset;
				WorldLocationComp = WorldRotationComp.UnrotateVector(WorldLocationComp);

				NewSplineMeshComponent->SetStartAndEnd(WorldLocationComp + StartPos, StartTangent, WorldLocationComp + EndPos, EndTangent);
			}

			FRotator PivotPointRotation = FRotator::ZeroRotator;
			FRotator StartPointRotation = FRotator::ZeroRotator;
			FRotator EndPointRotation = FRotator::ZeroRotator;

			if (Layer->LayerData.MeshesData.ApplyRollFromSpline)
			{
				//Add local Roll rotation after lateral offset + Twist + Spline Rotation
				PivotPointRotation = Layer->SplineComponent->GetRotationAtDistanceAlongSpline(DistOnSpline + SegmentLength / 2.0f, ESplineCoordinateSpace::Local);
				StartPointRotation = Layer->SplineComponent->GetRotationAtDistanceAlongSpline(DistOnSpline, ESplineCoordinateSpace::Local);
				EndPointRotation = Layer->SplineComponent->GetRotationAtDistanceAlongSpline(DistOnSpline + SegmentLength, ESplineCoordinateSpace::Local);
			}

			if (!Layer->LayerData.MeshesData.SplineTwistData.SplineRollTwist)
			{
				StartPointRotation = PivotPointRotation;
				EndPointRotation = PivotPointRotation;
			}

			if (Layer->LayerData.MeshesData.Offsets.AllowWorldOffsetRotation)
			{
				FVector WorldLocationOffset = Layer->LayerData.MeshesData.Offsets.WorldLocationOffset;
				FVector ForwardVectorPivot = Layer->SplineComponent->GetDirectionAtDistanceAlongSpline(DistOnSpline + SegmentLength / 2.0f, ESplineCoordinateSpace::Local);
				FVector ForwardVectorStart = Layer->SplineComponent->GetDirectionAtDistanceAlongSpline(DistOnSpline, ESplineCoordinateSpace::Local);
				FVector ForwardVectorEnd = Layer->SplineComponent->GetDirectionAtDistanceAlongSpline(DistOnSpline + SegmentLength, ESplineCoordinateSpace::Local);

				if (!Layer->LayerData.MeshesData.SplineTwistData.SplineRollTwist)
				{
					ForwardVectorStart = ForwardVectorPivot;
					ForwardVectorEnd = ForwardVectorPivot;
				}

				FVector WorldLocationOffsetStart = WorldLocationOffset.RotateAngleAxis(-StartPointRotation.Roll, ForwardVectorStart);
				FVector WorldLocationOffsetEnd = WorldLocationOffset.RotateAngleAxis(-EndPointRotation.Roll, ForwardVectorEnd);

				FVector WorldLocationCompStart = Layer->SplineComponent->GetComponentLocation() + WorldLocationOffsetStart;
				WorldLocationCompStart = WorldRotationComp.UnrotateVector(WorldLocationCompStart);

				FVector WorldLocationCompEnd = Layer->SplineComponent->GetComponentLocation() + WorldLocationOffsetEnd;
				WorldLocationCompEnd = WorldRotationComp.UnrotateVector(WorldLocationCompEnd);

				NewSplineMeshComponent->SetStartAndEnd(WorldLocationCompStart + StartPos, StartTangent, WorldLocationCompEnd + EndPos, EndTangent);
			}

			float RollToApply = FMath::DegreesToRadians(EndPointRotation.Roll + SplineMeshLocalRotation.Roll);
			if (!IsUsingMeshOverride)
			{
				if (!Layer->LayerData.MeshesData.DeformSegments)
				{
					SetSplineRollWithoutDeform(FMath::DegreesToRadians(PivotPointRotation.Roll + SplineMeshLocalRotation.Roll) + TwistAngleCumulativeStart);
				}
				else
				{
					SetSplineRollWithDeform(FMath::DegreesToRadians(StartPointRotation.Roll + SplineMeshLocalRotation.Roll) + TwistAngleCumulativeStart,
						RollToApply + TwistAngleCumulativeStart,
						RollToApply + TwistAngleCumulativeEnd);
				}
			}
			else
			{
				for (int i = 0; i < Layer->LayerData.SegmentOverrides.Num(); i++)
				{
					if (!Layer->LayerData.SegmentOverrides[i].Deform || !Layer->LayerData.SegmentOverrides[i].AllowSplineRoll)
					{
						SetSplineRollWithoutDeform(FMath::DegreesToRadians(PivotPointRotation.Roll + SplineMeshLocalRotation.Roll) + TwistAngleCumulativeStart);
					}
					else
					{
						SetSplineRollWithDeform(FMath::DegreesToRadians(StartPointRotation.Roll + SplineMeshLocalRotation.Roll) + TwistAngleCumulativeStart,
							RollToApply + TwistAngleCumulativeStart,
							RollToApply + TwistAngleCumulativeEnd);
					}
				}
			}

			NewSplineMeshComponent->SetVisibleFlag(Layer->LayerData.Visible);
			NewSplineMeshComponent->SetHiddenInGame(Layer->LayerData.HiddenInGame);
			NewSplineMeshComponent->SetCastShadow(Layer->LayerData.CastShadow);
		}
	}
}

void USME_Segment::SetSplineRollWithoutDeform(float Roll)
{
	NewSplineMeshComponent->SetStartRoll(Roll);
	NewSplineMeshComponent->SetEndRoll(Roll);
}

void USME_Segment::SetSplineRollWithDeform(float StartRoll, float EndRoll, float EndRollTwist)
{
	NewSplineMeshComponent->SetStartRoll(StartRoll);
	if (Layer->LayerData.MeshesData.SplineTwistData.SeamlessTwist)
	{
		NewSplineMeshComponent->SetEndRoll(EndRollTwist);
	}
	else
	{
		NewSplineMeshComponent->SetEndRoll(EndRoll);
	}
}


void USME_Segment::ScaleSegment(UStaticMeshComponent* MeshComponent, bool IsUsingMeshOverride, float DistOnSpline)
{
	//If no Scale Curve is defined, only the Max values are considered 
	float ScaleWidthStart = Layer->LayerData.MeshesData.Offsets.ScaleData.MaxScaleWidth;
	float ScaleWidthEnd = Layer->LayerData.MeshesData.Offsets.ScaleData.MaxScaleWidth;
	float ScaleHeightStart = Layer->LayerData.MeshesData.Offsets.ScaleData.MaxScaleHeight;
	float ScaleHeightEnd = Layer->LayerData.MeshesData.Offsets.ScaleData.MaxScaleHeight;
	float ImperfectionScaleX = 0.0f;
	float ImperfectionScaleY = 0.0f;
	float ImperfectionScaleZ = 0.0f;

	if (IsUsingMeshOverride && !OverrideMatchScale)
	{
		ScaleWidthStart = OverrideScaleWidthStart;
		ScaleHeightStart = OverrideScaleHeightStart;
		ScaleWidthEnd = OverrideScaleWidthEnd;
		ScaleHeightEnd = OverrideScaleHeightEnd;
	}

	if (!IsValid(Layer->LayerData.MeshesData.Offsets.ScaleData.ScaleCurve)) {
		const FVector StartScale = Layer->SplineComponent->GetScaleAtDistanceAlongSpline(DistOnSpline);
		const FVector EndScale = Layer->SplineComponent->GetScaleAtDistanceAlongSpline(DistOnSpline + SegmentLength);
		ScaleWidthStart *= StartScale.Y;
		ScaleHeightStart *= StartScale.Z;
		ScaleWidthEnd *= EndScale.Y;
		ScaleHeightEnd *= EndScale.Z;
	}

	if (IsValid(Layer->LayerData.MeshesData.Offsets.ScaleData.ScaleCurve) && (!IsUsingMeshOverride || OverrideUseCurveScale))
	{
		float SplineLength = Layer->GetClampedSplineUtilization() * Layer->SplineComponent->GetSplineLength();
		float TimeOnCurveStart = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, SplineLength), FVector2D(0.0f, 1.0f), Layer->GetTotalSegmentsLength() + OverrideLengthDifference - SegmentLength);
		float CurveValueStart = Layer->LayerData.MeshesData.Offsets.ScaleData.ScaleCurve->GetFloatValue(TimeOnCurveStart);
		if (CurveValueStart <= KINDA_SMALL_NUMBER)
		{
			CurveValueStart = 0.0001f;
		}
		float TimeOnCurveEnd = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, SplineLength), FVector2D(0.0f, 1.0f), Layer->GetTotalSegmentsLength() + OverrideLengthDifference);
		float CurveValueEnd = Layer->LayerData.MeshesData.Offsets.ScaleData.ScaleCurve->GetFloatValue(TimeOnCurveEnd);
		if (CurveValueEnd <= KINDA_SMALL_NUMBER)
		{
			CurveValueEnd = 0.0001f;
		}

		ScaleWidthStart = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(Layer->LayerData.MeshesData.Offsets.ScaleData.MinScaleWidth, Layer->LayerData.MeshesData.Offsets.ScaleData.MaxScaleWidth), CurveValueStart) + OverrideScaleWidthStart - 1.0f;
		ScaleHeightStart = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(Layer->LayerData.MeshesData.Offsets.ScaleData.MinScaleHeight, Layer->LayerData.MeshesData.Offsets.ScaleData.MaxScaleHeight), CurveValueStart) + OverrideScaleHeightStart - 1.0f;
		ScaleWidthEnd = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(Layer->LayerData.MeshesData.Offsets.ScaleData.MinScaleWidth, Layer->LayerData.MeshesData.Offsets.ScaleData.MaxScaleWidth), CurveValueEnd) + OverrideScaleWidthEnd - 1.0f;
		ScaleHeightEnd = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(Layer->LayerData.MeshesData.Offsets.ScaleData.MinScaleHeight, Layer->LayerData.MeshesData.Offsets.ScaleData.MaxScaleHeight), CurveValueEnd) + OverrideScaleHeightEnd - 1.0f;
	}

	if (ScaleWidthStart <= KINDA_SMALL_NUMBER)
	{
		ScaleWidthStart = 0.001f;
	}
	if (ScaleHeightStart <= KINDA_SMALL_NUMBER)
	{
		ScaleHeightStart = 0.001f;
	}
	if (ScaleWidthEnd <= KINDA_SMALL_NUMBER)
	{
		ScaleWidthEnd = 0.001f;
	}
	if (ScaleHeightEnd <= KINDA_SMALL_NUMBER)
	{
		ScaleHeightEnd = 0.001f;
	}

	if (Layer->LayerData.Imperfections.EnableImperfections)
	{
		FVector ScaleRange = Layer->LayerData.Imperfections.ScaleRange;
		ImperfectionScaleX = Layer->RandStreamScaleX.FRandRange(0.0f, ScaleRange.X);
		if (Layer->LayerData.Imperfections.UniformScale)
		{
			ImperfectionScaleY = ImperfectionScaleX;
			ImperfectionScaleZ = ImperfectionScaleX;
		}
		else
		{
			ImperfectionScaleY = Layer->RandStreamScaleY.FRandRange(0.0f, ScaleRange.Y);
			ImperfectionScaleZ = Layer->RandStreamScaleZ.FRandRange(0.0f, ScaleRange.Z);
		}
	}

	USplineMeshComponent* SplineMeshComponent = Cast<USplineMeshComponent>(MeshComponent);
	if (IsValid(SplineMeshComponent))
	{
		SplineMeshComponent->SetStartScale(FVector2D(ScaleWidthStart + ImperfectionScaleY, ScaleHeightStart + ImperfectionScaleZ), false);
		if ((!IsUsingMeshOverride && Layer->LayerData.MeshesData.DeformSegments) || (IsUsingMeshOverride && OverrideDeform))
		{
			SplineMeshComponent->SetEndScale(FVector2D(ScaleWidthEnd + ImperfectionScaleY, ScaleHeightEnd + ImperfectionScaleZ), false);
		}
		else
		{
			SplineMeshComponent->SetEndScale(FVector2D(ScaleWidthStart + ImperfectionScaleY, ScaleHeightStart + ImperfectionScaleZ), false);
		}
	}
	else if (IsValid(MeshComponent))//StaticMeshes
	{
		float ExtraLength = Layer->LayerData.MeshesData.Offsets.ScaleData.ExtraLength;
		switch (ForwardAxisToUse)
		{
		case ESplineMeshAxis::Type::X:
			MeshComponent->SetWorldScale3D(FVector(MeshComponent->GetRelativeScale3D().X + ExtraLength + ImperfectionScaleX, ScaleWidthStart + ImperfectionScaleY, ScaleHeightStart + ImperfectionScaleZ));
			break;

		case ESplineMeshAxis::Type::Y:
			MeshComponent->SetWorldScale3D(FVector(ScaleHeightStart + ImperfectionScaleX, MeshComponent->GetRelativeScale3D().Y + ExtraLength + ImperfectionScaleY, ScaleWidthStart + ImperfectionScaleY));
			break;

		case ESplineMeshAxis::Type::Z:
			MeshComponent->SetWorldScale3D(FVector(ScaleWidthStart + ImperfectionScaleY, ScaleHeightStart + ImperfectionScaleZ, MeshComponent->GetRelativeScale3D().Z + ExtraLength + ImperfectionScaleX));
			break;

		default:
			break;
		}
	}
}

void USME_Segment::AddOffsets(UStaticMeshComponent* StaticMeshComponent, bool IsUsingMeshOverride)
{
	FVector LocalLocationOffset = Layer->LayerData.MeshesData.Offsets.LocalLocationOffset;
	FVector WorldLocationOffset = Layer->LayerData.MeshesData.Offsets.WorldLocationOffset;
	
	FRotator RotationOffset = Layer->LayerData.MeshesData.Offsets.RotationOffset;

	if (Layer->LayerData.MeshesData.Offsets.CumulativeSegmentRotationX)
	{
		RotationOffsetCumulative.Roll = RotationOffset.Roll * SegmentIndex;
	}
	if (Layer->LayerData.MeshesData.Offsets.CumulativeSegmentRotationY)
	{
		RotationOffsetCumulative.Pitch = RotationOffset.Pitch * SegmentIndex;
	}
	if (Layer->LayerData.MeshesData.Offsets.CumulativeSegmentRotationZ)
	{
		RotationOffsetCumulative.Yaw = RotationOffset.Yaw * SegmentIndex;
	}
	RotationOffset += RotationOffsetCumulative;

	if (Layer->LayerData.Imperfections.EnableImperfections)
	{
		FVector LocationRange = Layer->LayerData.Imperfections.LocationRange;
		LocalLocationOffset.X = 0.0f;
		LocalLocationOffset.Y += Layer->RandStreamLocationY.FRandRange(-1.0f * LocationRange.Y, LocationRange.Y);
		LocalLocationOffset.Z += Layer->RandStreamLocationZ.FRandRange(-1.0f * LocationRange.Z, LocationRange.Z);

		FRotator RotationRange = Layer->LayerData.Imperfections.RotationRange;
		RotationOffset.Roll += Layer->RandStreamRotationX.FRandRange(-1 * RotationRange.Roll, RotationRange.Roll);
		RotationOffset.Pitch += Layer->RandStreamRotationY.FRandRange(-1 * RotationRange.Pitch, RotationRange.Pitch);
		RotationOffset.Yaw += Layer->RandStreamRotationZ.FRandRange(-1 * RotationRange.Yaw, RotationRange.Yaw);
	}

	USplineMeshComponent* SplineMeshComponent = Cast<USplineMeshComponent>(StaticMeshComponent);
	FRotator SplineRotation = Layer->SplineComponent->GetRotationAtDistanceAlongSpline(Layer->GetTotalSegmentsLength() - SegmentLength / 2.0f + OverrideLocationOffset.X, ESplineCoordinateSpace::Local);
	FVector ForwardVector = Layer->SplineComponent->GetDirectionAtDistanceAlongSpline(Layer->GetTotalSegmentsLength() - SegmentLength / 2.0f + OverrideLocationOffset.X, ESplineCoordinateSpace::Local);
	FVector RotatedSplineDirectionY = FRotationMatrix(FRotator(0.0f, 90.0f, 0.0f)).TransformVector(ForwardVector);
	FVector RotatedSplineDirectionZ = FRotationMatrix(FRotator(0.0f, 0.0f, 90.0f)).TransformVector(ForwardVector);
	ForwardVector.Normalize();

	if (IsValid(SplineMeshComponent))
	{
		//Latteral Offset
		FVector DirectionY = FVector::CrossProduct(FVector::UpVector, ForwardVector);
		DirectionY.Normalize();
		LateralOffset = DirectionY * FVector(LocalLocationOffset.Y + OverrideLocationOffset.Y, LocalLocationOffset.Y + OverrideLocationOffset.Y, 0.0f);
		if (((LateralOffset.Y > 0 && Layer->PreviousLateralOffset.Y < 0) || (LateralOffset.Y < 0 && Layer->PreviousLateralOffset.Y > 0)))
		{
			LateralOffset *= -1;
		}

		FVector DirectionZ = FVector::CrossProduct(ForwardVector, FVector::RightVector);
		DirectionZ.Normalize();
		LateralOffset += DirectionZ * FVector(LocalLocationOffset.Z + OverrideLocationOffset.Z, 0.0f, LocalLocationOffset.Z + OverrideLocationOffset.Z);
		if (((LateralOffset.Z > 0 && Layer->PreviousLateralOffset.Z < 0) || (LateralOffset.Z < 0 && Layer->PreviousLateralOffset.Z > 0)))
		{
			LateralOffset *= -1;
		}

		Layer->PreviousLateralOffset = LateralOffset;

		//Save the rotation offset for after the translations
		SplineMeshLocalRotation = RotationOffset + OverrideRotationOffset;

		//Add Twist
		Twist(NewSplineMeshComponent, IsUsingMeshOverride);

		LateralOffset = LateralOffset.RotateAngleAxis(-SplineRotation.Roll, ForwardVector);

		SplineMeshComponent->AddLocalOffset(LateralOffset);
		//SplineMeshComponent->AddWorldOffset(WorldLocationOffset);
	}
	else if (IsValid(StaticMeshComponent))//Static Meshes
	{
		//Add Translations Offset
		FVector OverrideOffset = OverrideLocationOffset;
		OverrideOffset.X = 0.0f;
		LocalLocationOffset.X = 0.0f;
		StaticMeshComponent->AddLocalOffset(OverrideOffset);
		StaticMeshComponent->AddLocalOffset(LocalLocationOffset);
		//StaticMeshComponent->AddWorldOffset(WorldLocationOffset);

		//Add the rotations
		StaticMeshComponent->AddLocalRotation(RotationOffset);
		StaticMeshComponent->AddLocalRotation(OverrideRotationOffset);

		if (Layer->LayerData.MeshesData.Offsets.AllowWorldOffsetRotation)
		{
			WorldLocationOffset = WorldLocationOffset.RotateAngleAxis(-SplineRotation.Roll, ForwardVector);
		}
		StaticMeshComponent->AddWorldOffset(WorldLocationOffset);
	}
}

void USME_Segment::Twist(USplineMeshComponent* SplineMeshComponent, bool IsUsingMeshOverride)
{
	float TwistAngle = FMath::DegreesToRadians(Layer->LayerData.MeshesData.SplineTwistData.SplineTwistDegrees);
	float GapLength = Layer->GetGapLength();
	
	if (Layer->LayerData.MeshesData.SplineTwistData.CumualativeSegmentTwist)
	{
		TwistAngle *= Layer->GetTotalNumSegments();
	}
	if (IsValid(Layer->LayerData.MeshesData.SplineTwistData.TwistCurve))
	{
		float TimeOnCurveStart = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, Layer->GetClampedSplineUtilization() * Layer->SplineComponent->GetSplineLength()), FVector2D(0.0f, 1.0f), Layer->GetTotalSegmentsLength());
		float TimeOnCurveEnd = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, Layer->GetClampedSplineUtilization() * Layer->SplineComponent->GetSplineLength()), FVector2D(0.0f, 1.0f), Layer->GetTotalSegmentsLength() + SegmentLength + GapLength);
		float CurveValueStart = Layer->LayerData.MeshesData.SplineTwistData.TwistCurve->GetFloatValue(TimeOnCurveStart);
		float CurveValueEnd = Layer->LayerData.MeshesData.SplineTwistData.TwistCurve->GetFloatValue(TimeOnCurveEnd);
		if (CurveValueStart <= KINDA_SMALL_NUMBER)
		{
			CurveValueStart = 0.0f;
		}
		if (CurveValueEnd <= KINDA_SMALL_NUMBER)
		{
			CurveValueEnd = 0.0f;
		}
		TwistAngleCumulativeStart = TwistAngle * CurveValueStart;
		TwistAngleCumulativeEnd = TwistAngle * CurveValueEnd;
	}
	else
	{
		float TotalSegmentLength = Layer->GetTotalSegmentsLength();
		float SplineLength = Layer->SplineComponent->GetSplineLength();
		UE_LOG(LogTemp, Warning, TEXT("TotalSegmentLength = %f"), TotalSegmentLength);
		UE_LOG(LogTemp, Warning, TEXT("SplineLength: %f"), SplineLength);
		TwistAngleCumulativeStart += TwistAngle * (Layer->GetTotalSegmentsLength() - SegmentLength) / Layer->SplineComponent->GetSplineLength();
		TwistAngleCumulativeEnd += TwistAngle * (Layer->GetTotalSegmentsLength()) / (Layer->SplineComponent->GetSplineLength());
	}
}

bool USME_Segment::SegmentsOverrides()
{
	OverrideEndOffset = 0.0f;
	OverrideLocationOffset = FVector::ZeroVector;
	OverrideDeform = false;
	OverrideUseCurveScale = false;
	bool OverrrideOccured = false;

	if (SegmentLength > 0)
	{
		int NbTotalSegments = Layer->GetTotalNumSegments();
		bool IsLastSegment = SegmentIndex >= NbTotalSegments - 1;

		float GapLength = Layer->GetGapLength();
		bool ResultCurrentIteration = false;
		float OffsetDistOnSpline = 0.0f;
		for (int i = 0; i < Layer->LayerData.SegmentOverrides.Num(); i++)
		{
			UStaticMesh* OverrideMesh = nullptr;
			if (Layer->LayerData.SegmentOverrides[i].UseDefaultMesh && Layer->LayerData.Meshes.Num() > 0)
			{
				OverrideMesh = Layer->LayerData.Meshes[0];
			}
			else
			{
				OverrideMesh = Layer->LayerData.SegmentOverrides[i].Mesh;
			}

			switch (Layer->LayerData.SegmentOverrides[i].SegmentOverrideMode)
			{
			case ESMESegmentOverrideMode::Arbitrary:
				if (Layer->LayerData.SegmentOverrides[i].SegmentIndex == SegmentIndex)
				{
					ResultCurrentIteration = ApplySegmentsOverrides(OverrideMesh, i);
				}
				break;

			case ESMESegmentOverrideMode::Interval:
				if (SegmentIndex == 0)
				{
					if (Layer->LayerData.SegmentOverrides[i].StartIntervalIndex == 0)
					{
						ResultCurrentIteration = ApplySegmentsOverrides(OverrideMesh, i);
					}
				}
				else
				{
					int StartIndex = SegmentIndex - Layer->LayerData.SegmentOverrides[i].StartIntervalIndex;
					if (StartIndex >= 0 && StartIndex % Layer->LayerData.SegmentOverrides[i].Interval == 0)
					{
						ResultCurrentIteration = ApplySegmentsOverrides(OverrideMesh, i);
					}
				}
				break;

			case ESMESegmentOverrideMode::Last:

				if (Layer->LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentLength || Layer->LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SemiFixedSegmentLength)
				{
					float StartOffset = FMath::Max(FMath::Min(GapLength, 0.0f), Layer->LayerData.SegmentOverrides[i].StartOffset);
					IsLastSegment = Layer->GetTotalSegmentsLength() + SegmentLength + GapLength + StartOffset + Layer->LayerData.SegmentOverrides[i].EndOffset > Layer->SplineComponent->GetSplineLength() + 1.0f;
				}

				OffsetDistOnSpline = Layer->GetClampedSplineUtilization() * Layer->SplineComponent->GetSplineLength();
				if ((OffsetDistOnSpline <= Layer->GetTotalSegmentsLength() + SegmentLength + GapLength - 1.0f) || IsLastSegment)
				{
					ResultCurrentIteration = ApplySegmentsOverrides(OverrideMesh, i);
				}
				break;

			default:
				break;
			}
		}

		if (ResultCurrentIteration)
		{
			OverrrideOccured = true;
		}

		//Special case when the length of the meshes is not the same on all segments in FixedSegmentCount DistributionMode
		//The length of the overrided meshes should affect the length of the other meshes
		if (Layer->LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentCount
			|| Layer->LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SemiFixedSegmentLength)
		{
			float CumulativeLengthOverrides = 0.0f;
			for (int i = 0; i < Layer->LayerData.SegmentOverrides.Num(); i++)
			{
				float StartOffset;
				float EndOffset;
				/*if (Layer->LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Last)
				{
					StartOffset = FMath::Max(0.0f, Layer->LayerData.SegmentOverrides[i].StartOffset);
					EndOffset = FMath::Max(0.0f, Layer->LayerData.SegmentOverrides[i].EndOffset);
				}
				else
				{*/
					float NormalSegmentLength = Layer->LayerData.MeshesData.MeshDistribution.SegmentsLength;
					StartOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength * 2.0f + NormalSegmentLength), 0.0f), Layer->LayerData.SegmentOverrides[i].StartOffset);
					EndOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength), 0.0f), Layer->LayerData.SegmentOverrides[i].EndOffset);
				//}

				if (Layer->LayerData.SegmentOverrides[i].UseDefaultLength)
				{
					CumulativeLengthOverrides += Layer->LayerData.MeshesData.MeshDistribution.SegmentsLength + StartOffset + EndOffset;
				}
				else
				{
					CumulativeLengthOverrides += Layer->LayerData.SegmentOverrides[i].SegmentLength + StartOffset + EndOffset;
				}
			}

			for (int i = 0; i < Layer->LayerData.SegmentOverrides.Num(); i++)
			{
				float StartOffset;
				float EndOffset;
				/*if (Layer->LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Last)
				{
					StartOffset = FMath::Max(0.0f, Layer->LayerData.SegmentOverrides[i].StartOffset);
					EndOffset = FMath::Max(0.0f, Layer->LayerData.SegmentOverrides[i].EndOffset);
				}
				else
				{*/
					float NormalSegmentLength = Layer->LayerData.MeshesData.MeshDistribution.SegmentsLength;
					StartOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength * 2.0f + NormalSegmentLength), 0.0f), Layer->LayerData.SegmentOverrides[i].StartOffset);
					EndOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(GapLength), 0.0f), Layer->LayerData.SegmentOverrides[i].EndOffset);
			//	}

				OverrideLengthDifference = SegmentLength;
				Layer->SetTotalSegmentsLength(Layer->GetTotalSegmentsLength() - SegmentLength);
				float OverridedSegmentLength = Layer->LayerData.SegmentOverrides[i].SegmentLength;
				if (Layer->LayerData.SegmentOverrides[i].UseDefaultLength)
				{
					OverridedSegmentLength = Layer->LayerData.MeshesData.MeshDistribution.SegmentsLength;
				}

				if (ResultCurrentIteration)//Mesh to override
				{
					SegmentLength = OverridedSegmentLength;
					Layer->SetTotalSegmentsLength(Layer->GetTotalSegmentsLength() + StartOffset);
				}
				else// the other meshes
				{
					int NbInterval = 1;
					const int NumSegments = Layer->GetTotalNumSegments();
					if (Layer->LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Interval)
					{
						NbInterval = FMath::DivideAndRoundUp(NumSegments, Layer->LayerData.SegmentOverrides[i].Interval);
					}
					float Divider = NumSegments - NbInterval;
					int NumSegmentsClosedLoop = NumSegments;
					if (!Layer->IsClosedLoop())
					{
						NumSegmentsClosedLoop--;
					}

					if (Layer->LayerData.SegmentOverrides[i].SegmentOverrideMode == ESMESegmentOverrideMode::Interval)
					{
						SegmentLength = (Layer->SplineComponent->GetSplineLength() * Layer->LayerData.MeshesData.MeshDistribution.SplineUtilization - Layer->GetGapLength() * NumSegmentsClosedLoop - (OverridedSegmentLength + StartOffset + EndOffset) * NbInterval) / Divider;
					}
					else
					{
						Divider = NumSegments - Layer->LayerData.SegmentOverrides.Num();
						SegmentLength = (Layer->SplineComponent->GetSplineLength() * Layer->LayerData.MeshesData.MeshDistribution.SplineUtilization - Layer->GetGapLength() * NumSegmentsClosedLoop - CumulativeLengthOverrides) / Divider;
					}

					if (SegmentLength < SME_SplineLayer::MinMeshLength)
					{
						SegmentLength = SME_SplineLayer::MinMeshLength;
					}
				}
				Layer->SetTotalSegmentsLength(Layer->GetTotalSegmentsLength() + SegmentLength);
				OverrideLengthDifference -= SegmentLength;
			}
		}
	}

	return OverrrideOccured;
}

bool USME_Segment::ApplySegmentsOverrides(UStaticMesh* OverrideMesh, int SegmentOverridesIndex)
{
	Mesh = OverrideMesh;
	OverrideMatchScale = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].ScaleData.MatchLayerScale;
	OverrideScaleWidthStart = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].ScaleData.ScaleWidthStart;
	OverrideScaleHeightStart = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].ScaleData.ScaleHeightStart;
	OverrideScaleWidthEnd = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].ScaleData.ScaleWidthEnd;
	OverrideScaleHeightEnd = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].ScaleData.ScaleHeightEnd;
	ForwardAxisToUse = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].ForwardAxis;
	OverrideRotationOffset = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].RotationOffset;
	OverrideEndOffset = FMath::Max(FMath::Min(-1 * FMath::Abs(Layer->GetGapLength()), 0.0f), Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].EndOffset);
	OverrideLocationOffset = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].LocationOffset;
	OverrideDeform = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].Deform;
	OverrideUseCurveScale = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].UseCurveScale;

	if (Layer->LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::FixedSegmentLength)
	{
		OverrideLengthDifference = SegmentLength;
		Layer->SetTotalSegmentsLength(Layer->GetTotalSegmentsLength() - SegmentLength);
		SegmentLength = Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].SegmentLength;
		Layer->SetTotalSegmentsLength(Layer->GetTotalSegmentsLength() + SegmentLength);
		OverrideLengthDifference -= SegmentLength;
		Layer->SetTotalSegmentsLength(Layer->GetTotalSegmentsLength() + Layer->LayerData.SegmentOverrides[SegmentOverridesIndex].StartOffset);
	}

	return true;
}

