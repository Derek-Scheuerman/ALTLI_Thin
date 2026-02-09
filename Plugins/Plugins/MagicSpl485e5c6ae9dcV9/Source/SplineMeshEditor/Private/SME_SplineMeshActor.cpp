// Copyright 2022 Triple Scale Games. All Rights Reserved.
//#pragma optimize("", off)

#include "SME_SplineMeshActor.h"
#include "Engine/SplineMeshActor.h"
#include "Engine/StaticMesh.h"

// Sets default values
ASME_SplineMeshActor::ASME_SplineMeshActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Rooter"));
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent0"));
	SplineComponent->SetupAttachment(RootComponent);

}

ASME_SplineMeshActor::~ASME_SplineMeshActor()
{
	SplineComponent = nullptr;
	LinkedExternalSplineMeshActor = nullptr;
	DefaultAnchorMesh = nullptr;
	DefaultAnchorMaterial = nullptr;
	ExternalEndAnchorActor = nullptr;
	PreviousExternalSplineActor = nullptr;

	for (int i = 0; i < SplineLayers.Num(); i++)
	{
		SplineLayers[i] = nullptr;
	}
	SplineLayers.Empty();

	for (int i = 0; i < SplineLayersRuntime.Num(); i++)
	{
		SplineLayersRuntime[i] = nullptr;
	}
	SplineLayersRuntime.Empty();	
}

void ASME_SplineMeshActor::OnConstruction(const FTransform& Transform)
{
	/*for (int i = 0; i < SplineLayers.Num(); i++)
	{
		USME_SplineLayer* SplineLayer = SplineLayers[i];
		for (int j = 0; j < SplineLayer->GetSegments().Num(); j++)
		{
			if (SplineLayer->GetSegments()[j]->GetSplineMeshComponent())
			{
				SplineLayer->GetSegments()[j]->GetSplineMeshComponent()->CachePaintedDataIfNecessary();
			}
			else if (SplineLayer->GetSegments()[j]->GetStaticMeshComponent())
			{
				SplineLayer->GetSegments()[j]->GetStaticMeshComponent()->CachePaintedDataIfNecessary();
			}
		}
	}*/

	Super::OnConstruction(Transform);

	ConstructSplineMeshActor();
}

void ASME_SplineMeshActor::ConstructSplineMeshActor()
{
	USplineComponent* SplineComponentToUse = SplineComponent;

	/*TArray<UActorComponent*> ActorComponents = GetComponents().Array();
	TArray<UActorComponent*> ActorComponentsToDestroy;
	TArray<UActorComponent*> NewActorComponents;
	for (int i = 0; i < ActorComponents.Num(); i++)
	{
		UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(ActorComponents[i]);
		if (IsValid(StaticMesh) && StaticMesh->GetName().Contains("SplineLayerComponent"))
		{
			ActorComponentsToDestroy.Add(StaticMesh);
		}

		USplineMeshComponent* SplineMesh = Cast<USplineMeshComponent>(ActorComponents[i]);
		if (IsValid(SplineMesh) && SplineMesh->GetName().Contains("SplineMeshComponent"))
		{
			ActorComponentsToDestroy.Add(SplineMesh);
		}
	}*/
	SplineLayers.Empty();
	SplineLayersRuntime.Empty();

	//Set the ExternalSplineActor
	ASME_SplineMeshActor* ExternalSMESplineActor = Cast<ASME_SplineMeshActor>(ExternalSplineActor);
	if (IsValid(ExternalSplineActor) && !IsValid(ExternalSMESplineActor))
	{
		//Search for USplineComponent
		TArray<UActorComponent*> ExternalComponents = ExternalSplineActor->GetComponents().Array();
		for (int i = 0; i < ExternalComponents.Num(); i++)
		{
			USplineComponent* ExternalSplineComponent = Cast<USplineComponent>(ExternalComponents[i]);
			if (IsValid(ExternalSplineComponent))
			{
				SplineComponentToUse = ExternalSplineComponent;
				SetActorLocation(ExternalSplineActor->GetActorLocation());
				SetActorRotation(ExternalSplineActor->GetActorRotation());
				PreviousExternalSplineActor = ExternalSplineActor;
				break;
			}
		}
	}
	else if (IsValid(ExternalSMESplineActor))
	{
		SplineComponentToUse = ExternalSMESplineActor->SplineComponent;
		ExternalSMESplineActor->LinkedExternalSplineMeshActor = this;
		PreviousExternalSplineActor = ExternalSMESplineActor;

		ExternalSMESplineActor->ShouldUpdateLinkedExternalSpline = false;
		ExternalSMESplineActor->UpdateConstruction();
		ExternalSMESplineActor->ShouldUpdateLinkedExternalSpline = true;
	}

	if (IsValid(SplineComponentToUse))
	{
		NumSplinePoints = SplineComponentToUse->GetNumberOfSplinePoints();

		if (ArcSettings.MakeArc)
		{
			if (ArcSettings.SyncRadius && ArcSettings.CircularRadius.X != ArcSettings.CircularRadius.Y)
			{
				ArcSettings.CircularRadius.Y = ArcSettings.CircularRadius.X;
			}

			MakeSplineArc(SplineComponentToUse, ArcSettings.NumArcPoints, ArcSettings.CircularRadius, ArcSettings.PointRotationOffset, ArcSettings.DegreesOfArc, ClosedLoop, SplinePointMode);
		}
		else
		{
			SplineComponentToUse->SetClosedLoop(ClosedLoop);
		}

		//Segment curved or linear
		for (int i = 0; i < SplineComponentToUse->GetNumberOfSplinePoints(); i++)
		{
			if (SplinePointMode == ESMESplinePointType::Linear)
			{
				SplineComponentToUse->SetSplinePointType(i, ESplinePointType::Linear);
			}
			else if (SplinePointMode == ESMESplinePointType::Curved)
			{
				SplineComponentToUse->SetSplinePointType(i, ESplinePointType::Curve);
			}
			else if (SplinePointMode == ESMESplinePointType::UserDefined || i == SplineComponentToUse->GetNumberOfSplinePoints() - 1)
			{
				SplineComponentToUse->SetSplinePointType(i, ESplinePointType::CurveCustomTangent);
			}
		}

		if (ArcSettings.MakeArc /* || SplinePointMode == ESMESplinePointType::Curved*/)
		{
			if (SplineComponentToUse->GetSplinePointType(0) >= ESplinePointType::Curve)
			{
				//Fix 1st curved tangent
				SetElipseTangent(SplineComponentToUse, 0, 1, ArcSettings.CircularRadius);
			}
			if (SplineComponentToUse->GetSplinePointType(SplineComponentToUse->GetNumberOfSplinePoints() - 1) >= ESplinePointType::Curve)
			{
				//Fix last curved tangent
				SetElipseTangent(SplineComponentToUse, NumSplinePoints - 1, NumSplinePoints - 2, ArcSettings.CircularRadius);
			}
		}
		//Create the layers based on the SplineLayersData edited in the editor
		for (int i = 0; i < SplineLayersData.Num(); i++)
		{			
			USME_SplineLayer* NewSplineLayer = nullptr;

			FString ComponentName = GetName().Append(FString::Printf(TEXT("SplineLayerComponent%d"), i));
			NewSplineLayer = NewObject<USME_SplineLayer>(USME_SplineLayer::StaticClass(), *ComponentName);
			

			if (IsValid(NewSplineLayer))
			{
				NewSplineLayer->LayerData = SplineLayersData[i];
				NewSplineLayer->Mobility = Mobility;

				if (DuplicateSegmentsPercentages && i > 0)
				{
					NewSplineLayer->LayerData.MeshesData.MeshDistribution.SegmentStartPercentage = SplineLayersData[0].MeshesData.MeshDistribution.SegmentStartPercentage;
					NewSplineLayer->LayerData.MeshesData.MeshDistribution.SplineUtilization = SplineLayersData[0].MeshesData.MeshDistribution.SplineUtilization;
					NewSplineLayer->LayerData.MeshesData.MeshDistribution.OnlyUseRemainingLength = SplineLayersData[0].MeshesData.MeshDistribution.OnlyUseRemainingLength;
					SplineLayersData[i].MeshesData.MeshDistribution.SegmentStartPercentage = NewSplineLayer->LayerData.MeshesData.MeshDistribution.SegmentStartPercentage;
					SplineLayersData[i].MeshesData.MeshDistribution.SplineUtilization = NewSplineLayer->LayerData.MeshesData.MeshDistribution.SplineUtilization;
					SplineLayersData[i].MeshesData.MeshDistribution.OnlyUseRemainingLength = NewSplineLayer->LayerData.MeshesData.MeshDistribution.OnlyUseRemainingLength;
				}

				for (int j = 0; j < NewSplineLayer->LayerData.SegmentOverrides.Num(); j++)
				{
					if (SplineLayersData[i].SegmentOverrides[j].UseDefaultLength)
					{
						SplineLayersData[i].SegmentOverrides[j].SegmentLength = NewSplineLayer->LayerData.MeshesData.MeshDistribution.SegmentsLength;
						NewSplineLayer->LayerData.SegmentOverrides[j].SegmentLength = NewSplineLayer->LayerData.MeshesData.MeshDistribution.SegmentsLength;
					}

					SplineLayersData[i].SegmentOverrides[j].IsStaticMesh = NewSplineLayer->LayerData.MeshesData.UseStaticMeshes;
					NewSplineLayer->LayerData.SegmentOverrides[j].IsStaticMesh = NewSplineLayer->LayerData.MeshesData.UseStaticMeshes;
				}

				NewSplineLayer->Init(SplineComponentToUse, ClosedLoop);

				/*for (int j = 0; j < NewSplineLayer->GetSegments().Num(); j++)
				{
					if (NewSplineLayer->GetSegments()[j]->GetSplineMeshComponent())
					{
						NewActorComponents.Add(NewSplineLayer->GetSegments()[j]->GetSplineMeshComponent());
					}
					else if (NewSplineLayer->GetSegments()[j]->GetStaticMeshComponent())
					{
						NewActorComponents.Add(NewSplineLayer->GetSegments()[j]->GetStaticMeshComponent());
					}
				}*/		

				SplineLayersData[i].MeshesData.MeshDistribution.SegmentsLength = NewSplineLayer->LayerData.MeshesData.MeshDistribution.SegmentsLength;
				SplineLayersData[i].MeshesData.MeshDistribution.NumSegments = NewSplineLayer->GetNumSegments();
				SplineLayersData[i].MeshesData.MeshDistribution.SplineUtilization = NewSplineLayer->GetClampedSplineUtilization();

				if (NewSplineLayer->LayerData.MeshesData.MeshDistribution.UseDefaultMeshSize)
				{
					SplineLayersData[i].MeshesData.MeshDistribution.TargetSegmentLength = NewSplineLayer->LayerData.MeshesData.MeshDistribution.TargetSegmentLength;
				}

				if (NewSplineLayer->LayerData.MeshesData.MeshDistribution.DistributionMode != ESMEStretchMode::SemiFixedSegmentLength)
				{
					SplineLayersData[i].MeshesData.MeshDistribution.TargetSegmentLength = NewSplineLayer->LayerData.MeshesData.MeshDistribution.SegmentsLength;
				}

				if (NewSplineLayer->LayerData.MeshesData.UseStaticMeshes)
				{
					SplineLayersData[i].MeshesData.DeformSegments = false;
				}

				for (int j = 0; j < NewSplineLayer->LayerData.SegmentOverrides.Num(); j++)
				{
					if (NewSplineLayer->LayerData.MeshesData.MeshDistribution.DistributionMode == ESMEStretchMode::SegmentsPerSplinePoint)
					{
						SplineLayersData[i].SegmentOverrides[j].UseDefaultLength = true;
					}

					if (NewSplineLayer->LayerData.SegmentOverrides[j].UseDefaultMesh && SplineLayersData[i].Meshes.Num() != 0)
					{
						SplineLayersData[i].SegmentOverrides[j].Mesh = SplineLayersData[i].Meshes[0];
					}

					if (!NewSplineLayer->LayerData.SegmentOverrides[j].UseDefaultLength)
					{
						SplineLayersData[i].SegmentOverrides[j].SegmentLength = NewSplineLayer->LayerData.SegmentOverrides[j].SegmentLength;
					}
					SplineLayersData[i].SegmentOverrides[j].StartOffset = NewSplineLayer->LayerData.SegmentOverrides[j].StartOffset;
					SplineLayersData[i].SegmentOverrides[j].EndOffset = NewSplineLayer->LayerData.SegmentOverrides[j].EndOffset;
					//SplineLayersData[i].SegmentOverrides[j].SegmentIndex = NewSplineLayer->LayerData.SegmentOverrides[j].SegmentIndex;
				}
								
				if (Mobility == EComponentMobility::Movable && bIsRuningBeginPlay)
				{
					SplineLayersRuntime.Add(NewSplineLayer);
				}
				SplineLayers.Add(NewSplineLayer);
			}
		}
#if ENGINE_MAJOR_VERSION >= 5
		//Physics
		if (PhysicData.SetupPhysicsConnections)
		{
			InitPhysicsConstraints();
		}
#endif
	}

	float BaseSplineUtilization = 1.0f;
	if (SplineLayers.Num() > 0)
	{
		BaseSplineUtilization = SplineLayers[0]->LayerData.MeshesData.MeshDistribution.SplineUtilization;
	}
	//Branches
	for (int i = 0; i < Branches.Num(); i++)
	{
		Branches[i].PositionOnSpline = FMath::Clamp(Branches[i].PositionOnSpline, 0.0f, SplineComponent->GetSplineLength());

		AActor* Branch = Branches[i].Branch;
		if (IsValid(Branch))
		{
			FTransform InitialTransform = Branch->GetTransform();
			FVector Offset = GetActorRotation().RotateVector(Branches[i].Offset);
			Branch->SetActorRelativeLocation(SplineComponent->GetLocationAtDistanceAlongSpline(Branches[i].PositionOnSpline, ESplineCoordinateSpace::World) + Offset);
			Branch->SetActorRelativeRotation(SplineComponent->GetRotationAtDistanceAlongSpline(Branches[i].PositionOnSpline, ESplineCoordinateSpace::World) + Branches[i].Rotation);

			ASME_SplineMeshActor* SplineMeshActor = Cast<ASME_SplineMeshActor>(Branch);
			if (IsValid(SplineMeshActor))
			{
				if (SplineLayers.Num() > 0)
				{
					SplineMeshActor->SetActorRelativeLocation(SplineComponent->GetLocationAtDistanceAlongSpline(BaseSplineUtilization * Branches[i].PositionOnSpline, ESplineCoordinateSpace::World) + Offset);
					SplineMeshActor->SetActorRelativeRotation(SplineComponent->GetRotationAtDistanceAlongSpline(BaseSplineUtilization * Branches[i].PositionOnSpline, ESplineCoordinateSpace::World) + Branches[i].Rotation);
				}
			}
			else
			{
				Branch->SetActorRelativeRotation(GetActorRotation() + SplineComponent->GetRotationAtDistanceAlongSpline(Branches[i].PositionOnSpline, ESplineCoordinateSpace::Local) + Branches[i].Rotation);
			}

			Branch->UserConstructionScript();
			if (PreviousBranches.Num() == 0 || PreviousBranches.Find(Branch) == INDEX_NONE)
			{
				PreviousBranches.Add(Branch);
				PreviousBranchesInitialTransform.Add(InitialTransform);
			}
		}
		else if (i < PreviousBranches.Num())
		{
			AActor* PreviousBranch = PreviousBranches[i];
			if (IsValid(PreviousBranch))
			{
				PreviousBranches.Remove(PreviousBranch);
				if (i < PreviousBranchesInitialTransform.Num())
				{
					PreviousBranch->SetActorTransform(PreviousBranchesInitialTransform[i]);
					PreviousBranch->UserConstructionScript();
					PreviousBranchesInitialTransform.RemoveAt(i);
				}
			}
		}
	}

	//Clean deleted branches
	for (int i = 0; i < PreviousBranches.Num(); i++)
	{
		int indexFound = FindBranch(PreviousBranches[i]);

		if (indexFound == INDEX_NONE && IsValid(PreviousBranches[i]) && PreviousBranchesInitialTransform.Num() > i)
		{
			PreviousBranches[i]->SetActorTransform(PreviousBranchesInitialTransform[i]);
			PreviousBranches[i]->UserConstructionScript();
			PreviousBranchesInitialTransform.RemoveAt(i);
			PreviousBranches.RemoveAt(i);
		}
	}

	if (!IsValid(ExternalSplineActor) && IsValid(PreviousExternalSplineActor))
	{
		if (PreviousExternalSplineActor->IsValidLowLevel())
		{
			//Remove StaticMeshcompoment attached when removing the External Spline Actor
			TArray<UActorComponent*> Components = PreviousExternalSplineActor->GetComponents().Array();
			for (int i = 0; i < Components.Num(); i++)
			{
				UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Components[i]);
				if (IsValid(Mesh) && Mesh->GetName().Contains("SplineLayerComponent"))
				{
					Mesh->DestroyComponent();
				}
			}
			ASME_SplineMeshActor* PreviousExternalIMSplineActor = Cast<ASME_SplineMeshActor>(PreviousExternalSplineActor);
			if (PreviousExternalIMSplineActor->IsValidLowLevel())
			{
				PreviousExternalIMSplineActor->LinkedExternalSplineMeshActor = nullptr;
			}
			PreviousExternalSplineActor->UserConstructionScript();
		}
		PreviousExternalSplineActor = nullptr;
	}

	//Update the External Spline Actor
	if (IsValid(LinkedExternalSplineMeshActor) && ShouldUpdateLinkedExternalSpline)
	{
		LinkedExternalSplineMeshActor->UpdateConstruction();
	}


	//Copy vertex info from the previous components to new components, then destroy the old ones
	/*for (int i = 0; i < NewActorComponents.Num(); i++)
	{
		if (ActorComponentsToDestroy.Num() > i)
		{
			UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(NewActorComponents[i]);
			UStaticMeshComponent* OldStaticMesh = Cast<UStaticMeshComponent>(ActorComponentsToDestroy[i]);
			if (IsValid(StaticMesh) && IsValid(OldStaticMesh))
			{
				StaticMesh->CopyInstanceVertexColorsIfCompatible(OldStaticMesh);
			}
			USplineMeshComponent* SplineMesh = Cast<USplineMeshComponent>(NewActorComponents[i]);
			USplineMeshComponent* OldSplineMesh = Cast<USplineMeshComponent>(ActorComponentsToDestroy[i]);
			if (IsValid(SplineMesh) && IsValid(OldSplineMesh))
			{
				SplineMesh->CopyInstanceVertexColorsIfCompatible(OldSplineMesh);
			}
		}		
	}
	for (int i = 0; i < ActorComponentsToDestroy.Num(); i++)
	{
		ActorComponentsToDestroy[i]->DestroyComponent();
	}
	NewActorComponents.Empty();
	ActorComponentsToDestroy.Empty();
	*/
}

void ASME_SplineMeshActor::Destroyed()
{
	if (IsValid(ExternalSplineActor))
	{
		TArray<UActorComponent*> ActorComponents = ExternalSplineActor->GetComponents().Array();
		for (int i = 0; i < ActorComponents.Num(); i++)
		{
			UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(ActorComponents[i]);
			if (IsValid(Mesh) && Mesh->GetName().Contains("SplineLayerComponent"))
			{
				Mesh->DestroyComponent();
			}
		}
	}

	Super::Destroyed();
}

void ASME_SplineMeshActor::UpdateConstruction()
{
	bool bAllowReconstruction = !bActorSeamlessTraveled && !IsValid(this) && !HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed);
	if (bAllowReconstruction)
	{
		UserConstructionScript();
	}
}

void ASME_SplineMeshActor::UpdateSplineAtRuntime()
{
	//ConstructSplineMeshActor();

	if (Mobility == EComponentMobility::Movable)
	{
		for (int i = 0; i < SplineLayersRuntime.Num(); i++)
		{
			SplineLayersRuntime[i]->UpdateAtRuntime();
		}
	}
}

void ASME_SplineMeshActor::SetSplineUtilization(float value)
{
	float clampedValue = FMath::Clamp(value, 0.0f, 1.1f);

	if (Mobility == EComponentMobility::Movable)
	{
		for (int i = 0; i < SplineLayersRuntime.Num(); i++)
		{
			SplineLayersRuntime[i]->LayerData.MeshesData.MeshDistribution.SplineUtilization = clampedValue;
		}
	}
}

float ASME_SplineMeshActor::GetSplineUtilization()
{
	if (SplineLayersRuntime.Num() > 0)
	{
		return SplineLayersRuntime[0]->LayerData.MeshesData.MeshDistribution.SplineUtilization;
	}
	return 0.0f;
}

// Called when the game starts or when spawned
void ASME_SplineMeshActor::BeginPlay()
{
	Super::BeginPlay();

	if (Mobility == EComponentMobility::Movable)
	{
		bIsRuningBeginPlay = true;
		ConstructSplineMeshActor();
		bIsRuningBeginPlay = false;
	}

#if ENGINE_MAJOR_VERSION >= 5
	if (PhysicData.SetupPhysicsConnections && BreackablePhysicsConstraints)
	{
		for (int i = 0; i < PhysicsConstraints.Num(); i++)
		{
			UPrimitiveComponent* PrimitiveComponentHitable = nullptr;
			FName BoneName;
			PhysicsConstraints[i]->GetConstrainedComponents(PrimitiveComponentHitable, BoneName, PrimitiveComponentHitable, BoneName);
			if (IsValid(PrimitiveComponentHitable))
			{
				PrimitiveComponentHitable->SetNotifyRigidBodyCollision(true);
				PrimitiveComponentHitable->OnComponentHit.AddDynamic(this, &ASME_SplineMeshActor::OnHit);
				//PrimitiveComponentHitable->bApplyImpulseOnDamage .AddDynamic(this, &ASME_SplineMeshActor::Damaged);
			}
		}
	}
#endif
}

// Called every frame
void ASME_SplineMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASME_SplineMeshActor::MakeSplineArc(USplineComponent* SplineComp, int NumPoints, FVector2D Radius, float RotationOffset, float Degrees, bool Closed, ESMESplinePointType PointMode)
{
	/*TArray<int> SplinePointTypes;
	TArray<FVector> SplinePointTangents;
	if (PointMode == ESMESplinePointType::UserDefined)
	{
		for (int i = 0; i < NumPoints; i++)
		{
			SplinePointTypes.Add(SplineComp->GetSplinePointType(i));
			SplinePointTangents.Add(SplineComp->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local));
		}
	}*/

	SplineComp->ClearSplinePoints(true);
	SplineComp->SetClosedLoop(Closed);

	int ClosedLoopExtraPoint = 1;
	if (Closed)
	{
		ClosedLoopExtraPoint = 0;
	}

	FRotator Rotation = FRotator::ZeroRotator;
	for (int i = 0; i < NumPoints; i++)
	{
		float RangeValueClamped = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, NumPoints - ClosedLoopExtraPoint), FVector2D(0.0f, Degrees), i);
		RangeValueClamped += RotationOffset;
		float PositionX = FMath::Sin(PI / (180.f) * RangeValueClamped) * Radius.X;
		float PositionY = FMath::Cos(PI / (180.f) * RangeValueClamped) * Radius.Y;
		FVector NewPoint = FVector(PositionX, PositionY, 0.0f);
		SplineComp->AddSplinePoint(NewPoint, ESplineCoordinateSpace::Local, false);
		if (PointMode == ESMESplinePointType::UserDefined)
		{
			/*if (SplinePointTypes[i] == 0)
			{
				SplineComp->SetSplinePointType(i, ESplinePointType::Linear);
			}
			else
			{*/
				
				//SplineComp->SetSplinePointType(i, ESplinePointType::CurveCustomTangent, false);
				//if (i > 0)
				//{
				//	SetElipseTangent(SplineComp, i, i - 1, Radius);
				//}
			//	SplineComp->SetTangentAtSplinePoint(i, SplinePointTangents[i], ESplineCoordinateSpace::Local);
			//}
		}
	}

	if (PointMode == ESMESplinePointType::UserDefined)
	{
		for (int i = 0; i < NumPoints; i++)
		{
			SplineComp->SetSplinePointType(i, ESplinePointType::CurveCustomTangent);
		}
	}
	
}

void ASME_SplineMeshActor::SetElipseTangent(USplineComponent* SplineComp, int TargetPointIndex, int ReferencePointIndex, FVector2D Radius)
{
	float LongRadius = FMath::Max(Radius.X, Radius.Y);
	float ShortRadius = FMath::Min(Radius.X, Radius.Y);
	float BSideLength1 = FMath::Sqrt(FMath::Square(LongRadius) - FMath::Square(ShortRadius));
	float BSideLength2 = BSideLength1 * (-1);

	FVector TargetLocation = SplineComp->GetLocationAtSplinePoint(TargetPointIndex, ESplineCoordinateSpace::Local);
	FVector ReferenceLocation = SplineComp->GetLocationAtSplinePoint(ReferencePointIndex, ESplineCoordinateSpace::Local);
	float Distance = FVector::Dist(TargetLocation, ReferenceLocation);

	FVector VectorA = FVector::ZeroVector;
	FVector VectorB = FVector::ZeroVector;
	if (Radius.X > Radius.Y)
	{
		VectorA = (FVector(BSideLength1, 0, 0) - TargetLocation).GetSafeNormal();
		VectorB = (FVector(BSideLength2, 0, 0) - TargetLocation).GetSafeNormal();
	}
	else
	{
		VectorA = (FVector(0, BSideLength1, 0) - TargetLocation).GetSafeNormal();
		VectorB = (FVector(0, BSideLength2, 0) - TargetLocation).GetSafeNormal();
	}
	FVector LineraInterp = FMath::Lerp(VectorA, VectorB, 0.5f);
	FRotator Rotation = FRotationMatrix::MakeFromX(LineraInterp).Rotator();
	FVector RightVector = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
	FVector ElipseTangent = RightVector * Distance;

	SplineComp->SetTangentAtSplinePoint(TargetPointIndex, ElipseTangent, ESplineCoordinateSpace::Local);
}

#if ENGINE_MAJOR_VERSION >= 5
void ASME_SplineMeshActor::InitPhysicsConstraints()
{
	if (PhysicData.SetupPhysicsConnections)
	{
		PhysicsConstraints.Empty();
		for (int i = 0; i < SplineLayers.Num(); i++)
		{
			if (SplineLayers[i]->LayerData.MeshesData.UseStaticMeshes)
			{
				UStaticMeshComponent* StaticMeshComp1 = nullptr;
				UStaticMeshComponent* EndPoint = nullptr;
				float DistanceBetweenMeshes = 0.0f;
				for (int j = 0; j < SplineLayers[i]->GetSegments().Num(); j++)
				{
					UStaticMeshComponent* StaticMeshComp2 = SplineLayers[i]->GetSegments()[j]->GetStaticMeshComponent();
					if (IsValid(StaticMeshComp2))
					{
						if (j == 0)
						{
							FTransform StartPointRelativeTransform;
							if (SplineLayers[i]->GetSegments().Num() > j + 1 && IsValid(SplineLayers[i]->GetSegments()[j + 1]->GetStaticMeshComponent()))
							{
								DistanceBetweenMeshes = FVector::Dist(StaticMeshComp2->GetRelativeTransform().GetLocation(), SplineLayers[i]->GetSegments()[j + 1]->GetStaticMeshComponent()->GetRelativeTransform().GetLocation());
							}

							StaticMeshComp1 = Cast<UStaticMeshComponent>(AddComponentByClass(UStaticMeshComponent::StaticClass(), false, StartPointRelativeTransform, false));
							StaticMeshComp1->SetRelativeLocation(SplineComponent->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::Local) - SplineComponent->GetDirectionAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::Local) * DistanceBetweenMeshes / 2.0f);
							if (DefaultAnchorMesh != nullptr)
							{
								StaticMeshComp1->SetStaticMesh(DefaultAnchorMesh);
								if (DefaultAnchorMaterial != nullptr)
								{
									StaticMeshComp1->SetMaterial(0, DefaultAnchorMaterial);
								}
							}
							else
							{
								StaticMeshComp1->SetStaticMesh(StaticMeshComp2->GetStaticMesh());
							}
							StaticMeshComp1->SetHiddenInGame(HideDefaultAnchorInGame);
							SetNeutralPhysics(StaticMeshComp1);
						}
						else
						{
							StaticMeshComp1 = SplineLayers[i]->GetSegments()[j - 1]->GetStaticMeshComponent();
							AddPhysicsConstraint(StaticMeshComp1, StaticMeshComp2);
						}

						StaticMeshComp2->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
						StaticMeshComp2->SetCollisionProfileName("BlockAll");
						StaticMeshComp2->SetSimulatePhysics(true);
						AddPhysicsConstraint(StaticMeshComp1, StaticMeshComp2);
					}
				}

				if (PhysicData.AttachEnd)
				{
					FTransform EndPointTransform;
					UStaticMeshComponent* LastComp = SplineLayers[i]->GetSegments().Last()->GetStaticMeshComponent();

					if (IsValid(ExternalEndAnchorActor))
					{
						TArray<UActorComponent*> ExternalComponents = ExternalEndAnchorActor->GetComponents().Array();
						for (int j = 0; j < ExternalComponents.Num(); j++)
						{
							UStaticMeshComponent* ExternalStaticMeshComponent = Cast<UStaticMeshComponent>(ExternalComponents[j]);
							if (IsValid(ExternalStaticMeshComponent))
							{
								EndPoint = ExternalStaticMeshComponent;
								break;
							}
						}
					}

					if(!IsValid(EndPoint))
					{
						EndPoint = Cast<UStaticMeshComponent>(AddComponentByClass(UStaticMeshComponent::StaticClass(), false, EndPointTransform, false));
						EndPoint->SetRelativeLocation(SplineComponent->GetLocationAtDistanceAlongSpline(SplineComponent->GetSplineLength(), ESplineCoordinateSpace::Local) + SplineComponent->GetDirectionAtDistanceAlongSpline(SplineComponent->GetSplineLength(), ESplineCoordinateSpace::Local) * DistanceBetweenMeshes / 2.0f);

						if (DefaultAnchorMesh != nullptr)
						{
							EndPoint->SetStaticMesh(DefaultAnchorMesh);
							if (DefaultAnchorMaterial != nullptr)
							{
								EndPoint->SetMaterial(0, DefaultAnchorMaterial);
							}
						}
						else
						{
							EndPoint->SetStaticMesh(LastComp->GetStaticMesh());
						}
						EndPoint->SetHiddenInGame(HideDefaultAnchorInGame);
					}

					SetNeutralPhysics(EndPoint);
					AddPhysicsConstraint(LastComp, EndPoint);
				}
			}
		}
	}
}

void ASME_SplineMeshActor::SetNeutralPhysics(UStaticMeshComponent* StaticMeshComp)
{
	StaticMeshComp->SetCollisionProfileName("Spectator");
	StaticMeshComp->SetAngularDamping(PhysicData.AngularLimits.AngularDamping);
	StaticMeshComp->SetMassOverrideInKg(NAME_None, PhysicData.AngularLimits.Mass);
}

void ASME_SplineMeshActor::AddPhysicsConstraint(UStaticMeshComponent* Comp1, UStaticMeshComponent* Comp2)
{
	FTransform PhysicsConstraintComponentTransform;
	PhysicsConstraintComponentTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));

	UPhysicsConstraintComponent* NewPhysicsConstraintComponent = Cast<UPhysicsConstraintComponent>(AddComponentByClass(UPhysicsConstraintComponent::StaticClass(), true, PhysicsConstraintComponentTransform, false));

	if (PhysicsConstraints.Num() == 0)
	{
		NewPhysicsConstraintComponent->AttachToComponent(Comp1, FAttachmentTransformRules::KeepRelativeTransform);
	}
	else
	{
		NewPhysicsConstraintComponent->AttachToComponent(Comp2, FAttachmentTransformRules::KeepRelativeTransform);
	}

	NewPhysicsConstraintComponent->SetDisableCollision(true);
	NewPhysicsConstraintComponent->SetConstrainedComponents(Comp1, NAME_None, Comp2, NAME_None);

	NewPhysicsConstraintComponent->SetAngularSwing1Limit(PhysicData.AngularLimits.Swing1LimitMode, PhysicData.AngularLimits.Swing1LimitAmount);
	NewPhysicsConstraintComponent->SetAngularSwing2Limit(PhysicData.AngularLimits.Swing2LimitMode, PhysicData.AngularLimits.Swing2LimitAmount);
	NewPhysicsConstraintComponent->SetAngularTwistLimit(PhysicData.AngularLimits.TwistLimitMode, PhysicData.AngularLimits.TwistLimitAmount);
	NewPhysicsConstraintComponent->SetLinearXLimit(PhysicData.LinearLimits.xMotion, PhysicData.LinearLimits.LimitAmount);
	NewPhysicsConstraintComponent->SetLinearYLimit(PhysicData.LinearLimits.yMotion, PhysicData.LinearLimits.LimitAmount);
	NewPhysicsConstraintComponent->SetLinearZLimit(PhysicData.LinearLimits.zMotion, PhysicData.LinearLimits.LimitAmount);

	PhysicsConstraints.Add(NewPhysicsConstraintComponent);
}
#endif

void ASME_SplineMeshActor::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
#if ENGINE_MAJOR_VERSION >= 5
	for (int i = 0; i < PhysicsConstraints.Num(); i++)
	{
		UPrimitiveComponent* PrimitiveComponentHitable = nullptr;
		FName BoneName;
		PhysicsConstraints[i]->GetConstrainedComponents(PrimitiveComponentHitable, BoneName, PrimitiveComponentHitable, BoneName);
		if (IsValid(PrimitiveComponentHitable) && HitComponent == PrimitiveComponentHitable)
		{
			PhysicsConstraints[i]->BreakConstraint();
		}
	}
#endif
}

int ASME_SplineMeshActor::FindBranch(AActor* ActorToFind)
{
	int IndexFound = INDEX_NONE;
	for (int j = 0; j < Branches.Num(); j++)
	{
		AActor* Branch = Branches[j].Branch;
		if (Branch == ActorToFind)
		{
			return j;
		}
	}
	return IndexFound;
}

